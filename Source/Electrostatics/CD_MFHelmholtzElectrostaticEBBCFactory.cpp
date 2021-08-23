/* chombo-discharge
 * Copyright © 2021 SINTEF Energy Research.
 * Please refer to Copyright.txt and LICENSE in the chombo-discharge root directory.
 */

/*!
  @file   CD_MFHelmholtzElectrostaticEBBCFactory.cpp
  @brief  Implementation of CD_MFHelmholtzElectrostaticEBBCFactory.H
  @author Robert Marskar
*/

// Our includes
#include <CD_MFHelmholtzElectrostaticEBBCFactory.H>
#include <CD_MFHelmholtzElectrostaticEBBC.H>
#include <CD_NamespaceHeader.H>

MFHelmholtzElectrostaticEBBCFactory::MFHelmholtzElectrostaticEBBCFactory(const int a_order, const int a_weight, const ElectrostaticEbBc& a_electrostaticBCs){
  CH_TIME("MFHelmholtzElectrostaticEBBCFactory::MFHelmholtzElectrostaticEBBCFactory(int, int, ElectrostaticEbBc)");

  CH_assert(m_order  >  0);
  CH_assert(m_weight >= 0);
  
  m_order            = a_order;
  m_weight           = a_weight;  
  m_electrostaticBCs = a_electrostaticBCs;
}

MFHelmholtzElectrostaticEBBCFactory::~MFHelmholtzElectrostaticEBBCFactory(){
  CH_TIME("MFHelmholtzElectrostaticEBBCFactory::~MFHelmholtzElectrostaticEBBCFactory()");
}
  
RefCountedPtr<EBHelmholtzEBBC> MFHelmholtzElectrostaticEBBCFactory::create(const int a_iphase, const RefCountedPtr<JumpBC>& a_jumpBC) const {
  CH_TIME("MFHelmholtzElectrostaticEBBCFactory::create(int, RefCountedPtr<JumpBC>)");
  
  CH_assert(m_order  >  0);
  CH_assert(m_weight >= 0);
  
  auto bc = new MFHelmholtzElectrostaticEBBC(a_iphase, m_electrostaticBCs, a_jumpBC);

  bc->setOrder(m_order);
  bc->setWeight(m_weight);

  return RefCountedPtr<EBHelmholtzEBBC> (bc);
}

#include <CD_NamespaceFooter.H>
