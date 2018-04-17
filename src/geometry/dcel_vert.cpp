/*!
  @file   dcel_vert.H
  @brief  Declaration of a dcel_edge class for describing surface tesselations
  @author Robert Marskar
  @date   Apr. 2018
*/

#include "dcel_vert.H"
#include "dcel_edge.H"
#include "dcel_poly.H"
#include "dcel_iterator.H"

dcel_vert::dcel_vert(){
  m_pos    = RealVect::Zero;
  m_normal = RealVect::Zero;

  m_polycache.resize(0);
}

dcel_vert::dcel_vert(const RealVect a_pos){
  m_pos    = a_pos;
  m_normal = RealVect::Zero;
}

dcel_vert::~dcel_vert(){

}

void dcel_vert::define(const RealVect a_pos, const RefCountedPtr<dcel_edge>& a_edge, const RealVect a_normal){
  this->set_pos(a_pos);
  this->set_edge(a_edge);
  this->set_normal(a_normal);
}


void dcel_vert::set_pos(const RealVect a_pos){
  m_pos = a_pos;
}


void dcel_vert::set_edge(const RefCountedPtr<dcel_edge>& a_edge){
  m_edge = a_edge;
}

void dcel_vert::set_normal(const RealVect a_normal){
  m_normal = a_normal;
}

void dcel_vert::add_polygon(const RefCountedPtr<dcel_poly>& a_poly){
  m_polycache.push_back(a_poly);
}

Vector<RefCountedPtr<dcel_poly> > dcel_vert::get_polygons() {
  Vector<RefCountedPtr<dcel_poly> > polygons;
  for (edge_iterator iter(*this); iter.ok(); ++iter){
    polygons.push_back(iter()->get_poly());
  }

  //  MayDay::Abort("dcel_vert::get_polygons - this routine has crashed in the past and probably contains some bug.");

  return polygons;
}

const Vector<RefCountedPtr<dcel_poly> >& dcel_vert::get_polycache() const{
  return m_polycache;
}

Vector<RefCountedPtr<dcel_poly> >& dcel_vert::get_polycache(){
  return m_polycache;
}

const RefCountedPtr<dcel_edge>& dcel_vert::get_edge() const{
  return m_edge;
}

RefCountedPtr<dcel_edge>& dcel_vert::get_edge(){
  return m_edge;
}

RealVect dcel_vert::get_pos() const {
  return m_pos;
}

RealVect dcel_vert::get_normal() const {
  return m_normal;
}
  
Real dcel_vert::signed_distance(const RealVect a_x0) const {
  return 0.0;
}
