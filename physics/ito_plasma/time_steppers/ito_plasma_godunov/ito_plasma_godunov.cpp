/*!
  @file   ito_plasma_godunov.cpp
  @author Robert Marskar
  @date   June 2020
  @brief  Implementation of ito_plasma_godunov
*/

#include "ito_plasma_godunov.H"
#include "data_ops.H"
#include "units.H"
#include "poisson_multifluid_gmg.H"

#include <ParmParse.H>

using namespace physics::ito_plasma;

ito_plasma_godunov::ito_plasma_godunov(RefCountedPtr<ito_plasma_physics>& a_physics){
  m_name    = "ito_plasma_godunov";
  m_physics = a_physics;

  m_dt_relax = 1.E99;

  ParmParse pp("ito_plasma_godunov");
  pp.get("particle_realm", m_particle_realm);
  pp.get("profile", m_profile);

  m_avg_cfl = 0.0;


}

ito_plasma_godunov::~ito_plasma_godunov(){

}

int ito_plasma_godunov::get_num_plot_vars() const {
  CH_TIME("ito_plasma_godunov::get_num_plot_vars");
  if(m_verbosity > 5){
    pout() << "ito_plasma_godunov::get_num_plot_vars" << endl;
  }

  int ncomp = ito_plasma_stepper::get_num_plot_vars();

  ncomp++; // Add conductivity

  return ncomp;
}

void ito_plasma_godunov::write_plot_data(EBAMRCellData& a_output, Vector<std::string>& a_plotvar_names, int& a_icomp) const {
  CH_TIME("ito_plasma_godunov::write_conductivity");
  if(m_verbosity > 5){
    pout() << "ito_plasma_godunov::write_conductivity" << endl;
  }

  ito_plasma_stepper::write_plot_data(a_output, a_plotvar_names, a_icomp);

  // Do conductivity
  this->write_conductivity(a_output, a_icomp);
  a_plotvar_names.push_back("conductivity");
}

void ito_plasma_godunov::write_conductivity(EBAMRCellData& a_output, int& a_icomp) const {
  CH_TIME("ito_plasma_stepper::write_conductivity");
  if(m_verbosity > 5){
    pout() << "ito_plasma_stepper::write_conductivity" << endl;
  }

  const Interval src_interv(0, 0);
  const Interval dst_interv(a_icomp, a_icomp);

  for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
    if(m_conduct_cell.get_realm() == a_output.get_realm()){
      m_conduct_cell[lvl]->localCopyTo(src_interv, *a_output[lvl], dst_interv);
    }
    else {
      m_conduct_cell[lvl]->copyTo(src_interv, *a_output[lvl], dst_interv);
    }
  }
  a_icomp += 1;
}

void ito_plasma_godunov::allocate(){
  CH_TIME("ito_plasma_godunov::allocate");
  if(m_verbosity > 5){
    pout() << "ito_plasma_godunov::allocate" << endl;
  }

  m_ito->allocate_internals();
  m_rte->allocate_internals();
  m_poisson->allocate_internals();
  m_sigma->allocate_internals();

  // Now allocate for the conductivity particles and rho^dagger particles
  const int num_ito_species = m_physics->get_num_ito_species();
  
  m_conductivity_particles.resize(num_ito_species);
  m_rho_dagger_particles.resize(num_ito_species);
  
  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    const RefCountedPtr<ito_solver>& solver = solver_it();

    const int idx         = solver_it.get_solver();
    const int pvr_buffer  = solver->get_pvr_buffer();
    const int halo_buffer = solver->get_halo_buffer();

    m_conductivity_particles[idx] = new particle_container<godunov_particle>();
    m_rho_dagger_particles[idx]   = new particle_container<godunov_particle>();
    
    m_amr->allocate(*m_conductivity_particles[idx], pvr_buffer, halo_buffer, m_particle_realm);
    m_amr->allocate(*m_rho_dagger_particles[idx],   pvr_buffer, halo_buffer, m_particle_realm);
  }
}

void ito_plasma_godunov::parse_options() {
  CH_TIME("ito_plasma_godunov::parse_options");
  if(m_verbosity > 5){
    pout() << m_name + "::parse_options" << endl;
  }

  ParmParse pp(m_name.c_str());
  std::string str;
  
  pp.get("verbosity",      m_verbosity);
  pp.get("ppc",            m_ppc);
  pp.get("max_cells_hop",  m_max_cells_hop);
  pp.get("merge_interval", m_merge_interval);
  pp.get("relax_factor",   m_relax_factor);
  pp.get("regrid_super",   m_regrid_superparticles);
  pp.get("algorithm",      str);
  pp.get("load_balance",   m_load_balance);
  pp.get("min_dt",         m_min_dt);
  pp.get("max_dt",         m_max_dt);


  if(str == "euler"){
    m_algorithm = which_algorithm::euler;
  }
  else if(str == "euler_maruyama"){
    m_algorithm = which_algorithm::euler_maruyama;
  }
  else if(str ==  "midpoint"){
    m_algorithm = which_algorithm::midpoint;
  }
  else{
    MayDay::Abort("ito_plasma_godunov::parse_options - unknown algorithm requested");
  }


  pp.get("which_dt", str);
  if(str == "advection"){
    m_whichDt = which_dt::advection;
  }
  else if(str == "diffusion"){
    m_whichDt = which_dt::diffusion;
  }
  else if(str == "advection_diffusion"){
    m_whichDt = which_dt::advection_diffusion;
  }
  else{
    MayDay::Abort("ito_plasma_godunov::parse_options - unknown 'which_dt' requested");
  }

  // Setup runtime storage (requirements change with algorithm)
  this->setup_runtime_storage();
}

