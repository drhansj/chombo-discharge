/*!
  @file time_stepper.cpp
  @brief Implementation of time_stepper.H
  @author Robert Marskar
  @date Nov. 2017
*/

#include "time_stepper.H"
#include "poisson_multifluid_gmg.H"
#include "cdr_iterator.H"
#include "rte_iterator.H"
#include "units.H"

time_stepper::time_stepper(){
  this->set_verbosity(10);

}

time_stepper::~time_stepper(){
}

int time_stepper::query_ghost(){
  return 3;
}

bool time_stepper::stationary_rte(){
  CH_TIME("time_stepper::stationary_rte");
  if(m_verbosity > 5){
    pout() << "time_stepper::stationary_rte" << endl;
  }

  return m_rte->is_stationary();
}

void time_stepper::compute_E(EBAMRCellData& a_E, const phase::which_phase a_phase, const MFAMRCellData& a_potential){
  CH_TIME("time_stepper::compute_E(ebamrcell, phase, mfamrcell)");
  if(m_verbosity > 5){
    pout() << "time_stepper::compute_E(ebamrcell, phase, mfamrcell)" << endl;
  }

  EBAMRCellData pot_gas;
  m_amr->allocate_ptr(pot_gas);
  m_amr->alias(pot_gas, a_phase, a_potential);

  m_amr->compute_gradient(a_E, pot_gas);
  data_ops::scale(a_E, -1.0);

  m_amr->average_down(a_E, a_phase);
  m_amr->interp_ghost(a_E, a_phase);
}


void time_stepper::compute_E(MFAMRCellData& a_E, const MFAMRCellData& a_potential){
  CH_TIME("time_stepper::compute_E(mfamrcell, mfamrcell)");
  if(m_verbosity > 5){
    pout() << "time_stepper::compute_E(mfamrcell, mfamrcell)" << endl;
  }

  m_amr->compute_gradient(a_E, a_potential);
  data_ops::scale(a_E, -1.0);

  m_amr->average_down(a_E);
  m_amr->interp_ghost(a_E);
}

void time_stepper::compute_photon_source_terms(Vector<EBAMRCellData*>        a_source,
					       const Vector<EBAMRCellData*>& a_cdr_states,
					       const EBAMRCellData&          a_E,
					       const centering::which_center a_centering){
  CH_TIME("time_stepper::compute_photon_source_terms(full)");
  if(m_verbosity > 5){
    pout() << "time_stepper::compute_photon_source_terms(full)" << endl;
  }

  const phase::which_phase rte_phase = m_rte->get_phase();
  const int comp         = 0;
  const int ncomp        = 1;
  const int finest_level = m_amr->get_finest_level();

  const irreg_amr_stencil<centroid_interp>& interp_stencils = m_amr->get_centroid_interp_stencils(rte_phase);

  Vector<Real> cdr_densities(1 + m_plaskin->get_num_species());

  for (int lvl = 0; lvl <= finest_level; lvl++){
    const DisjointBoxLayout& dbl  = m_amr->get_grids()[lvl];
    const EBISLayout& ebisl       = m_amr->get_ebisl(rte_phase)[lvl];
    const Real dx                 = m_amr->get_dx()[lvl];
    const irreg_stencil& stencils = interp_stencils[lvl];
    
    for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){
      const Box box          = dbl.get(dit());
      const EBISBox& ebisbox = ebisl[dit()];
      const EBGraph& ebgraph = ebisbox.getEBGraph();
      const EBCellFAB& E     = (*a_E[lvl])[dit()];


      // Do all cells
      IntVectSet ivs(box);
      for (VoFIterator vofit(ivs, ebgraph); vofit.ok(); ++vofit){
	const VolIndex& vof = vofit();

	const RealVect e = RealVect(D_DECL(E(vof, 0), E(vof, 1), E(vof, 2)));
	for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
	  const int idx = solver_it.get_solver();
	  cdr_densities[idx] = (*(*a_cdr_states[idx])[lvl])[dit](vof, comp);
	}

	Vector<Real> sources = m_plaskin->compute_rte_source_terms(cdr_densities, e);
	for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
	  const int idx = solver_it.get_solver();
	  (*(*a_source[idx])[lvl])[dit()](vof, comp) = sources[idx];
	}
      }

      // Do irregular cells
      if(a_centering == centering::cell_center){
	ivs = ebisbox.getIrregIVS(box);
	for (VoFIterator vofit(ivs, ebgraph); vofit.ok(); ++vofit){
	  const VolIndex& vof = vofit();

	  // Reset these and apply stencil
	  RealVect e = RealVect::Zero;
	  cdr_densities.assign(0.0);
	
	  const VoFStencil& stencil = stencils[dit()](vof, comp);
	  for (int i = 0; i < stencil.size(); i++){
	    const VolIndex& ivof = stencil.vof(i);
	    const Real iweight   = stencil.weight(i);

	    for (int dir = 0; dir < SpaceDim; dir++){
	      e[dir] = E(ivof, dir)*iweight;
	    }
	  
	    for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
	      const int idx = solver_it.get_solver();
	      cdr_densities[idx] = +(*(*a_cdr_states[idx])[lvl])[dit](ivof, comp)*iweight;
	    }
	  }

	  Vector<Real> sources = m_plaskin->compute_rte_source_terms(cdr_densities, e);
	  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
	    const int idx = solver_it.get_solver();
	    (*(*a_source[idx])[lvl])[dit()](vof, comp) = sources[idx];
	  }
	}
      }
    }
  }

  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
    const int idx = solver_it.get_solver();
    m_amr->average_down(*a_source[idx], rte_phase);
    m_amr->interp_ghost(*a_source[idx], rte_phase);
  }
}

