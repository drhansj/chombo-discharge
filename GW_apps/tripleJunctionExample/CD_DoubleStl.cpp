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
  ParmParse pp3("DoubleStl.stl3");
  ParmParse ppC("DoubleStl.cylinder");
  ParmParse ppC2("DoubleStl.cylinder2");
  ParmParse ppC3("DoubleStl.cylinder3");

  // bool use_stl1;
  // bool use_stl2;
  // bool use_stl3;

  // pp1.get("on", use_stl1);
  // pp2.get("on", use_stl2);
  // pp3.get("on", use_stl3);

  std::string filename;
  Real zCoord;
  bool flipInside;
  bool live;

  // if (use_stl1) {
  //   pp1.get("mesh_file", filename);
  //   pp1.get("z_coord", zCoord);
  //   pp1.get("flip_inside", flipInside);
  //   pp1.get("live", live);
  //   Real x_offset = 0.;
  //   pp1.query("x_offset", x_offset);
  //   Real scale = 1.;
  //   pp1.query("scale", scale);

  //   // Read the PLY file and put it in a linearized BVH hierarchy.
  //   auto stlIF = EBGeometry::Parser::readIntoLinearBVH<T>(filename);
  //   BaseIF* baseStlIF = new EBGeometryIF<T>(stlIF, flipInside, zCoord);
  //   TransformIF* transIF = new TransformIF(*baseStlIF);
  //   RealVect offset = RealVect::Zero;
  //   offset[0] = x_offset;
  //   transIF->translate(offset);
  //   transIF->scale(scale);
  //   RefCountedPtr<BaseIF> baseIF = RefCountedPtr<BaseIF>(transIF);
  //       // new EBGeometryIF<T>(reflectIF, flipInside, zCoord));
  //   m_electrodes.push_back(Electrode(baseIF, live));
  // }

  // if (use_stl2) {
  //   pp2.get("mesh_file", filename);
  //   pp2.get("z_coord", zCoord);
  //   pp2.get("flip_inside", flipInside);
  //   pp2.get("live", live);
  //   Real x_offset = 0.;
  //   pp2.query("x_offset", x_offset);
  //   Real scale = 1.;
  //   pp2.query("scale", scale);

  //   // Read the PLY file and put it in a linearized BVH hierarchy.
  //   auto stlIF = EBGeometry::Parser::readIntoLinearBVH<T>(filename);
  //   BaseIF* baseStlIF = new EBGeometryIF<T>(stlIF, flipInside, zCoord);
  //   TransformIF* transIF = new TransformIF(*baseStlIF);
  //   RealVect offset = RealVect::Zero;
  //   offset[0] = x_offset;
  //   transIF->translate(offset);
  //   transIF->scale(scale);
  //   RefCountedPtr<BaseIF> baseIF = RefCountedPtr<BaseIF>(transIF);
  //       // new EBGeometryIF<T>(reflectIF, flipInside, zCoord));
  //   m_electrodes.push_back(Electrode(baseIF, live));
  // }

  // if (use_stl3) {
  //   pp3.get("mesh_file", filename);
  //   pp3.get("z_coord", zCoord);
  //   pp3.get("flip_inside", flipInside);
  //   Real x_offset = 0.;
  //   pp3.query("x_offset", x_offset);
  //   Real eps;
  //   pp3.get("permittivity", eps);
  //   Real scale = 1.;
  //   pp3.query("scale", scale);
  //   // Read the PLY file and put it in a linearized BVH hierarchy.
  //   auto stlIF = EBGeometry::Parser::readIntoLinearBVH<T>(filename);
  //   BaseIF* baseStlIF = new EBGeometryIF<T>(stlIF, flipInside, zCoord);
  //   TransformIF* transIF = new TransformIF(*baseStlIF);
  //   RealVect offset = RealVect::Zero;
  //   offset[0] = x_offset;
  //   transIF->translate(offset);
  //   transIF->scale(scale);
  //   RefCountedPtr<BaseIF> baseIF = RefCountedPtr<BaseIF>(transIF);
  //       // new EBGeometryIF<T>(reflectIF, flipInside, zCoord));
  //   m_dielectrics.push_back(Dielectric(baseIF, eps));
  // }

  bool use_cyl = false;
  ppC.query("on", use_cyl);
  if (use_cyl){
    Real rad;
    ppC.get("radius", rad);
    Vector<Real> top, bot;
    ppC.getarr("top", top, 0, 3);
    ppC.getarr("bot", bot, 0, 3);
    bool live = false;
    ppC.query("live", live);
    using Vec3    = EBGeometry::Vec3T<Real>;
    auto buffer = std::make_shared<EBGeometry::CylinderSDF<Real>>(Vec3(top[0],top[1],top[2]), Vec3(bot[0],bot[1],bot[2]), rad);
    const auto baseifbuff = RefCountedPtr<BaseIF>(new EBGeometryIF<Real>(buffer, true));
    m_electrodes.push_back(Electrode(baseifbuff, live));
  }

  bool use_cyl2 = false;
  ppC2.query("on", use_cyl2);
  if (use_cyl2){
    Real rad;
    ppC2.get("radius", rad);
    Vector<Real> top, bot;
    ppC2.getarr("top", top, 0, 3);
    ppC2.getarr("bot", bot, 0, 3);
    bool live = false;
    ppC2.query("live", live);
    using Vec3    = EBGeometry::Vec3T<Real>;
    auto buffer = std::make_shared<EBGeometry::CylinderSDF<Real>>(Vec3(top[0],top[1],top[2]), Vec3(bot[0],bot[1],bot[2]), rad);
    const auto baseifbuff = RefCountedPtr<BaseIF>(new EBGeometryIF<Real>(buffer, true));
    m_electrodes.push_back(Electrode(baseifbuff, live));
  }

  bool use_cyl3 = false;
  ppC3.query("on", use_cyl3);
  if (use_cyl3){
    Real eps;
    ppC3.get("permittivity", eps);
    Real rad;
    ppC3.get("radius", rad);
    Vector<Real> top, bot;
    ppC3.getarr("top", top, 0, 3);
    ppC3.getarr("bot", bot, 0, 3);
    using Vec3    = EBGeometry::Vec3T<Real>;
    auto buffer = std::make_shared<EBGeometry::CylinderSDF<Real>>(Vec3(top[0],top[1],top[3]), Vec3(bot[0],bot[1],bot[3]), rad);
    const auto baseifbuff = RefCountedPtr<BaseIF>(new EBGeometryIF<Real>(buffer, true));
    m_dielectrics.push_back(Dielectric(baseifbuff, eps));
  }
}

DoubleStl::~DoubleStl()
{}

#include <CD_NamespaceFooter.H>
