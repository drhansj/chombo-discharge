#include <CD_Driver.H>
#include <CD_FieldSolverMultigrid.H>
#include <CD_RegularGeometry.H>
#include <CD_Tesselation.H>
#include <CD_FieldStepper.H>
#include <ParmParse.H>

using namespace ChomboDischarge;
using namespace Physics::Electrostatics;

int main(int argc, char* argv[]){

#ifdef CH_MPI
  MPI_Init(&argc, &argv);
#endif

  // Build class options from input script and command line options
  const std::string input_file = argv[1];
  ParmParse pp(argc-2, argv+2, NULL, input_file.c_str());

  // Set geometry and AMR 
  RefCountedPtr<ComputationalGeometry> compgeom = RefCountedPtr<ComputationalGeometry> (new Tesselation());
  RefCountedPtr<AmrMesh> amr                    = RefCountedPtr<AmrMesh> (new AmrMesh());
  RefCountedPtr<GeoCoarsener> geocoarsen        = RefCountedPtr<GeoCoarsener> (new GeoCoarsener());
  RefCountedPtr<CellTagger> tagger              = RefCountedPtr<CellTagger> (NULL);

  // Set up basic Poisson, potential = 1 
  auto timestepper = RefCountedPtr<FieldStepper<FieldSolverMultigrid> >
     (new FieldStepper<FieldSolverMultigrid>());

  // Real sigma0 = 0.0;
  // Real sigma1 = 10.0;
  // auto sigma = [&](const RealVect& x) -> Real {
  //   if (x[0] < 0)
  //     return sigma0;
  //   else
  //     return sigma1;
  // };

  // timestepper->setSigma(sigma);

  // Set up the Driver and run it
  RefCountedPtr<Driver> engine = RefCountedPtr<Driver> (new Driver(compgeom, timestepper, amr, tagger, geocoarsen));
  //engine->setupAndRun(input_file);

  int initialRegrids, restartStep;
  pp.get("initial_regrids", initialRegrids);
  pp.get("restart", restartStep);
  std::string outputDirectory, outputFileNames;
  pp.get("output_directory", outputDirectory);
  pp.get("output_names", outputFileNames);
  char iter_str[100];
  sprintf(iter_str, ".check%07d.%dd.hdf5", 0, SpaceDim);
  const std::string restartFile = outputDirectory + "/chk/" + outputFileNames + std::string(iter_str);
  engine->setup(input_file, initialRegrids, false, restartFile);

  Real volt0 = 0.0;
  Real volt1 = 10.0;
  auto myElectrodeVoltage = [&](const RealVect a_pos, const Real a_time) -> Real{
    if (a_pos[0] < 0.1)
      return volt0;
    else
      return volt1;
  };
  auto fieldsolver = timestepper->getSolver();
  fieldsolver->setElectrodeDirichletFunction(0, myElectrodeVoltage);

  int maxSteps;
  Real startTime, stopTime;
  pp.get("max_steps", maxSteps);
  pp.get("start_time", startTime);
  pp.get("stop_time", stopTime);
  engine->run(startTime, stopTime, maxSteps);

#ifdef CH_MPI
  CH_TIMER_REPORT();
  MPI_Finalize();
#endif
}
