/*!
  @file   dcel_poly.cpp
  @brief  Implementation of dcel_poly.H
  @author Robert Marskar
  @date   Apr. 2018
*/

#include "dcel_vert.H"
#include "dcel_edge.H"
#include "dcel_poly.H"
#include "dcel_iterator.H"

#include <PolyGeom.H>
  
#define EPSILON 1.E-8
#define TWOPI 6.283185307179586476925287

using namespace dcel;

polygon::polygon(){
  m_normal = RealVect::Zero;
}

polygon::~polygon(){

}


const std::shared_ptr<edge>& polygon::get_edge() const{
  return m_edge;
}


std::shared_ptr<edge>& polygon::get_edge(){
  return m_edge;
}


void polygon::define(const RealVect& a_normal, const std::shared_ptr<edge>& a_edge){
  this->set_normal(a_normal);
  this->set_edge(a_edge);
}


void polygon::set_edge(const std::shared_ptr<edge>& a_edge){
  m_edge = a_edge;
}


void polygon::set_normal(const RealVect& a_normal){
  m_normal = a_normal;
}


void polygon::normalize(){
  m_normal *= 1./m_normal.vectorLength();
}


void polygon::compute_area() {
  const std::vector<std::shared_ptr<vertex> > vertices = this->get_vertices();

  Real area = 0.0;

  for (int i = 0; i < vertices.size() - 1; i++){
    const RealVect v1 = vertices[i]->get_pos();
    const RealVect v2 = vertices[i+1]->get_pos();
    area += PolyGeom::dot(PolyGeom::cross(v2,v1), m_normal);
  }

  m_area = Abs(0.5*area);
}


void polygon::compute_centroid() {
  m_centroid = RealVect::Zero;
  const std::vector<std::shared_ptr<vertex> > vertices = this->get_vertices();

  for (int i = 0; i < vertices.size(); i++){
    m_centroid += vertices[i]->get_pos();
  }
  m_centroid = m_centroid/vertices.size();
}


void polygon::compute_normal(const bool a_outward_normal){
  
  // We assume that the normal is defined by right-hand rule where the rotation direction is along the half edges

  bool found_normal = false;
  std::vector<std::shared_ptr<vertex> > vertices = this->get_vertices();
  CH_assert(vertices.size() > 2);
  const int n = vertices.size();
  for (int i = 0; i < n; i++){
    const RealVect x0 = vertices[i]->get_pos();
    const RealVect x1 = vertices[(i+1)%n]->get_pos();
    const RealVect x2 = vertices[(i+2)%n]->get_pos();

    m_normal = PolyGeom::cross(x2-x1, x2-x0);
    if(m_normal.vectorLength() > 0.0){
      found_normal = true;
      break;
    }
  }

  
  if(!found_normal){
    pout() << "polygon::compute_normal - vertex vectors:" << endl;
    for (int i = 0; i < vertices.size(); i++){
      pout() << "\t" << vertices[i]->get_pos() << endl;
    }
    pout() << "polygon::compute_normal - From this I computed n = " << m_normal << endl;
    pout() << "polygon::compute_normal - Aborting..." << endl;
    MayDay::Warning("polygon::compute_normal - Cannot compute normal vector. The polygon is probably degenerate");
  }
  else{
    m_normal *= 1./m_normal.vectorLength();
  }

#if 0
  const std::shared_ptr<vertex>& v0 = m_edge->get_prev()->get_vert();
  const std::shared_ptr<vertex>& v1 = m_edge->get_vert();
  const std::shared_ptr<vertex>& v2 = m_edge->get_next()->get_vert();
  
  const RealVect x0 = v0->get_pos();
  const RealVect x1 = v1->get_pos();
  const RealVect x2 = v2->get_pos();
  
  m_normal = PolyGeom::cross(x2-x1,x1-x0);
  if(m_normal.vectorLength() < 1.E-40){
    MayDay::Abort("polygon::compute_normal - vertices lie on a line. Cannot compute normal vector");
  }
  else{
    m_normal = m_normal/m_normal.vectorLength();
  }
#endif

  if(!a_outward_normal){ // If normal points inwards, make it point outwards
    m_normal = -m_normal;
  }
}