void ito_plasma_godunov::allocate_internals(){
  CH_TIME("ito_plasma_godunov::allocate_internals");
  if(m_verbosity > 5){
    pout() << m_name + "::allocate_internals" << endl;
  }

  m_amr->allocate(m_fluid_scratch1,    m_fluid_realm,    m_phase, 1);
  m_amr->allocate(m_fluid_scratchD,    m_fluid_realm,    m_phase, SpaceDim);
  
  m_amr->allocate(m_particle_scratch1,  m_particle_realm, m_phase, 1);
  m_amr->allocate(m_particle_scratchD,  m_particle_realm, m_phase, SpaceDim);
  m_amr->allocate(m_particle_E, m_particle_realm, m_phase, SpaceDim);

  m_amr->allocate(m_J,            m_fluid_realm, m_phase, SpaceDim);
  m_amr->allocate(m_scratch1,     m_fluid_realm, m_phase, 1);
  m_amr->allocate(m_scratch2,     m_fluid_realm, m_phase, 1);
  m_amr->allocate(m_conduct_cell, m_fluid_realm, m_phase, 1);
  m_amr->allocate(m_conduct_face, m_fluid_realm, m_phase, 1);
  m_amr->allocate(m_conduct_eb,   m_fluid_realm, m_phase, 1);
  m_amr->allocate(m_fluid_E,      m_fluid_realm, m_phase, SpaceDim);

  // Allocate for energy sources
  const int num_ito_species = m_physics->get_num_ito_species();
  m_energy_sources.resize(num_ito_species);
  for (int i = 0; i < m_energy_sources.size(); i++){
    m_amr->allocate(m_energy_sources[i],  m_particle_realm, m_phase, 1);
  }
}

Real ito_plasma_godunov::advance(const Real a_dt) {
  CH_TIME("ito_plasma_godunov::advance");
  if(m_verbosity > 5){
    pout() << m_name + "::advance" << endl;
  }


  Real particle_time = 0.0;
  Real relax_time    = 0.0;
  Real photon_time   = 0.0;
  Real sort_time     = 0.0;
  Real super_time    = 0.0;
  Real reaction_time = 0.0;
  Real clear_time    = 0.0;
  Real velo_time     = 0.0;
  Real diff_time     = 0.0;

  Real total_time    = -MPI_Wtime();
  
  // Particle algorithms
  particle_time = -MPI_Wtime();
  switch(m_algorithm){
  case which_algorithm::euler:
    this->advance_particles_euler(a_dt);
    break;
  case which_algorithm::euler_maruyama:
    this->advance_particles_euler_maruyama(a_dt);
    break;
  case which_algorithm::midpoint:
    this->advance_particles_midpoint(a_dt);
    break;
  default:
    MayDay::Abort("ito_plasma_godunov::advance - logic bust");
  }
  particle_time += MPI_Wtime();

  // Compute current and relaxation time.
  relax_time = -MPI_Wtime();
  this->compute_J(m_J, a_dt);
  m_dt_relax = this->compute_relaxation_time(); // This is for the restricting the next step.
  relax_time += MPI_Wtime();

  // Move photons
  photon_time = -MPI_Wtime();
  this->advance_photons(a_dt);
  photon_time += MPI_Wtime();

  // If we are using the LEA, we must compute the Ohmic heating term. This must be done
  // BEFORE sorting the particles per cell. 
  if(m_physics->get_coupling() == ito_plasma_physics::coupling::LEA){
    this->compute_EdotJ_source();
  }
  
  // Sort the particles and photons per cell so we can call reaction algorithms
  sort_time = -MPI_Wtime();
  m_ito->sort_particles_by_cell();
  this->sort_bulk_photons_by_cell();
  this->sort_source_photons_by_cell();
  sort_time += MPI_Wtime();

  // Chemistry kernel.
  reaction_time = -MPI_Wtime();
  this->advance_reaction_network(a_dt);
  reaction_time += MPI_Wtime();

  // Make superparticles
  super_time = -MPI_Wtime();
  if((m_step+1) % m_merge_interval == 0 && m_merge_interval > 0){
    m_ito->make_superparticles(m_ppc);
  }
  super_time += MPI_Wtime();

  // Sort particles per patch.
  sort_time -= MPI_Wtime();
  m_ito->sort_particles_by_patch();
  this->sort_bulk_photons_by_patch();
  this->sort_source_photons_by_patch();
  sort_time += MPI_Wtime();

  // Clear other data holders for now. BC comes later...
  clear_time = -MPI_Wtime();
  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    solver_it()->clear(solver_it()->get_eb_particles());
    solver_it()->clear(solver_it()->get_domain_particles());
  }
  clear_time += MPI_Wtime();

  // 
  m_ito->deposit_particles();

  // Prepare next step
  velo_time = -MPI_Wtime();
  this->compute_ito_velocities();
  velo_time += MPI_Wtime();
  diff_time -= MPI_Wtime();
  this->compute_ito_diffusion();
  diff_time += MPI_Wtime();

  total_time += MPI_Wtime();

  // Convert to %
  particle_time *= 100./total_time;
  relax_time    *= 100./total_time;
  photon_time   *= 100./total_time;
  sort_time     *= 100./total_time;
  super_time    *= 100./total_time;
  reaction_time *= 100./total_time;
  clear_time    *= 100./total_time;
  velo_time     *= 100./total_time;
  diff_time     *= 100./total_time;

  if(m_profile){
    pout() << endl
      	   << "ito_plasma_godunov::advance breakdown:" << endl
	   << "======================================" << endl
	   << "particle time = " << particle_time << "%" << endl
	   << "relax time    = " << relax_time << "%" << endl
	   << "photon time   = " << photon_time << "%" << endl
	   << "sort time     = " << sort_time << "%" << endl
	   << "super time    = " << super_time << "%" << endl
	   << "reaction time = " << reaction_time << "%" << endl
	   << "clear time    = " << clear_time << "%" << endl
	   << "velo time     = " << velo_time << "%" << endl
      	   << "diff time     = " << diff_time << "%" << endl
	   << "total time    = " << total_time << " (seconds)" << endl
	   << endl;
  }
  
  return a_dt;
}

