#include <CD_Driver.H>
#include "CD_Vessel.H"
#include "CD_DoubleStl.H"
#include <CD_DischargeInceptionStepper.H>
#include <CD_DischargeInceptionTagger.H>
#include <CD_LookupTable.H>
#include <CD_DataParser.H>
#include <ParmParse.H>

using namespace ChomboDischarge;
using namespace Physics::DischargeInception;

int
main(int argc, char* argv[])
{

#ifdef CH_MPI
  MPI_Init(&argc, &argv);
#endif

  // Read the input file into the ParmParse table
  const std::string input_file = argv[1];
  ParmParse         pp(argc - 2, argv + 2, NULL, input_file.c_str());

  // gas type
  enum gasType {air, sf6};
  auto activeGas = air;

  // Read BOLSIG+ data into alpha and eta coefficients
  const Real N  = 2.45E25;
  const Real O2 = 0.2;
  const Real N2 = 0.8;

  const Real T  = 300.0; // room temp in K
  const Real P = 1.e5; // 1atm in Pa

  LookupTable1D<> ionizationData = DataParser::fractionalFileReadASCII("transport_data.txt",
                                                                       "E/N (Td)	Townsend ioniz. coef. alpha/N (m2)",
                                                                       "");
  LookupTable1D<> attachmentData = DataParser::fractionalFileReadASCII("transport_data.txt",
                                                                       "E/N (Td)	Townsend attach. coef. eta/N (m2)",
                                                                       "");

  ionizationData.truncate(10, 2000, 0);
  attachmentData.truncate(10, 2000, 0);

  ionizationData.scale<0>(N * 1.E-21);
  attachmentData.scale<0>(N * 1.E-21);

  ionizationData.scale<1>(N);
  attachmentData.scale<1>(N);

  ionizationData.prepareTable(0, 500, LookupTable::Spacing::Exponential);
  attachmentData.prepareTable(0, 500, LookupTable::Spacing::Exponential);

  LookupTable1D<> ionDiffData = DataParser::fractionalFileReadASCII("ionDiffusion.txt",
                                                                    "E (V/cm-atm)  Ion Diffusion (cm^2/V s)",
                                                                    "");
  ionDiffData.scale<0>(10./133.322);
  ionDiffData.scale<1>(1./100.);
  ionDiffData.prepareTable(0, 7, LookupTable::Spacing::Uniform);

  // Read data for the voltage curve.
  Real peak           = 0.0;
  Real t0             = 0.0;
  Real t1             = 0.0;
  Real t2             = 0.0;
  Real secondTownsend = 0.0;

  ParmParse li("lightning_impulse");
  ParmParse di("DischargeInception");
  li.get("peak", peak);
  li.get("start", t0);
  li.get("tail_time", t1);
  li.get("front_time", t2);

  di.get("second_townsend", secondTownsend);

  // Set geometry and AMR
  RefCountedPtr<ComputationalGeometry> compgeom = RefCountedPtr<ComputationalGeometry>(new DoubleStl());
  RefCountedPtr<AmrMesh>               amr      = RefCountedPtr<AmrMesh>(new AmrMesh());

  // Define transport data
  auto alpha = [&](const Real& E, const RealVect& x) -> Real {
    Real e_si = E/P; // V/m pa
    Real e = e_si * (1.e5)/(1.e3 * 1.e3); // kV/mm bar

    Real alpha = 0;
    // air - in units 1/mm bar
    if (activeGas == air)
      {
        if (e < 2.588)
          alpha = 0;
        else if (e < 7.943)
          alpha = 1.6053 * std::pow(e - 2.165, 2) - 0.2873;
        else
          alpha = 16.7766 * e - 80.006;
      }

    // sf6 - in units 1/mm bar
    else if (activeGas == sf6)
      {
        if (e < 8.9246)
          alpha = 0;
        else if (e < 12.36)
          alpha = 27.9 * (e - 8.9246);
        else
          alpha = 22.3595 * e - 180.1709;
      }

    alpha *= 1.e3 / 1.e0; // from 1/mm bar to 1/m pa - but bar to Pa scales seem wong??
    return alpha;
    //return ionizationData.interpolate<1>(E);
  };
  auto eta = [&](const Real& E, const RealVect& x) -> Real {
    //return attachmentData.interpolate<1>(E);
    return 0.;
  };
  auto alphaEff = [&](const Real& E, const RealVect x) -> Real {
    return alpha(E, x) - eta(E, x);
  };
  auto bgRate = [&](const Real& E, const RealVect& x) -> Real {
    return 0.;
  };
  auto detachRate = [&](const Real& E, const RealVect& x) -> Real {
    return 0.;
  };
  auto ionMobility = [&](const Real& E) -> Real {
    // return 0.;
    return 0.595e-4; // 0.595 cm2/V-sec
  };
  auto ionDiffusion = [&](const Real& E) -> Real {
    //return 0.;
    // pout() << "ionDiff" << E << " -> "
    //        << ionMobility(E) * Units::kb * T / Units::Qe
    //        << " vs "
    //        << ionDiffData.interpolate<1>(E)
    //        << std::endl;
    // return ionMobility(E) * Units::kb * T / Units::Qe;
    return ionDiffData.interpolate<1>(E);
  };
  auto ionDensity = [&](const RealVect& x) -> Real {
    return 0.;
  };
  auto voltageCurve = [&](const Real& t) -> Real {
    return 100000 * 1.054 * (exp(-t * 1.e6/67.) - exp(-t * 1.e6/.61)); // V
  };
  auto fieldEmission = [&](const Real& E, const RealVect& x) -> Real {
    return 0.;
  };
  auto secondaryEmission = [&](const Real& E, const RealVect& x) -> Real {
    return 0.;
    //return secondTownsend;
  };

  // Set up time stepper
  auto timestepper = RefCountedPtr<DischargeInceptionStepper<>>(new DischargeInceptionStepper<>());
  auto celltagger  = RefCountedPtr<DischargeInceptionTagger>(
    new DischargeInceptionTagger(amr, timestepper->getElectricField(), alphaEff));

  // Set transport data
  timestepper->setAlpha(alpha);
  timestepper->setEta(eta);
  timestepper->setBackgroundRate(bgRate);
  timestepper->setDetachmentRate(detachRate);
  timestepper->setIonMobility(ionMobility);
  timestepper->setIonDiffusion(ionDiffusion);
  timestepper->setIonDensity(ionDensity);
  timestepper->setVoltageCurve(voltageCurve);
  timestepper->setFieldEmission(fieldEmission);
  timestepper->setSecondaryEmission(secondaryEmission);

  // Set up the Driver and run it
  RefCountedPtr<Driver> engine = RefCountedPtr<Driver>(new Driver(compgeom, timestepper, amr, celltagger));
  engine->setupAndRun(input_file);

#ifdef CH_MPI
  CH_TIMER_REPORT();
  MPI_Finalize();
#endif
}
