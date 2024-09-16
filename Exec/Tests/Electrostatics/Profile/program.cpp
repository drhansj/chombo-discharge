#include "CD_Driver.H"
#include "CD_FieldSolverMultigrid.H"
#include <CD_RodPlaneProfile.H>
#include "CD_FieldStepper.H"
#include "CD_CellTagger.H"
#include "CD_AmrMesh.H"
#include "DebuggingTools.H"
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
  CellTaggerAroundPoint(RefCountedPtr<AmrMesh> a_mesh_ptr ,
                        RealVect               a_center   ,
                        Real                   a_manhattan_radius ): CellTagger()
  {
    m_domains = a_mesh_ptr->getDomains();
    m_dx      = a_mesh_ptr->getDx();
    m_origin  = a_mesh_ptr->getProbLo();
    m_center  = a_center;
    m_manhattan_radius  = a_manhattan_radius;
  }
  virtual ~CellTaggerAroundPoint()
  {;}

  void regrid()
  {;}

  void parseOptions()
  {;}

  ///more standard API
  void getTagsVector(Vector<IntVectSet>& a_tags) const
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
      a_tags[ilev] = IntVectSet(tag_region);
    } //end loop over levels
  }   //end function getTagVec

  ///EBAMRTags is a Vector<RefCountedPtr<LayoutData<DenseIntVectSet>
  bool tagCells(EBAMRTags& a_tags)
  {
    Vector<IntVectSet> vec_ivs_tags;
    getTagsVector(vec_ivs_tags);
    bool retval = false;
    for(int ilev = 0; ilev < a_tags.size(); ilev++)
    {
      LayoutData<DenseIntVectSet>  & level_tag_ldivs = (*a_tags[ilev]);
      const BoxLayout& base_layout = level_tag_ldivs.boxLayout();
      const DisjointBoxLayout* dis_box_ptr = dynamic_cast<const DisjointBoxLayout*>(&base_layout);
      if(dis_box_ptr == NULL)
      {
        MayDay::Error("program.cpp: tagCells:logic error 4586");
      }
      DataIterator dit = dis_box_ptr->dataIterator();
      for(int ibox = 0; ibox < dit.size(); ibox++)
      {
        DenseIntVectSet& box_dense_ivs = level_tag_ldivs[dit[ibox]];
        Box  valid_box                  = (*dis_box_ptr) [dit[ibox]];
        IntVectSet box_intersected_ivs  = vec_ivs_tags[ilev];
        box_intersected_ivs &= valid_box;
        box_dense_ivs.makeEmptyBits();
        for(IVSIterator ivsit(box_intersected_ivs); ivsit.ok(); ++ivsit)
        {
          IntVect iv_tag = ivsit();
          box_dense_ivs |= iv_tag;
          retval = true;
        } //end loop over tagged cells for this box
      }   //end loop over boxes
    }     //end loop over levels
    return retval;
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
  RealVect center = RealVect::Unit;
  center *= 0.5;
  Real radius = 0.1;
  CellTaggerAroundPoint* derived_ptr = new CellTaggerAroundPoint( amr, center, radius);
  CellTagger*               base_ptr = static_cast<CellTagger*>(derived_ptr);
  Vector<IntVectSet>         tags_level;
  derived_ptr->getTagsVector(tags_level);
  RefCountedPtr<CellTagger>  tagger  = RefCountedPtr<CellTagger>(base_ptr);

  // Set up basic Poisson, potential = 1
  auto timestepper = RefCountedPtr<FieldStepper<FieldSolverMultigrid>>(new FieldStepper<FieldSolverMultigrid>());

  // Set up the Driver and run it
  RefCountedPtr<Driver> engine = RefCountedPtr<Driver>(new Driver(compgeom, timestepper, amr, geocoarsen, tagger, &tags_level));
  engine->setupAndRun(input_file);

#ifdef CH_MPI
  CH_TIMER_REPORT();
  MPI_Finalize();
#endif
}