void ito_plasma_godunov::compute_dt(Real& a_dt, time_code& a_timecode){
  CH_TIME("ito_plasma_godunov::compute_dt");
  if(m_verbosity > 5){
    pout() << "ito_plasma_godunov::compute_dt" << endl;
  }

  a_dt = 1.E99;
  
  if(m_whichDt == which_dt::advection){
    a_dt = m_ito->compute_advective_dt();
  }
  else if(m_whichDt == which_dt::diffusion){
    a_dt = m_ito->compute_diffusive_dt();
  }
  else if(m_whichDt == which_dt::advection_diffusion){
    a_dt = m_ito->compute_dt();
  }
    
  a_dt = a_dt*m_max_cells_hop;
  a_timecode = time_code::cfl;

  // Euler needs to limit by relaxation time
  if(m_algorithm == which_algorithm::euler){
    const Real dtRelax = m_relax_factor*m_dt_relax;
    if(dtRelax < a_dt){
      a_dt = dtRelax;
      a_timecode = time_code::relaxation_time;
    }
  }

  // Physics-based restriction
  const Real physicsDt = this->compute_physics_dt();
  if(physicsDt < a_dt){
    a_dt = physicsDt;
    a_timecode = time_code::physics;
  }

  if(a_dt < m_min_dt){
    a_dt = m_min_dt;
    a_timecode = time_code::hardcap;
  }

  if(a_dt > m_max_dt){
    a_dt = m_max_dt;
    a_timecode = time_code::hardcap;
  }

  m_timecode = a_timecode;

#if 0 // Debug code
  const Real dtCFL = m_ito->compute_dt();
  m_avg_cfl += a_dt/dtCFL;
  if(procID() == 0) std::cout << "dt = " << a_dt
			      << "\t relax dt = " << m_dt_relax
			      << "\t factor = " << a_dt/m_dt_relax
			      << "\t CFL = " << a_dt/dtCFL
			      << "\t avgCFL = " << m_avg_cfl/(1+m_step)
			      << std::endl;
#endif
}

void ito_plasma_godunov::pre_regrid(const int a_lmin, const int a_old_finest_level){
  CH_TIME("ito_plasma_godunov::pre_regrid");
  if(m_verbosity > 5){
    pout() << "ito_plasma_godunov::pre_regrid" << endl;
  }

  ito_plasma_stepper::pre_regrid(a_lmin, a_old_finest_level);

  if(m_algorithm != which_algorithm::euler){

    // Copy conductivity to scratch storage
    const int ncomp        = 1;
    const int finest_level = m_amr->get_finest_level();
    m_amr->allocate(m_cache,  m_fluid_realm, m_phase, ncomp);
    for (int lvl = 0; lvl <= a_old_finest_level; lvl++){
      m_conduct_cell[lvl]->localCopyTo(*m_cache[lvl]);
    }

    for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
      const int idx = solver_it.get_solver();
      
      m_conductivity_particles[idx]->pre_regrid(a_lmin);
      m_rho_dagger_particles[idx]->pre_regrid(a_lmin);
    }
  }
}

void ito_plasma_godunov::regrid(const int a_lmin, const int a_old_finest_level, const int a_new_finest_level) {
  CH_TIME("ito_plasma_godunov::regrid");
  if(m_verbosity > 5){
    pout() << "ito_plasma_godunov::regrid" << endl;
  }

  if(m_algorithm == which_algorithm::euler){
    ito_plasma_stepper::regrid(a_lmin, a_old_finest_level, a_new_finest_level);
  }
  else{
    this->regrid_si(a_lmin, a_old_finest_level, a_new_finest_level);
  }
}

void ito_plasma_godunov::setup_runtime_storage(){
    CH_TIME("ito_plasma_godunov::setup_runtime_storage");
  if(m_verbosity > 5){
    pout() << m_name + "::setup_runtime_storage" << endl;
  }

  switch (m_algorithm){
  case which_algorithm::midpoint:
    ito_particle::set_num_runtime_vectors(1);
    break;
  default:
    ito_particle::set_num_runtime_vectors(0);
  }
}

