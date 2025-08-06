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
#include <TransformIF.H>

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
    Real x_offset = 0.;
    pp1.query("x_offset", x_offset);
    Real scale = 1.;
    pp1.query("scale", scale);

    // Read the PLY file and put it in a linearized BVH hierarchy.
    auto stlIF = EBGeometry::Parser::readIntoLinearBVH<T>(filename);
    BaseIF* baseStlIF = new EBGeometryIF<T>(stlIF, flipInside, zCoord);
    TransformIF* transIF = new TransformIF(*baseStlIF);
    RealVect offset = RealVect::Zero;
    offset[0] = x_offset;
    transIF->translate(offset);
    transIF->scale(scale);
    RefCountedPtr<BaseIF> baseIF = RefCountedPtr<BaseIF>(transIF);
        // new EBGeometryIF<T>(reflectIF, flipInside, zCoord));
    m_electrodes.push_back(Electrode(baseIF, live));
  }

  if (use_stl2) {
    pp2.get("mesh_file", filename);
    pp2.get("z_coord", zCoord);
    pp2.get("flip_inside", flipInside);
    pp2.get("live", live);
    Real x_offset = 0.;
    pp2.query("x_offset", x_offset);
    Real scale = 1.;
    pp2.query("scale", scale);

    // Read the PLY file and put it in a linearized BVH hierarchy.
    auto stlIF = EBGeometry::Parser::readIntoLinearBVH<T>(filename);
    BaseIF* baseStlIF = new EBGeometryIF<T>(stlIF, flipInside, zCoord);
    TransformIF* transIF = new TransformIF(*baseStlIF);
    RealVect offset = RealVect::Zero;
    offset[0] = x_offset;
    transIF->translate(offset);
    transIF->scale(scale);
    RefCountedPtr<BaseIF> baseIF = RefCountedPtr<BaseIF>(transIF);
        // new EBGeometryIF<T>(reflectIF, flipInside, zCoord));
    m_electrodes.push_back(Electrode(baseIF, live));
  }
}

DoubleStl::~DoubleStl()
{}

#include <CD_NamespaceFooter.H>
