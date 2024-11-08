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

using T = float;
using RIFT = EBGeometry::ReflectIF<T>;

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
    auto stlIF = EBGeometry::Parser::readIntoLinearBVH<T>(filename);
    // std::shared_ptr<RIFT> reflectIF(new RIFT(implicitFunction, 0)); // 0 = yz plane
    RefCountedPtr<BaseIF> baseIF = RefCountedPtr<BaseIF>(
        new EBGeometryIF<T>(stlIF, flipInside, zCoord));
        // new EBGeometryIF<T>(reflectIF, flipInside, zCoord));
    m_electrodes.push_back(Electrode(baseIF, live));
  }

  if (use_stl2) {
    pp2.get("mesh_file", filename);
    pp2.get("z_coord", zCoord);
    pp2.get("flip_inside", flipInside);
    pp2.get("live", live);

    // Read the PLY file and put it in a linearized BVH hierarchy.
    auto stlIF = EBGeometry::Parser::readIntoLinearBVH<T>(filename);
    // std::shared_ptr<RIFT> reflectIF(new RIFT(implicitFunction, 0)); // 0 = yz plane
    RefCountedPtr<BaseIF> baseIF = RefCountedPtr<BaseIF>(
        new EBGeometryIF<T>(stlIF, flipInside, zCoord));
        // new EBGeometryIF<T>(reflectIF, flipInside, zCoord));
    m_electrodes.push_back(Electrode(baseIF, live));

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