void ito_plasma_godunov::regrid_si(const int a_lmin, const int a_old_finest_level, const int a_new_finest_level) {
  CH_TIME("ito_plasma_godunov::regrid_si");
  if(m_verbosity > 5){
    pout() << "ito_plasma_godunov::regrid_si" << endl;
  }

  // Regrid solvers
  m_ito->regrid(a_lmin,     a_old_finest_level, a_new_finest_level);
  m_poisson->regrid(a_lmin, a_old_finest_level, a_new_finest_level);
  m_rte->regrid(a_lmin,     a_old_finest_level, a_new_finest_level);
  m_sigma->regrid(a_lmin,   a_old_finest_level, a_new_finest_level);

  // Allocate internal memory for ito_plasma_godunov now....
  this->allocate_internals();

  // We need to remap/regrid the stored particles as well. 
  const Vector<DisjointBoxLayout>& grids = m_amr->get_grids(m_particle_realm);
  const Vector<ProblemDomain>& domains   = m_amr->get_domains();
  const Vector<Real>& dx                 = m_amr->get_dx();
  const Vector<int>& ref_rat             = m_amr->get_ref_rat();

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    const int idx = solver_it.get_solver();
    m_conductivity_particles[idx]->regrid(grids, domains, dx, ref_rat, a_lmin, a_new_finest_level);
    m_rho_dagger_particles[idx]->regrid(grids, domains, dx, ref_rat, a_lmin, a_new_finest_level);
  }


  // Recompute the conductivity with the other particles
  this->compute_regrid_conductivity();

  // Set up semi-implicit poisson again, with particles after diffusion jump (the rho^\dagger)
  this->setup_semi_implicit_poisson(m_prevDt);

  // Recompute the space charge. This deposits the m_rho_dagger particles onto the states. 
  this->compute_regrid_rho();
  
  // Compute the electric field
  const bool converged = this->solve_poisson();
  if(!converged){
    MayDay::Abort("ito_plasma_stepper::regrid - Poisson solve did not converge after regrid!!!");
  }

  // Regrid superparticles. 
  if(m_regrid_superparticles){
    m_ito->sort_particles_by_cell();
    m_ito->make_superparticles(m_ppc);
    m_ito->sort_particles_by_patch();
  }

  // Now let the ito solver deposit its actual particles...
  m_ito->deposit_particles();

  // Recompute new velocities and diffusion coefficients
  this->compute_ito_velocities();
  this->compute_ito_diffusion();
}

