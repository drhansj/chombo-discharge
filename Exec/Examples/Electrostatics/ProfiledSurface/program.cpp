#include <CD_Driver.H>
#include <CD_FieldSolverMultigrid.H>
#include <CD_DiskProfiledPlane.H>
#include <CD_FieldStepper.H>
#include <CD_DataParser.H>
#include <ParmParse.H>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

using namespace ChomboDischarge;
using namespace Physics::Electrostatics;

// FieldStepper that reads grid-based boundary conditions from a text file
template <class T>
class GridBCFieldStepper : public FieldStepper<T>
{
private:
  // Face configuration structure
  struct FaceConfig {
    int faceId;
    std::string fileName;
    std::vector<int> coordIndices;  // Which coordinates vary for this face
    std::vector<int> gridIndices;   // Which grid indices to use for storage
    bool isActive;                  // Whether this face is active in current dimension
  };
  
  // Get face configurations for loading and retrieval
  std::vector<FaceConfig> getFaceConfigs() const {
    return {
      {0, "face_0.txt", {1}, {0, 1}, true},      // x_low: y varies, store in [0][j]
      {1, "face_1.txt", {1}, {0, 1}, true},      // x_high: y varies, store in [0][j]
      {2, "face_2.txt", {0}, {0, 1}, true},      // y_low: x varies, store in [i][0]
      {3, "face_3.txt", {0}, {0, 1}, true},      // y_high: x varies, store in [i][0]
      {4, "face_4.txt", {0, 1}, {0, 1}, SpaceDim == 3}, // z_low: x,y vary, store in [i][j] (3D only)
      {5, "face_5.txt", {0, 1}, {0, 1}, SpaceDim == 3}  // z_high: x,y vary, store in [i][j] (3D only)
    };
  }

public:
  GridBCFieldStepper() : FieldStepper<T>() {}
  
  virtual ~GridBCFieldStepper() {}
  