void time_stepper::compute_rho(){
  CH_TIME("time_stepper::compute_rho()");
  if(m_verbosity > 5){
    pout() << "time_stepper::compute_rho()" << endl;
  }

  Vector<EBAMRCellData*> densities;
  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver> solver = solver_it();
    densities.push_back(&(solver->get_state()));
  }

  this->compute_rho(m_poisson->get_source(),
		    densities,
		    centering::cell_center);
}

void time_stepper::compute_rho(MFAMRCellData&                a_rho,
			       const Vector<EBAMRCellData*>  a_densities,
			       const centering::which_center a_centering){
  CH_TIME("time_stepper::compute_rho(mfamrcell, vec(ebamrcell))");
  if(m_verbosity > 5){
    pout() << "time_stepper::compute_rho(mfamrcell, vec(ebamrcell))" << endl;
  }

  data_ops::set_value(a_rho, 0.0);

  EBAMRCellData rho_gas;
  m_amr->allocate_ptr(rho_gas);
  m_amr->alias(rho_gas, phase::gas, a_rho);

  const int finest_level = m_amr->get_finest_level();
  for (int lvl = 0; lvl <= finest_level; lvl++){
    data_ops::set_value(rho_gas, 0.0);

    // Add volumetric charge 
    for (cdr_iterator solver_it(*m_cdr, a_densities); solver_it.ok(); ++solver_it){
      const EBAMRCellData& density       = solver_it.get_data();
      const RefCountedPtr<species>& spec = solver_it.get_species();

      data_ops::incr(*rho_gas[lvl], *density[lvl], spec->get_charge());
    }

    // Scale by s_Qe/s_eps0
    data_ops::scale(*a_rho[lvl], units::s_Qe);
    data_ops::kappa_scale(*a_rho[lvl]);
  }

  // Transform to centroids
  if(a_centering == centering::cell_center){
    m_amr->interpolate_to_centroids(rho_gas, phase::gas);
  }


}

void time_stepper::instantiate_solvers(){
  CH_TIME("time_stepper::instantiate_solvers");
  if(m_verbosity > 5){
    pout() << "time_stepper::instantiate_solvers" << endl;
  }

  this->sanity_check();

  this->setup_cdr();
  this->setup_rte();
  this->setup_poisson();
  this->setup_sigma();
}

void time_stepper::sanity_check(){
  CH_TIME("time_stepper::sanity_check");
  if(m_verbosity > 5){
    pout() << "time_stepper::sanity_check" << endl;
  }

  CH_assert(!m_compgeom.isNull());
  CH_assert(!m_physdom.isNull());
  CH_assert(!m_amr.isNull());
  CH_assert(!m_plaskin.isNull());
}