void ito_plasma_godunov::set_old_positions(){
  CH_TIME("ito_plasma_godunov::set_old_positions()");
  if(m_verbosity > 5){
    pout() << m_name + "::set_old_positions()" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    
    if(solver->is_diffusive()){
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();


	  for (ListIterator<ito_particle> lit(particleList); lit.ok(); ++lit){
	    ito_particle& p = particleList[lit];
	    p.oldPosition() = p.position();
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::rewind_particles(){
  CH_TIME("ito_plasma_godunov::rewind_particles");
  if(m_verbosity > 5){
    pout() << m_name + "::rewind_particles" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    
    if(solver->is_diffusive()){
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  // Diffusion hop.
	  for (lit.rewind(); lit; ++lit){
	    ito_particle& p = particleList[lit];
	    p.position() = p.oldPosition();
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::intersect_particles(const Real a_dt){
  CH_TIME("ito_plasma_godunov::intersect_particles");
  if(m_verbosity > 5){
    pout() << m_name + "::intersect_particles" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();

    const bool mobile    = solver->is_mobile();
    const bool diffusive = solver->is_diffusive();

    if(mobile || diffusive){
      solver->intersect_particles();
    }
  }
}

void ito_plasma_godunov::compute_conductivity(){
  CH_TIME("ito_plasma_godunov::compute_conductivity");
  if(m_verbosity > 5){
    pout() << m_name + "::compute_conductivity" << endl;
  }

  ito_plasma_stepper::compute_conductivity(m_conduct_cell);

  // Now do the faces
  this->compute_face_conductivity();
}

void ito_plasma_godunov::compute_face_conductivity(){
  CH_TIME("ito_plasma_godunov::compute_face_conductivity");
  if(m_verbosity > 5){
    pout() << m_name + "::compute_face_conductivity" << endl;
  }

  // This code does averaging from cell to face. 
  data_ops::average_cell_to_face_allcomps(m_conduct_face, m_conduct_cell, m_amr->get_domains());

  // This code extrapolates the conductivity to the EB. This should actually be the EB centroid but since the stencils
  // for EB extrapolation can be a bit nasty (e.g. negative weights), we do the centroid instead and take that as an approximation.
#if 0
  const irreg_amr_stencil<centroid_interp>& ebsten = m_amr->get_centroid_interp_stencils(m_fluid_realm, m_phase);
  for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
    ebsten.apply(m_conduct_eb, m_conduct_cell, lvl);
  }
#else
  data_ops::set_value(m_conduct_eb, 0.0);
  data_ops::incr(m_conduct_eb, m_conduct_cell, 1.0);
#endif

}

void ito_plasma_godunov::setup_semi_implicit_poisson(const Real a_dt){
  CH_TIME("ito_plasma_godunov::setup_semi_implicit_poisson");
  if(m_verbosity > 5){
    pout() << m_name + "::setup_semi_implicit_poisson" << endl;
  }

  poisson_multifluid_gmg* poisson = (poisson_multifluid_gmg*) (&(*m_poisson));

  // Set coefficients as usual
  poisson->set_coefficients();

  // Get bco and increment with mobilities
  MFAMRFluxData& bco   = poisson->get_bco();
  MFAMRIVData& bco_irr = poisson->get_bco_irreg();
  
  EBAMRFluxData bco_gas;
  EBAMRIVData   bco_irr_gas;
  
  m_amr->allocate_ptr(bco_gas);
  m_amr->allocate_ptr(bco_irr_gas);
  
  m_amr->alias(bco_gas,     phase::gas, bco);
  m_amr->alias(bco_irr_gas, phase::gas, bco_irr);

  data_ops::scale(m_conduct_face, a_dt/units::s_eps0);
  data_ops::scale(m_conduct_eb,   a_dt/units::s_eps0);

  data_ops::multiply(m_conduct_face, bco_gas);
  data_ops::multiply(m_conduct_eb,   bco_irr_gas);

  data_ops::incr(bco_gas,     m_conduct_face, 1.0);
  data_ops::incr(bco_irr_gas, m_conduct_eb,   1.0);

  // Set up the multigrid solver
  poisson->setup_operator_factory();
  poisson->setup_solver();
  poisson->set_needs_setup(false);
}

void ito_plasma_godunov::setup_standard_poisson(){
  CH_TIME("ito_plasma_godunov::setup_standard_poisson");
  if(m_verbosity > 5){
    pout() << m_name + "::setup_standard_poisson" << endl;
  }

  poisson_multifluid_gmg* poisson = (poisson_multifluid_gmg*) (&(*m_poisson));

  // Set coefficients as usual
  poisson->set_coefficients();

  // Set up the multigrid solver
  poisson->setup_operator_factory();
  poisson->setup_solver();
  poisson->set_needs_setup(false);
}

void ito_plasma_godunov::copy_conductivity_particles(){
  CH_TIME("ito_plasma_godunov::copy_conductivity_particles");
  if(m_verbosity > 5){
    pout() << m_name + "::copy_conductivity_particles" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    const RefCountedPtr<ito_solver>& solver   = solver_it();
    const RefCountedPtr<ito_species>& species = solver->get_species();

    const int idx = solver_it.get_solver();
    const int q   = species->get_charge();

    for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
      const DisjointBoxLayout& dbl = m_amr->get_grids(m_particle_realm)[lvl];

      for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){
	const List<ito_particle>& ito_parts = solver->get_particles()[lvl][dit()].listItems();
	List<godunov_particle>& gdnv_parts  = (*m_conductivity_particles[idx])[lvl][dit()].listItems();

	gdnv_parts.clear();

	if(q != 0 && solver->is_mobile()){
	  for (ListIterator<ito_particle> lit(ito_parts); lit.ok(); ++lit){
	    const ito_particle& p = lit();
	    const RealVect& pos   = p.position();
	    const Real& mass      = p.mass();
	    const Real& mobility  = p.mobility();

	    gdnv_parts.add(godunov_particle(pos, mass*mobility));
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::copy_rho_dagger_particles(){
  CH_TIME("ito_plasma_godunov::copy_rho_dagger_particles");
  if(m_verbosity > 5){
    pout() << m_name + "::copy_rho_dagger_particles" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    const RefCountedPtr<ito_solver>& solver   = solver_it();
    const RefCountedPtr<ito_species>& species = solver->get_species();

    const int idx = solver_it.get_solver();
    const int q   = species->get_charge();

    for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
      const DisjointBoxLayout& dbl = m_amr->get_grids(m_particle_realm)[lvl];

      for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){
	const List<ito_particle>& ito_parts = solver->get_particles()[lvl][dit()].listItems();
	List<godunov_particle>& gdnv_parts  = (*m_rho_dagger_particles[idx])[lvl][dit()].listItems();

	gdnv_parts.clear();

	if(q != 0){
	  for (ListIterator<ito_particle> lit(ito_parts); lit.ok(); ++lit){
	    const ito_particle& p = lit();
	    const RealVect& pos   = p.position();
	    const Real& mass      = p.mass();

	    gdnv_parts.add(godunov_particle(pos, mass));
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::compute_regrid_conductivity(){
  CH_TIME("ito_plasma_godunov::compute_regrid_conductivity");
  if(m_verbosity > 5){
    pout() << m_name + "::compute_regrid_conductivity" << endl;
  }

  data_ops::set_value(m_conduct_cell, 0.0);

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>&   solver = solver_it();
    RefCountedPtr<ito_species>& species = solver->get_species();
    
    const int idx = solver_it.get_solver();
    const int q   = species->get_charge();

    if(q != 0 && solver->is_mobile()){
      data_ops::set_value(m_particle_scratch1, 0.0);

      solver->deposit_particles(m_particle_scratch1, *m_conductivity_particles[idx]); // This deposit mu*mass

      // Copy to fluid realm and add to fluid stuff
      m_fluid_scratch1.copy(m_particle_scratch1);
      data_ops::incr(m_conduct_cell, m_fluid_scratch1, Abs(q));
    }
  }
  
  data_ops::scale(m_conduct_cell, units::s_Qe);

  m_amr->average_down(m_conduct_cell, m_fluid_realm, m_phase);
  m_amr->interp_ghost_pwl(m_conduct_cell, m_fluid_realm, m_phase);
  m_amr->interpolate_to_centroids(m_conduct_cell, m_fluid_realm, m_phase);

  // Now do the faces
  this->compute_face_conductivity();
}

void ito_plasma_godunov::compute_regrid_rho(){
  CH_TIME("ito_plasma_godunov::compute_regrid_rho");
  if(m_verbosity > 5){
    pout() << m_name + "::compute_regrid_rho" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>&   solver = solver_it();
    RefCountedPtr<ito_species>& species = solver->get_species();

    const int idx = solver_it.get_solver();
    
    solver->deposit_particles(solver->get_state(), *m_rho_dagger_particles[idx]); 
  }
}

void ito_plasma_godunov::advance_particles_euler(const Real a_dt){
  CH_TIME("ito_plasma_godunov::advance_particles_euler");
  if(m_verbosity > 5){
    pout() << m_name + "::advance_particles_euler" << endl;
  }

  this->set_old_positions();
  this->advect_particles_euler(a_dt);
  this->diffuse_particles_euler(a_dt);
  
  this->intersect_particles(a_dt);
  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    solver_it()->remove_eb_particles();
  }

  m_ito->remap();
  m_ito->deposit_particles();

  this->solve_poisson();
}

void ito_plasma_godunov::advect_particles_euler(const Real a_dt){
  CH_TIME("ito_plasma_godunov::advect_particles_euler");
  if(m_verbosity > 5){
    pout() << m_name + "::advect_particles_euler" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    if(solver->is_mobile()){
      
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  // First step
	  for (lit.rewind(); lit; ++lit){
	    ito_particle& p = particleList[lit];

	    // Update positions. 
	    p.position() += p.velocity()*a_dt;
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::diffuse_particles_euler(const Real a_dt){
  CH_TIME("ito_plasma_godunov::diffuse_particles_euler");
  if(m_verbosity > 5){
    pout() << m_name + "::diffuse_particles_euler" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    
    if(solver->is_diffusive()){
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  // Diffusion hop.
	  for (lit.rewind(); lit.ok(); ++lit){
	    ito_particle& p    = particleList[lit];
	    const Real factor  = sqrt(2.0*p.diffusion()*a_dt);

	    // Do the diffusion hop. 
	    const RealVect hop = solver->random_gaussian();

	    p.position() += hop*factor;
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::advance_particles_euler_maruyama(const Real a_dt){
  CH_TIME("ito_plasma_godunov::advance_particles_euler_maruyama");
  if(m_verbosity > 5){
    pout() << m_name + "::advance_particles_euler_maruyama" << endl;
  }

  // Set the old particle position. 
  this->set_old_positions();

  // Need to copy the current particles because they will be used for computing the conductivity during regrids
  this->copy_conductivity_particles();

  // Compute conductivity and setup poisson
  this->compute_conductivity();
  this->setup_semi_implicit_poisson(a_dt);

  // Diffuse the particles now
  this->diffuse_particles_euler(a_dt);

  // Remap and deposit, only need to do this for diffusive solvers. 
  this->remap_diffusive_particles(); 
  //  m_ito->deposit_particles();
  this->deposit_diffusive_particles();


  // Need to copy the current particles because they will be used for the space charge during regrids. 
  this->copy_rho_dagger_particles();

  // Now compute the electric field
  this->solve_poisson();

  // We have field at k+1 but particles have been diffused. The ones that are diffusive AND mobile are put back to X^k positions
  // and then we compute velocities with E^(k+1). 
  this->swap_euler_maruyama();       // After this, oldPosition() holds X^\dagger, and position() holds X^k. 
  this->remap_diffusive_particles(); // Only need to do this for the ones that were diffusive

  // Recompute velocities with the new electric field
#if 0 // original code
  this->compute_ito_velocities();
#else // This is the version that leaves the mobility intact - which is what the algorithm says. 
  this->set_ito_velocity_funcs();
  m_ito->interpolate_velocities();
#endif
  this->advect_particles_euler_maruyama(a_dt);  
  
  // Remap, redeposit, store invalid particles, and intersect particles. Deposition is for relaxation time computation.
  this->remap_mobile_or_diffusive_particles();

  // Do intersection test and remove EB particles. These particles are NOT allowed to react later.
  this->intersect_particles(a_dt);
  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    solver_it()->remove_eb_particles();
  }

  // Deposit particles. This shouldn't be necessary unless we want to compute (E,J)
  this->deposit_mobile_or_diffusive_particles();
}

void ito_plasma_godunov::advect_particles_euler_maruyama(const Real a_dt){
  CH_TIME("ito_plasma_godunov::advect_particles_si");
  if(m_verbosity > 5){
    pout() << m_name + "::advect_particles_si" << endl;
  }



  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    if(solver->is_mobile()){
      
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  // First step
	  for (lit.rewind(); lit; ++lit){
	    ito_particle& p = particleList[lit];

	    // Add diffusion hop again. The position after the diffusion hop is oldPosition() and X^k is in position()
	    const RealVect Xk = p.position();
	    p.position()      = p.oldPosition() + p.velocity()*a_dt;
	    p.oldPosition()   = Xk;
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::diffuse_particles_euler_maruyama(const Real a_dt){
  CH_TIME("ito_plasma_godunov::diffuse_particles_euler_maruyama");
  if(m_verbosity > 5){
    pout() << m_name + "::diffuse_particles_euler_maruyama" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    
    if(solver->is_diffusive()){
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  // Diffusion hop.
	  for (lit.rewind(); lit.ok(); ++lit){
	    ito_particle& p    = particleList[lit];
	    const Real factor  = sqrt(2.0*p.diffusion()*a_dt);

	    // Do the diffusion hop. 
	    const RealVect hop = solver->random_gaussian();

	    p.position() += hop*factor;
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::swap_euler_maruyama(){
  CH_TIME("ito_plasma_godunov::swap_euler_maruyama");
  if(m_verbosity > 5){
    pout() << m_name + "::swap_euler_maruyama" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    const bool mobile    = solver->is_mobile();
    const bool diffusive = solver->is_diffusive();

    // No need to do this if solver is only mobile because the diffusion step didn't change the position.
    // Likewise, if the solver is only diffusive then advect_particles_si routine won't trigger so no need for that either. 
    if(mobile && diffusive){ 
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  for (lit.rewind(); lit; ++lit){
	    ito_particle& p = particleList[lit];

	    // We have made a diffusion hop, but we need p.position() to be X^k and p.oldPosition() to be the jumped position. 
	    const RealVect tmp = p.position();
	    
	    p.position()    = p.oldPosition();
	    p.oldPosition() = tmp;
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::advance_particles_midpoint(const Real a_dt){
  CH_TIME("ito_plasma_godunov::advance_particles_midpoint");
  if(m_verbosity > 5){
    pout() << m_name + "::advance_particles_midpoint" << endl;
  }
  // Set the old particle position. 
  this->set_old_positions();

  // Compute conductivity and setup poisson
  this->compute_conductivity();
  this->setup_semi_implicit_poisson(0.5*a_dt);

  // X^(k+1/2) = X^k + sqrt(D^k*dt)*N
  this->diffuse_particles_midpoint1(0.5*a_dt); // Only diffusive particles move

  // Remap and deposit diffusive particles
  this->remap_diffusive_particles(); 
  this->deposit_diffusive_particles();

  // Now compute the electric field => E^(k+1/2)
  this->solve_poisson();

  // We have field at k+1 but particles have been diffused. The ones that are diffusive AND mobile are put back to X^k positions
  // and then we compute velocities with E^(k+1). 
  this->swap_midpoint1();            // After this, oldPosition() holds X^\dagger, and position() holds X^k. 
  this->remap_diffusive_particles(); // Only need to do this for the ones that were diffusive. This reverts p.position() to X^k, while p.oldPosition() is X^k+1/2

  // Recompute velocities with the new electric field. Then advect the particles. 
  this->set_ito_velocity_funcs();
  m_ito->interpolate_velocities();
  this->advect_particles_midpoint1(0.5*a_dt); // After this, X^(k+1/2) = p.position(), X^k = p.oldPosition(), sqrt(D*dt)*N = p.runtime_vector(0);

  // =============== SECOND MIDPOINT STAGE ==================== //
  this->copy_conductivity_particles();     // Copy particles used for conductivity computation
  
  this->compute_conductivity();            // Computes sigma = sigma(X^k+1/2)
  this->setup_semi_implicit_poisson(a_dt); // Full time step this time. 

  // Interpolate diffusion coefficients for all the particles
  this->compute_ito_diffusion();                // Update D = D^(k+1/2)(X^(k+1/2))
  this->diffuse_particles_midpoint2(0.5*a_dt);   // Diffuse over another step. Sets p.position() = X^k + sqrt(D^k*dt)*N1 + sqrt(D^k*dt)*N2, p.oldPosition() = 
  this->remap_diffusive_particles();
  this->deposit_diffusive_particles();

  // Regrids require the particles used for computing the (semi-implicit) space charge
  this->copy_rho_dagger_particles(); 

  // Solve for E^(k+1)
  this->solve_poisson();

  // Do the complete second step. 
  this->swap_midpoint2();                 // p.position() = X^(k+1/2). p.oldPosition() = X^k. p.runtime_vector(0) = sqrt(D^k*dt)*N^1 + sqrt(D^(k+1/2)*dt*N^2
  this->compute_ito_velocities();         // Update V = v^(k+1)(X^(k+1/2))
  this->advect_particles_midpoint2(a_dt); // p.position() = p.oldPosition() + dt*V^(k+1)(X^(k+1/2)) + sqrt(D^k*dt)*N^1 + sqrt(D^(k+1/2)*dt*N^2
  
  // Remap, redeposit, store invalid particles, and intersect particles. Deposition is for relaxation time computation.
  this->remap_mobile_or_diffusive_particles();

  // Do intersection test and remove EB particles. These particles are NOT allowed to react later.
  this->intersect_particles(a_dt);
  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    solver_it()->remove_eb_particles();
  }

  // Deposit particles. This shouldn't be necessary unless we want to compute (E,J)
  this->deposit_mobile_or_diffusive_particles();
}

void ito_plasma_godunov::diffuse_particles_midpoint1(const Real a_dt){
  CH_TIME("ito_plasma_godunov::diffuse_particles_midpoint1");
  if(m_verbosity > 5){
    pout() << m_name + "::diffuse_particles_midpoint1" << endl;
  }

  
  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    
    if(solver->is_diffusive()){
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  // Diffusion hop.
	  for (lit.rewind(); lit.ok(); ++lit){
	    ito_particle& p    = particleList[lit];
	    const Real factor  = sqrt(2.0*p.diffusion()*a_dt);

	    // Do the hop, and storage the hop. 
	    RealVect& hop = p.runtime_vector(0);
	    hop = factor*solver->random_gaussian();

	    p.position() += hop;
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::diffuse_particles_midpoint2(const Real a_dt){
  CH_TIME("ito_plasma_godunov::diffuse_particles_midpoint2");
  if(m_verbosity > 5){
    pout() << m_name + "::diffuse_particles_midpoint2" << endl;
  }

  
  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    
    if(solver->is_diffusive()){
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  // Diffusion hop.
	  for (lit.rewind(); lit.ok(); ++lit){
	    ito_particle& p    = particleList[lit];

	    // Store X^(k+1/2) on p.velocity() since that storage is not currently needed
	    p.velocity() = p.position();

	    // Increment the hop and move the friggin particle
	    const Real factor  = sqrt(2.0*p.diffusion()*a_dt);
	    RealVect& hop = p.runtime_vector(0);
	    hop += factor*solver->random_gaussian();

	    p.position() = p.oldPosition() + hop;
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::advect_particles_midpoint1(const Real a_dt){
  CH_TIME("ito_plasma_godunov::advect_particles_midpoint1");
  if(m_verbosity > 5){
    pout() << m_name + "::advect_particles_midpoint1" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    if(solver->is_mobile()){
      
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  // First step
	  for (lit.rewind(); lit; ++lit){
	    ito_particle& p = particleList[lit];

	    // Add diffusion hop again. The position after the diffusion hop is oldPosition() and X^k is in position()
	    const RealVect Xk  = p.position();
	    const RealVect hop = p.runtime_vector(0);
	    p.oldPosition()    = Xk;
	    p.position()       = Xk + p.velocity()*a_dt + hop;
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::advect_particles_midpoint2(const Real a_dt){
  CH_TIME("ito_plasma_godunov::advect_particles_midpoint1");
  if(m_verbosity > 5){
    pout() << m_name + "::advect_particles_midpoint1" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    if(solver->is_mobile()){
      
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  // First step
	  for (lit.rewind(); lit; ++lit){
	    ito_particle& p = particleList[lit];

	    // Add diffusion hop again. The position after the diffusion hop is oldPosition() and X^k is in position()
	    const RealVect& Xk  = p.oldPosition();
	    const RealVect& hop = p.runtime_vector(0);
	    const RealVect& v   = p.velocity();
	    p.position()        = Xk + v*a_dt + hop;
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::swap_midpoint1(){
  CH_TIME("ito_plasma_godunov::swap_midpoint1");
  if(m_verbosity > 5){
    pout() << m_name + "::swap_midpoint1" << endl;
  }

  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    const bool mobile    = solver->is_mobile();
    const bool diffusive = solver->is_diffusive();

    // No need to do this if solver is only mobile because the diffusion step didn't change the position.
    // Likewise, if the solver is only diffusive then advect_particles_si routine won't trigger so no need for that either. 
    if(mobile && diffusive){ 
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  for (lit.rewind(); lit; ++lit){
	    ito_particle& p = particleList[lit];

	    // We have made a diffusion hop, but we need p.position() to be X^k and p.oldPosition() to be the jumped position. 
	    const RealVect tmp = p.position();
	    
	    p.position()    = p.oldPosition();
	    p.oldPosition() = tmp;
	  }
	}
      }
    }
  }
}

void ito_plasma_godunov::swap_midpoint2(){
  CH_TIME("ito_plasma_godunov::swap_midpoint1");
  if(m_verbosity > 5){
    pout() << m_name + "::swap_midpoint1" << endl;
  }

  // TLDR: When we enter here then p.velocity() = X^(k+1/2)
  //                               p.oldPosition() = X^k
  //                               p.position()    = X^k + sqrt(D*dt)*N1 + sqrt(D*dt)*N2
  //       p.position() can be discarded, so we set p.position() to X^(k+1/2)


  for (auto solver_it = m_ito->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<ito_solver>& solver = solver_it();
    const bool mobile    = solver->is_mobile();
    const bool diffusive = solver->is_diffusive();

    // No need to do this if solver is only mobile because the diffusion step didn't change the position.
    // Likewise, if the solver is only diffusive then advect_particles_si routine won't trigger so no need for that either. 
    if(diffusive){ 
      for (int lvl = 0; lvl <= m_amr->get_finest_level(); lvl++){
	const DisjointBoxLayout& dbl          = m_amr->get_grids(m_particle_realm)[lvl];
	ParticleData<ito_particle>& particles = solver->get_particles()[lvl];

	for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){

	  List<ito_particle>& particleList = particles[dit()].listItems();
	  ListIterator<ito_particle> lit(particleList);

	  for (lit.rewind(); lit; ++lit){
	    ito_particle& p = particleList[lit];

	    // We have made a diffusion hop, but we need p.position() to be X^k and p.oldPosition() to be the jumped position. 
	    p.position() = p.velocity(); // = X^(k+1/2)
	  }
	}
      }
    }
  }
}
