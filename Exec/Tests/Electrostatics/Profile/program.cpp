#include "CD_Driver.H"
#include "CD_FieldSolverMultigrid.H"
#include <CD_RodPlaneProfile.H>
#include "CD_FieldStepper.H"
#include "CD_CellTagger.H"
#include "ParmParse.H"

using namespace ChomboDischarge;
using namespace Physics::Electrostatics;
/// Only tag cells near a particular point
///this dance is because I cannot move CellTagger to EBTools.
class CellTaggerAroundPoint: public CellTagger
{
private:
  CellTaggerAroundPoint();
  
public:
  Vector<ProblemDomain>  m_domains           ;
  Vector<Real>           m_dx                ;
  RealVect               m_origin;
  RealVect               m_center            ;
  Real                   m_manhattan_radius  ;  
  CellTaggerAroundPoint(RefCountedPtr<AMRMesh> a_mesh_ptr ,
                        RealVect               a_center   ,
                        Real                   a_manhattan_radius ): CellTagger()
  {
    m_domains = a_mesh_ptr->getDomains();
    m_dx      = a_mesh_ptr->getDx();
    m_origin  = a_mesh_ptr->getProbLo();
    m_center  = a_center;
    m_radius  = a_radius;

  }
  virtual ~CellTaggerAroundPoint()
  {;}

  void regrid()
  {;}

  void parseOptions()
  {;}

  bool tagCells(EBAMRTags& a_tags)
  {
    a_tags.resize(m_domains.size());
    for(int ilev = 0; ilev < m_domains.size(); ilev++)
    {
      Real dx_lev = m_dx[ilev];
      IntVect iv_cent;
      Real r_min_rad = std::min(1.1*dx_lev, m_manhattan_radius);
      
      int i_radius = int(r_min_rad/dx_lev);
      IntVect region_lo, region_hi;
      for(int idir = 0; idir < SpaceDim; idir++)
      {
        Real center_dist = m_center[idir] - m_origin[idir];
        iv_cent[idir] = int(center_dist/dx_lev);
        region_lo[idir] = iv_cent[idir] - i_radius;
        region_hi[idir] = iv_cent[idir] + i_radius;
      }
      Box tag_region(region_lo, region_hi);
      a_tags[ilev] = RefCountedPtr<DenseIntVectSet>(new DenseIntVectSet(tag_region));
    } //end loop over levels
  } //end function tagCells
  
}; //end class CellTaggerAroundPoint
int
main(int argc, char* argv[])
{

#ifdef CH_MPI
  MPI_Init(&argc, &argv);
#endif

  // Build class options from input script and command line options
  const std::string input_file = argv[1];
  ParmParse         pp(argc - 2, argv + 2, NULL, input_file.c_str());

  // Set geometry and AMR
  RefCountedPtr<ComputationalGeometry> compgeom   = RefCountedPtr<ComputationalGeometry>(new RodPlaneProfile());
  RefCountedPtr<AmrMesh>               amr        = RefCountedPtr<AmrMesh>(new AmrMesh());
  RefCountedPtr<GeoCoarsener>          geocoarsen = RefCountedPtr<GeoCoarsener>(new GeoCoarsener());
  RealVect center(D_DECL(0.5, 0.5, 0,5));
  Real radius = 0.1;
  RefCountedPtr<CellTagger>            tagger     = RefCountedPtr<CellTagger>(new CellTaggerAroundPoint( amr, center, radius));

  // Set up basic Poisson, potential = 1
  auto timestepper = RefCountedPtr<FieldStepper<FieldSolverMultigrid>>(new FieldStepper<FieldSolverMultigrid>());

  // Set up the Driver and run it
  RefCountedPtr<Driver> engine = RefCountedPtr<Driver>(new Driver(compgeom, timestepper, amr, tagger, geocoarsen));
  engine->setupAndRun(input_file);

#ifdef CH_MPI
  CH_TIMER_REPORT();
  MPI_Finalize();
#endif
}
