/*!
  @file   dcel_edge.cpp
  @brief  Implementation of dcel_edge.H
  @author Robert Marskar
  @date   Apr. 2018
*/

#include "dcel_vert.H"
#include "dcel_edge.H"
#include "dcel_poly.H"
#include "dcel_iterator.H"
#include "PolyGeom.H"

dcel_edge::dcel_edge(){

}

dcel_edge::~dcel_edge(){

}

void dcel_edge::define(const RefCountedPtr<dcel_vert>& a_vert,
		       const RefCountedPtr<dcel_edge>& a_pair,
		       const RefCountedPtr<dcel_edge>& a_next,
		       const RefCountedPtr<dcel_edge>& a_prev,
		       const RealVect                  a_normal){
  this->set_vert(a_vert);
  this->set_pair(a_pair);
  this->set_next(a_next);
  this->set_prev(a_prev);
  this->set_normal(a_normal);
}

void dcel_edge::set_poly(const RefCountedPtr<dcel_poly>& a_poly){
  m_poly = a_poly;
}

void dcel_edge::set_vert(const RefCountedPtr<dcel_vert>& a_vert){
  m_vert = a_vert;
}

void dcel_edge::set_pair(const RefCountedPtr<dcel_edge>& a_pair){
  m_pair = a_pair;
}

void dcel_edge::set_next(const RefCountedPtr<dcel_edge>& a_next){
  m_next = a_next;
}

void dcel_edge::set_prev(const RefCountedPtr<dcel_edge>& a_prev){
  m_prev = a_prev;
}

void dcel_edge::set_normal(const RealVect a_normal){
  m_normal = a_normal;
}

const RefCountedPtr<dcel_vert>& dcel_edge::get_vert() const {
  return m_vert;
}

RefCountedPtr<dcel_vert>& dcel_edge::get_vert() {
  return m_vert;
}

const RefCountedPtr<dcel_vert>& dcel_edge::get_other_vert() const {
  return m_pair->get_vert();
}

RefCountedPtr<dcel_vert>& dcel_edge::get_other_vert(){
  return m_pair->get_vert();
}

const RefCountedPtr<dcel_edge>& dcel_edge::get_pair() const {
  return m_pair;
}

RefCountedPtr<dcel_edge>& dcel_edge::get_pair(){
  return m_pair;
}

const RefCountedPtr<dcel_edge>& dcel_edge::get_prev() const {
  return m_prev;
}

RefCountedPtr<dcel_edge>& dcel_edge::get_prev() {
  return m_prev;
}

const RefCountedPtr<dcel_edge>& dcel_edge::get_next() const {
  return m_next;
}

RefCountedPtr<dcel_edge>& dcel_edge::get_next(){
  return m_next;
}

const RefCountedPtr<dcel_poly>& dcel_edge::get_poly() const{
  return m_poly;
}

RefCountedPtr<dcel_poly>& dcel_edge::get_poly(){
  return m_poly;
}

RealVect dcel_edge::get_normal() const {
  return m_normal;
}

Real dcel_edge::signed_distance(const RealVect a_x0) const {
  Real retval = 1.234567E89;
  
  // Involved vertices
  //  const RealVect x1 = m_vert->get_pos();
  //  const RealVect x2 = this->get_other_vert()->get_pos();
  const RealVect x1 = this->get_other_vert()->get_pos();
  const RealVect x2 = m_vert->get_pos();

  const RealVect R  = PolyGeom::cross(x2-x1,PolyGeom::cross(x1-a_x0, x2-a_x0));
  const RealVect r  = R/R.vectorLength();
  const RealVect xp = a_x0 + PolyGeom::dot(x1-a_x0,r)*r;
  const Real t      = PolyGeom::dot(xp-x1,x2-x1)/PolyGeom::dot(x2-x1, x2-x1);

  if(PolyGeom::cross(xp-x1, x2-x1).vectorLength() > 1.E-8){
    MayDay::Abort("stop");
  }

  RealVect p;
  RealVect n;
  if(t < 0.0){ // Closest to x1, vertex normal takes precedence.
    p = x1;
    n = this->get_other_vert()->get_normal();
  }
  else if (t > 1.0){ // Closest to x2, vertex normal takes precedence
    p = x2;
    n = m_vert->get_normal();
  }
  else{ // Projection onto line lies on the line segment
    p = xp;
    n = m_normal;
  }


  const Real dot = PolyGeom::dot(n, (a_x0 - p)); // Determine sign from projection. If the point is orthogonal to the normal,
  const int sgn = dot >= 0.0 ? 1 : -1;            // it must (I think) be outside

  //  CH_assert(n.vectorLength() > 0.0);

  retval = (a_x0-p).vectorLength()*sgn;

  return retval;
}
