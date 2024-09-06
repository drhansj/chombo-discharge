/* chombo-discharge
 * Copyright © 2021 SINTEF Energy Research.
 * Please refer to Copyright.txt and LICENSE in the chombo-discharge root directory.
 */

/*!
  @file   CD_DoubleStl.cpp
  @brief  Implementation of CD_DoubleStl.H
  @author Hans Johansen
*/

// Chombo includes
#include <ParmParse.H>

// EBGeometry include
#include <EBGeometry.hpp>

// Our includes
#include <CD_DoubleStl.H>
#include <CD_EBGeometryIF.H>
#include <CD_NamespaceHeader.H>

#if 0

  std::string filename;
  Real        zCoord;
  bool        flipInside;

  // Get input options.
  ParmParse pp("Tesselation");

  pp.get("mesh_file", filename);
  pp.get("z_coord", zCoord);
  pp.get("flip_inside", flipInside);

  // Read the PLY file and put it in a linearized BVH hierarchy.
  auto implicitFunction = EBGeometry::Parser::readIntoLinearBVH<T>(filename);

  // Put our level-set into Chombo datastructures.
  RefCountedPtr<BaseIF> baseIF = RefCountedPtr<BaseIF>(new EBGeometryIF<T>(implicitFunction, flipInside, zCoord));

  m_electrodes.push_back(Electrode(baseIF, true));

#endif

using T = float;

DoubleStl::DoubleStl()
{
  this->setGasPermittivity(1.0);

  ParmParse pp1("DoubleStl.stl1");
  ParmParse pp2("DoubleStl.stl2");

  bool use_stl1;
  bool use_stl2;

  pp1.get("on", use_stl1);
  pp2.get("on", use_stl2);

  std::string filename;
  Real zCoord;
  bool flipInside;
  bool live;

  if (use_stl1) {
    pp1.get("mesh_file", filename);
    pp1.get("z_coord", zCoord);
    pp1.get("flip_inside", flipInside);
    pp1.get("live", live);

    // Read the PLY file and put it in a linearized BVH hierarchy.
    auto implicitFunction = EBGeometry::Parser::readIntoLinearBVH<T>(filename);

    // Put our level-set into Chombo datastructures.
    RefCountedPtr<BaseIF> baseIF = RefCountedPtr<BaseIF>(
        new EBGeometryIF<T>(implicitFunction, flipInside, zCoord));
    m_electrodes.push_back(Electrode(baseIF, live));

    /*
    pp1.get("radius", radius);
    pp1.get("live", live);
    pp1.getarr("endpoint1", v, 0, SpaceDim);
    e1 = RealVect(D_DECL(v[0], v[1], v[2]));
    pp1.getarr("endpoint2", v, 0, SpaceDim);
    e2 = RealVect(D_DECL(v[0], v[1], v[2]));

    RefCountedPtr<BaseIF> rod1 = RefCountedPtr<BaseIF>(new Tesselation(e1, e2, radius, false));

    m_electrodes.push_back(Electrode(rod1, live));
    */
  }

  if (use_stl2) {
    // TODO
    /*
    pp2.get("radius", radius);
    pp2.get("live", live);
    pp2.getarr("endpoint1", v, 0, SpaceDim);
    e1 = RealVect(D_DECL(v[0], v[1], v[2]));
    pp2.getarr("endpoint2", v, 0, SpaceDim);
    e2 = RealVect(D_DECL(v[0], v[1], v[2]));

    RefCountedPtr<BaseIF> stl2 = RefCountedPtr<BaseIF>(new Tesselation(e1, e2, radius, false));

    m_electrodes.push_back(Electrode(stl2, live));
    */
  }
}

DoubleStl::~DoubleStl()
{}

#include <CD_NamespaceFooter.H>
