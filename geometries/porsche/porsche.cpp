/*!
  @file   porsche.cpp
  @brief  Implementation of porsche.H
  @author Robert Marskar
  @date   Jan. 2019
*/

#include "porsche.H"

#include <ParmParse.H>

#include "dcel_BoundingVolumes.H"
#include "dcel_mesh.H"
#include "dcel_parser.H"
#include "dcel_if.H"

using namespace dcel;

using mesh = meshT<double>;
using AABB = AABBT<double>;
using Sphere = BoundingSphereT<double>;

porsche::porsche(){

  std::string filename;

  ParmParse pp("porsche");

  pp.get("mesh_file", filename);

  // Build the mesh and set default parameters. 
  auto m = std::make_shared<mesh>();

  // Read mesh from file and reconcile. 
  parser::PLY<double>::readASCII(*m, filename);
  m->sanityCheck();
  m->reconcile(VertexNormalWeight::Angle);
  m->setSearchAlgorithm(SearchAlgorithm::Direct2);
  m->setInsideOutsideAlgorithm(InsideOutsideAlgorithm::CrossingNumber);


  // Creat the object and build the BVH. 
  RefCountedPtr<dcel_if<double, AABB> > bif = RefCountedPtr<dcel_if<double, AABB> > (new dcel_if<double, AABB>(m,true));

  bif->buildBVH();

  m_electrodes.push_back(electrode(bif, true));
  
}

porsche::~porsche(){
  
}