void polygon::compute_bbox(){
  std::vector<std::shared_ptr<vertex> > vertices = this->get_vertices();
  std::vector<RealVect> coords;

  for (int i = 0; i < vertices.size(); i++){
    coords.push_back(vertices[i]->get_pos());
  }

  m_lo =  1.23456E89*RealVect::Unit;
  m_hi = -1.23456E89*RealVect::Unit;

  for (int i = 0; i < coords.size(); i++){
    for (int dir = 0; dir < SpaceDim; dir++){
      if(coords[i][dir] < m_lo[dir]){
	m_lo[dir] = coords[i][dir];
      }
      if(coords[i][dir] > m_hi[dir]){
	m_hi[dir] = coords[i][dir];
      }
    }
  }

  // Define ritter sphere
  m_ritter.define(coords);

#if 1 // Debug test
  for (int i = 0; i < vertices.size(); i++){
    const RealVect pos = vertices[i]->get_pos();
    for (int dir = 0; dir < SpaceDim; dir++){
      if(pos[dir] < m_lo[dir] || pos[dir] > m_hi[dir]){
	pout() << "pos = " << pos << "\t Lo = " << m_lo << "\t Hi = " << m_hi << endl;
	MayDay::Abort("polygon::compute_bbox - Box does not contain vertices");
      }
    }
  }
#endif

#if 0 // Disabled because I want a tight-fitting box
  Real widest = 0;
  for (int dir = 0; dir < SpaceDim; dir++){
    const Real cur = m_hi[dir] - m_lo[dir];
    widest = (cur > widest) ? cur : widest;
  }


  // Grow box by 5% in each direction
  m_hi += 5.E-2*widest*RealVect::Unit;
  m_lo -= 5.E-2*widest*RealVect::Unit;
#endif
}


Real polygon::get_area() const {
  return m_area;
}


Real polygon::signed_distance(const RealVect a_x0) {
#define bug_check 0
  Real retval = 1.234567E89;

  std::vector<std::shared_ptr<vertex> > vertices = this->get_vertices();

#if bug_check // Debug, return shortest distance to vertex
  CH_assert(vertices.size() > 0);
  Real min = 1.E99;
  for (int i = 0; i < vertices.size(); i++){
    const Real d = (a_x0 - vertices[i]->get_pos()).vectorLength();
    min = (d < min) ? d : min;
  }

  return min;
#endif

  // Compute projection of x0 on the polygon plane
  const RealVect x1 = vertices[0]->get_pos();
  const Real ncomp  = PolyGeom::dot(a_x0-x1, m_normal);
  const RealVect xp = a_x0 - ncomp*m_normal;


  // Use angle rule to check if projected point lies inside the polygon
  Real anglesum = 0.0;
  const int n = vertices.size();
  for(int i = 0; i < n; i++){
    const RealVect p1 = vertices[i]->get_pos() - xp;
    const RealVect p2 = vertices[(i+1)%n]->get_pos() - xp;

    const Real m1 = p1.vectorLength();
    const Real m2 = p2.vectorLength();

    if(m1*m2 <= EPSILON) {// Projected point hits a vertex, return early. 
      anglesum = 0.;
      break;
    }
    else {
      const Real cos_theta = PolyGeom::dot(p1, p2)/(m1*m2);
      anglesum += acos(cos_theta);
    }
  }

  // Projected point is inside if angles sum to 2*pi
  bool inside = false;
  if(Abs(Abs(anglesum) - TWOPI) < EPSILON){
    inside = true;
  }

  // If projection is inside, shortest distance is the normal component of the point
  if(inside){
#if bug_check
    CH_assert(Abs(ncomp) <= min);
#endif
    retval = ncomp;
  }
  else{ // The projected point lies outside the triangle. Check distance to edges/vertices
    const std::vector<std::shared_ptr<edge> > edges = this->get_edges();
    for (int i = 0; i < edges.size(); i++){
      const Real cur_dist = edges[i]->signed_distance(a_x0);
      if(Abs(cur_dist) < Abs(retval)){
	retval = cur_dist;
      }
    }
  }

  return retval;
}


RealVect polygon::get_normal() const {
  return m_normal;
}


RealVect polygon::get_centroid() const {
  return m_centroid;
}


RealVect polygon::get_coord() const {
  return m_centroid;
}


RealVect polygon::get_bbox_lo() const {
  return m_lo;
}


RealVect polygon::get_bbox_hi() const {
  return m_hi;
}


std::vector<RealVect> polygon::get_points(){
  std::vector<std::shared_ptr<vertex> > vertices = this->get_vertices();

  std::vector<RealVect> pos;
  for (int i = 0; i < vertices.size(); i++){
    pos.push_back(vertices[i]->get_pos());
  }

  return pos;
}


std::vector<std::shared_ptr<vertex> > polygon::get_vertices(){
  std::vector<std::shared_ptr<vertex> > vertices;

  for (edge_iterator iter(*this); iter.ok(); ++iter){
    std::shared_ptr<edge>& edge = iter();
    vertices.push_back(edge->get_vert());
  }

  return vertices;
}


std::vector<std::shared_ptr<edge> > polygon::get_edges(){
  std::vector<std::shared_ptr<edge> > edges;

  for (edge_iterator iter(*this); iter.ok(); ++iter){
    edges.push_back(iter());
  }

#if 1 // Debug test
  CH_assert(edges.size() == 3);
#endif

  return edges;
}