  virtual void setupSolvers() override
  {
    // Call the parent setupSolvers to create the field solver
    FieldStepper<T>::setupSolvers();
    
    // Now the field solver is created, so we can set the boundary functions
    auto fieldSolver = this->getSolver();
    
    // Load boundary condition values from separate face files
    // Each face has its own file: face_0.txt, face_1.txt, etc.
    std::map<int, std::vector<std::vector<Real>>> faceBCValues;
    
    // Define face configurations
    std::vector<FaceConfig> faceConfigs = getFaceConfigs();
    
    // Load data for each face
    for (const auto& config : faceConfigs) {
      if (!config.isActive) continue;
      
      // Check if file exists
      std::ifstream fileCheck(config.fileName);
      if (!fileCheck.good()) {
        pout() << "GridBCFieldStepper: Warning - file " << config.fileName << " not found, skipping face " << config.faceId << endl;
        continue;
      }
      fileCheck.close();
      
      // Read x y z value format from file
      std::vector<std::vector<Real>> rawData;
      std::ifstream file(config.fileName);
      if (!file.is_open()) {
        pout() << "GridBCFieldStepper: Warning - could not open file " << config.fileName << ", skipping face " << config.faceId << endl;
        continue;
      }
      
      std::string line;
      while (std::getline(file, line)) {
        // Skip comment lines and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::vector<Real> row;
        Real val;
        while (iss >> val) {
          row.push_back(val);
        }
        if (row.size() >= 4) {
          rawData.push_back(row);
        }
      }
      file.close();
      
      // Check if we got any data
      if (rawData.empty()) {
        pout() << "GridBCFieldStepper: Warning - no data read from " << config.fileName << ", skipping face " << config.faceId << endl;
        continue;
      }
      
      pout() << "GridBCFieldStepper: Raw data size for face " << config.faceId << ": " << rawData.size() << endl;
      
      // Debug: Print first few raw data rows
      int debugCount = 0;
      for (const auto& row : rawData) {
        if (debugCount < 3) {
          pout() << "GridBCFieldStepper: Raw row " << debugCount << ": ";
          for (size_t col = 0; col < row.size(); col++) {
            pout() << row[col] << " ";
          }
          pout() << endl;
          debugCount++;
        }
      }
      
      int valuesStored = 0;
      
      // Initialize empty grid (no default values)
      int gridSize = 64;
      faceBCValues[config.faceId] = std::vector<std::vector<Real>>(gridSize, std::vector<Real>(gridSize, 0.0));
      
      // Load values using the face configuration
      valuesStored = loadFaceValues(config, rawData, faceBCValues[config.faceId], gridSize);
      
      pout() << "GridBCFieldStepper: Loaded " << rawData.size() << " values from " << config.fileName 
             << ", stored " << valuesStored << " values in grid" << endl;
        
      // Debug: Print first few values for verification
      if (valuesStored > 0) {
        pout() << "GridBCFieldStepper: First few values for face " << config.faceId << ": ";
        int count = 0;
        for (int idx = 0; idx < gridSize && count < 5; idx++) {
          if (SpaceDim == 2) {
            if (config.faceId <= 1) {
              // x-faces: check first row
              if (faceBCValues[config.faceId][0][idx] != 0.0) {
                pout() << "[" << idx << "]=" << faceBCValues[config.faceId][0][idx] << " ";
                count++;
              }
            } else if (config.faceId <= 3) {
              // y-faces: check first column
              if (faceBCValues[config.faceId][idx][0] != 0.0) {
                pout() << "[" << idx << "]=" << faceBCValues[config.faceId][idx][0] << " ";
                count++;
              }
            }
          } else {
            // 3D case: check 2D grid
            for (int idx2 = 0; idx2 < gridSize && count < 5; idx2++) {
              if (faceBCValues[config.faceId][idx][idx2] != 0.0) {
                pout() << "[" << idx << "," << idx2 << "]=" << faceBCValues[config.faceId][idx][idx2] << " ";
                count++;
              }
            }
          }
        }
        pout() << endl;
      }
    }
    
    // Create boundary functions using loaded values
    auto xLoBcFunction = [this, faceBCValues](const RealVect a_position, const Real a_time) -> Real {
      return getFaceValue(0, a_position, faceBCValues);
    };
    
    auto xHiBcFunction = [this, faceBCValues](const RealVect a_position, const Real a_time) -> Real {
      return getFaceValue(1, a_position, faceBCValues);
    };
    
    auto yLoBcFunction = [this, faceBCValues](const RealVect a_position, const Real a_time) -> Real {
      return getFaceValue(2, a_position, faceBCValues);
    };
    
    auto yHiBcFunction = [this, faceBCValues](const RealVect a_position, const Real a_time) -> Real {
      return getFaceValue(3, a_position, faceBCValues);
    };
    
    auto zLoBcFunction = [this, faceBCValues](const RealVect a_position, const Real a_time) -> Real {
      return getFaceValue(4, a_position, faceBCValues);
    };
    
    auto zHiBcFunction = [this, faceBCValues](const RealVect a_position, const Real a_time) -> Real {
      return getFaceValue(5, a_position, faceBCValues);
    };
    
    // Set the boundary functions
    fieldSolver->setDomainSideBcFunction(0, Side::Lo, xLoBcFunction);  // x low
    fieldSolver->setDomainSideBcFunction(0, Side::Hi, xHiBcFunction);  // x high
    fieldSolver->setDomainSideBcFunction(1, Side::Lo, yLoBcFunction);  // y low
    //fieldSolver->setDomainSideBcFunction(1, Side::Hi, yHiBcFunction);  // y high
    
    // Set z-direction boundary functions (only for 3D)
    if (SpaceDim == 3) {
      fieldSolver->setDomainSideBcFunction(2, Side::Lo, zLoBcFunction);  // z low
      fieldSolver->setDomainSideBcFunction(2, Side::Hi, zHiBcFunction);  // z high
    }
    
    pout() << "GridBCFieldStepper: Boundary functions set successfully" << endl;
  }
  
private:
  // Load values for a specific face using its configuration
  int loadFaceValues(const FaceConfig& config, const std::vector<std::vector<Real>>& rawData, 
                    std::vector<std::vector<Real>>& faceValues, int gridSize) {
    int valuesStored = 0;
    
    // Get domain bounds by parsing input file directly
    RealVect domainLo, domainHi;
    {
      ParmParse pp("AmrMesh");
      Vector<Real> v(SpaceDim);
      
      pp.getarr("lo_corner", v, 0, SpaceDim);
      domainLo = RealVect(D_DECL(v[0], v[1], v[2]));
      pp.getarr("hi_corner", v, 0, SpaceDim);
      domainHi = RealVect(D_DECL(v[0], v[1], v[2]));
    }
    
    for (const auto& row : rawData) {
      if (row.size() < 4) continue;
      
      Real x = row[0];
      Real y = row[1];
      Real z = row[2];
      Real value = row[3];
      
      // Convert coordinates to grid indices based on face configuration
      std::vector<int> indices;
      std::vector<Real> coords = {x, y, z};
      
      for (int coordIdx : config.coordIndices) {
        if (coordIdx < coords.size()) {
          // Use domain bounds instead of hardcoded values
          Real domainMin = domainLo[coordIdx];
          Real domainMax = domainHi[coordIdx];
          Real domainWidth = domainMax - domainMin;
          
          int gridIdx = static_cast<int>((coords[coordIdx] - domainMin) / domainWidth * (gridSize - 1));
          gridIdx = std::max(0, std::min(gridIdx, gridSize - 1));
          indices.push_back(gridIdx);
        }
      }
      
      // Store value using the grid indices from configuration
      if (indices.size() == config.gridIndices.size()) {
        int i = (config.gridIndices[0] < indices.size()) ? indices[config.gridIndices[0]] : 0;
        int j = (config.gridIndices[1] < indices.size()) ? indices[config.gridIndices[1]] : 0;
        
        faceValues[i][j] = value;
        valuesStored++;
        
        // Debug: Print first few stored values
        if (valuesStored <= 3) {
          pout() << "GridBCFieldStepper: Stored face " << config.faceId << " [" << i << "][" << j << "] = " << value 
                 << " (from x=" << x << " y=" << y << " z=" << z << ")" << endl;
        }
      }
    }
    
    return valuesStored;
  }