void time_stepper::set_amr(const RefCountedPtr<amr_mesh>& a_amr){
  CH_TIME("time_stepper::set_amr");
  if(m_verbosity > 5){
    pout() << "time_stepper::set_amr" << endl;
  }

  m_amr = a_amr;
}

void time_stepper::set_computational_geometry(const RefCountedPtr<computational_geometry>& a_compgeom){
  CH_TIME("time_stepper::set_computational_geometry");
  if(m_verbosity > 5){
    pout() << "time_stepper::set_computational_geometry" << endl;
  }

  m_compgeom = a_compgeom;
}

void time_stepper::set_plasma_kinetics(const RefCountedPtr<plasma_kinetics>& a_plaskin){
  CH_TIME("time_stepper::set_plasma_kinetics");
  if(m_verbosity > 5){
    pout() << "time_stepper::set_plasma_kinetics" << endl;
  }

  m_plaskin = a_plaskin;
}

void time_stepper::set_physical_domain(const RefCountedPtr<physical_domain>& a_physdom){
  CH_TIME("time_stepper::set_physical_domain");
  if(m_verbosity > 5){
    pout() << "time_stepper::set_physical_domain" << endl;
  }

  m_physdom = a_physdom;
}

void time_stepper::set_potential(Real (*a_potential)(const Real a_time)){
  CH_TIME("time_stepper::set_potential");
  if(m_verbosity > 5){
    pout() << "time_stepper::set_potential" << endl;
  }
  m_potential     = a_potential;
}

void time_stepper::set_verbosity(const int a_verbosity){
  CH_TIME("time_stepper::set_verbosity");
  if(m_verbosity > 5){
    pout() << "time_stepper::set_verbosity" << endl;
  }
  
  m_verbosity = a_verbosity;
}

void time_stepper::setup_cdr(){
  CH_TIME("time_stepper::setup_cdr");
  if(m_verbosity > 5){
    pout() << "time_stepper::setup_cdr" << endl;
  }

  m_cdr = RefCountedPtr<cdr_layout> (new cdr_layout(m_plaskin));
  m_cdr->set_amr(m_amr);
  m_cdr->set_computational_geometry(m_compgeom);
  m_cdr->set_physical_domain(m_physdom);
  m_cdr->sanity_check();
  m_cdr->allocate_internals();
  m_cdr->initial_data();
}

void time_stepper::setup_poisson(){
  CH_TIME("time_stepper::setup_poisson");
  if(m_verbosity > 5){
    pout() << "time_stepper::setup_poisson" << endl;
  }

  m_poisson = RefCountedPtr<poisson_solver> (new poisson_multifluid_gmg());
  m_poisson->set_amr(m_amr);
  m_poisson->set_computational_geometry(m_compgeom);
  m_poisson->set_physical_domain(m_physdom);
  m_poisson->set_potential(m_potential);

  // Boundary conditions, but these should definitely come through an input function. 
  if(SpaceDim == 2){
    m_poisson->set_neumann_wall_bc(0,   Side::Lo, 0.0);                  
    m_poisson->set_neumann_wall_bc(0,   Side::Hi, 0.0);
    m_poisson->set_dirichlet_wall_bc(1, Side::Lo, potential::ground);
    m_poisson->set_dirichlet_wall_bc(1, Side::Hi, potential::live);
  }
  else if(SpaceDim == 3){
    m_poisson->set_neumann_wall_bc(0,   Side::Lo, 0.0);                  
    m_poisson->set_neumann_wall_bc(0,   Side::Hi, 0.0);
    m_poisson->set_neumann_wall_bc(1,   Side::Lo, 0.0);                  
    m_poisson->set_neumann_wall_bc(1,   Side::Hi, 0.0);
    m_poisson->set_dirichlet_wall_bc(2, Side::Lo, potential::ground);
    m_poisson->set_dirichlet_wall_bc(2, Side::Hi, potential::live);
  }

  m_poisson->sanity_check();
  m_poisson->allocate_internals();
}

