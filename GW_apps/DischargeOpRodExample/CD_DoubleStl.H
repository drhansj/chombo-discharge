/* chombo-discharge
 * Copyright © 2021 SINTEF Energy Research.
 * Please refer to Copyright.txt and LICENSE in the chombo-discharge root directory.
 */

/*!
  @file   CD_DoubleStl.H
  @brief  Double STL geometry - defined from one or two files.
  @author Hans Johansen
*/

#ifndef CD_DoubleStl_H
#define CD_DoubleStl_H

// Our includes
#include <CD_ComputationalGeometry.H>
#include <CD_NamespaceHeader.H>

/*!
  @brief This is a class that defines two electrodes from STLs.
*/
class DoubleStl : public ComputationalGeometry
{
public:
  DoubleStl();
  ~DoubleStl();
};

#include <CD_NamespaceFooter.H>

#endif