  // Get the boundary value for a specific face and position
  Real getFaceValue(int faceId, const RealVect& position, 
                   const std::map<int, std::vector<std::vector<Real>>>& faceBCValues) {
    
    if (faceBCValues.find(faceId) == faceBCValues.end()) {
      return 0.0; // No face data found
    }
    
    const auto& faceValues = faceBCValues.at(faceId);
    if (faceValues.empty() || faceValues[0].empty()) {
      return 0.0; // Empty face data
    }
    
    int gridSize = faceValues.size();
    Real result = 0.0;
    
    // Get face configurations for value retrieval
    std::vector<FaceConfig> faceConfigs = getFaceConfigs();
    
    // Find the configuration for this face
    const FaceConfig* config = nullptr;
    for (const auto& cfg : faceConfigs) {
      if (cfg.faceId == faceId && cfg.isActive) {
        config = &cfg;
        break;
      }
    }
    
    if (!config) {
      return 0.0; // Face not found or not active
    }
    
    // Get domain bounds by parsing input file directly
    RealVect domainLo, domainHi;
    {
      ParmParse pp("AmrMesh");
      Vector<Real> v(SpaceDim);
      
      pp.getarr("lo_corner", v, 0, SpaceDim);
      domainLo = RealVect(D_DECL(v[0], v[1], v[2]));
      pp.getarr("hi_corner", v, 0, SpaceDim);
      domainHi = RealVect(D_DECL(v[0], v[1], v[2]));
    }
    
    // Convert position to grid indices based on face configuration
    std::vector<int> indices;
    std::vector<Real> coords = {position[0], position[1], position[2]};
    
    for (int coordIdx : config->coordIndices) {
      if (coordIdx < coords.size()) {
        // Use domain bounds instead of hardcoded values
        Real domainMin = domainLo[coordIdx];
        Real domainMax = domainHi[coordIdx];
        Real domainWidth = domainMax - domainMin;
        
        int gridIdx = static_cast<int>((coords[coordIdx] - domainMin) / domainWidth * (gridSize - 1));
        gridIdx = std::max(0, std::min(gridIdx, gridSize - 1));
        indices.push_back(gridIdx);
      }
    }
    
    // Retrieve value using the grid indices from configuration
    if (indices.size() == config->gridIndices.size()) {
      int i = (config->gridIndices[0] < indices.size()) ? indices[config->gridIndices[0]] : 0;
      int j = (config->gridIndices[1] < indices.size()) ? indices[config->gridIndices[1]] : 0;
      
      result = faceValues[i][j];
      
      // Debug output (only for first few calls to avoid spam)
      static int debugCount = 0;
      if (debugCount < 10) {
        if (SpaceDim == 2) {
          pout() << "getFaceValue(2D): face=" << faceId << " pos=(" << position[0] << "," << position[1] 
                 << ") indices=(" << i << "," << j << ") value=" << result << endl;
        } else {
          pout() << "getFaceValue(3D): face=" << faceId << " pos=(" << position[0] << "," << position[1] << "," << position[2]
                 << ") indices=(" << i << "," << j << ") value=" << result << endl;
        }
        debugCount++;
      }
    }
    
    return result;
  }
};

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
  RefCountedPtr<ComputationalGeometry> compgeom   = RefCountedPtr<ComputationalGeometry>(new DiskProfiledPlane());
  RefCountedPtr<AmrMesh>               amr        = RefCountedPtr<AmrMesh>(new AmrMesh());
  RefCountedPtr<GeoCoarsener>          geocoarsen = RefCountedPtr<GeoCoarsener>(new GeoCoarsener());
  RefCountedPtr<CellTagger>            tagger     = RefCountedPtr<CellTagger>(NULL);

  // Set up basic Poisson with grid-based boundary conditions
  auto timestepper = RefCountedPtr<GridBCFieldStepper<FieldSolverMultigrid>>(new GridBCFieldStepper<FieldSolverMultigrid>());

  // Set up the Driver and run it
  RefCountedPtr<Driver> engine = RefCountedPtr<Driver>(new Driver(compgeom, timestepper, amr, tagger, geocoarsen));
  engine->setupAndRun(input_file);

#ifdef CH_MPI
  CH_TIMER_REPORT();
  MPI_Finalize();
#endif

  return 0;
} 