void time_stepper::setup_rte(){
  CH_TIME("time_stepper::setup_rte");
  if(m_verbosity > 5){
    pout() << "time_stepper::setup_rte" << endl;
  }

  m_rte = RefCountedPtr<rte_layout> (new rte_layout(m_plaskin));
  m_rte->set_amr(m_amr);
  m_rte->set_computational_geometry(m_compgeom);
  m_rte->set_physical_domain(m_physdom);
  m_rte->sanity_check();
  m_rte->allocate_internals();

  if(!m_rte->is_stationary()){
    m_rte->initial_data();
  }
}

void time_stepper::setup_sigma(){
  CH_TIME("time_stepper::setup_poisson");
  if(m_verbosity > 5){
    pout() << "time_stepper::setup_poisson" << endl;
  }

  m_sigma = RefCountedPtr<sigma_solver> (new sigma_solver());
  m_sigma->set_amr(m_amr);
  m_sigma->set_plasma_kinetics(m_plaskin);
  m_sigma->set_physical_domain(m_physdom);
  m_sigma->allocate_internals();
  m_sigma->initial_data();
}

void time_stepper::solve_poisson(){
  CH_TIME("time_stepper::solve_poisson()");
  if(m_verbosity > 5){
    pout() << "time_stepper::solve_poisson()" << endl;
  }

  this->compute_rho();
  m_poisson->solve(m_poisson->get_state(),
		   m_poisson->get_source(),
		   m_sigma->get_state(),
		   false);

  m_poisson->write_plot_file();
}

void time_stepper::solve_poisson(MFAMRCellData&                a_potential,
				 MFAMRCellData*                a_rhs,
				 const Vector<EBAMRCellData*>  a_densities,
				 const EBAMRIVData&            a_sigma,
				 const centering::which_center a_centering){
  CH_TIME("time_stepper::solve_poisson(full)");
  if(m_verbosity > 5){
    pout() << "time_stepper::solve_poisson(full)" << endl;
  }

  const int ncomp = 1;
  
  MFAMRCellData* p_rhs;
  MFAMRCellData rhs;
  if(a_rhs == NULL){
    m_amr->allocate(rhs, ncomp);
    p_rhs = &rhs;
  }
  else {
    p_rhs = a_rhs;
  }

  this->compute_rho(rhs, a_densities, a_centering);

  m_poisson->solve(a_potential, *p_rhs, a_sigma, false);
}

void time_stepper::solve_rte(const Real a_dt){
  CH_TIME("time_stepper::solve_rte()");
  if(m_verbosity > 5){
    pout() << "time_stepper::solve_rte()" << endl;
  }

  const phase::which_phase rte_phase = m_rte->get_phase();

  EBAMRCellData E;
  m_amr->allocate(E, rte_phase, SpaceDim);
  this->compute_E(E, rte_phase, m_poisson->get_state());

  Vector<EBAMRCellData*> states     = m_rte->get_states();
  Vector<EBAMRCellData*> rhs        = m_rte->get_sources();
  Vector<EBAMRCellData*> cdr_states = m_cdr->get_states();

  this->solve_rte(states, rhs, cdr_states, E, a_dt, centering::cell_center);
}

void time_stepper::solve_rte(Vector<EBAMRCellData*>&       a_states,
			     Vector<EBAMRCellData*>&       a_rhs,
			     const Vector<EBAMRCellData*>& a_cdr_states,
			     const EBAMRCellData&          a_E,
			     const Real                    a_dt,
			     const centering::which_center a_centering){
  CH_TIME("time_stepper::solve_rte(full)");
  if(m_verbosity > 5){
    pout() << "time_stepper::solve_rte(full)" << endl;
  }

  this->compute_photon_source_terms(a_rhs, a_cdr_states, a_E, a_centering);


  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
    const int idx = solver_it.get_solver();
    
    RefCountedPtr<rte_solver>& solver = solver_it();
    EBAMRCellData& state              = *a_states[idx];
    EBAMRCellData& rhs                = *a_rhs[idx];
    solver->advance(a_dt, state, rhs);
  }

}
