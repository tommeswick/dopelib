/**
*
* Copyright (C) 2012 by the DOpElib authors
*
* This file is part of DOpElib
*
* DOpElib is free software: you can redistribute it
* and/or modify it under the terms of the GNU General Public
* License as published by the Free Software Foundation, either
* version 3 of the License, or (at your option) any later
* version.
*
* DOpElib is distributed in the hope that it will be
* useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE.  See the GNU General Public License for more
* details.
*
* Please refer to the file LICENSE.TXT included in this distribution
* for further information on this license.
*
**/

#ifndef _TANGENT_DIRICHLET_DATA_H_
#define _TANGENT_DIRICHLET_DATA_H_

#include "function_wrapper.h"

namespace DOpE
{

  /**
   * This class is used to extract the Dirichlet Data for the Tangent Problem
   *
   * @tparam  DD              The Dirichlet Data Object under consideration
   * @tparam  VECTOR          The Vector type
   */
  template<typename DD, typename VECTOR, int dealdim>
    class TangentDirichletData : public DOpEWrapper::Function<dealdim>
  {
  public:
  TangentDirichletData(const DD& data) : DOpEWrapper::Function<dealdim>(data.n_components(), data.InitialTime()), _dirichlet_data(data)
    {
      _param_values = NULL;
      _domain_values = NULL;
      _color = 0;
    }

    /**
     * Initializes the private data, should be called prior to any value call!
     */
    void ReInit(
		const std::map<std::string, const dealii::Vector<double>* > &param_values,
		const std::map<std::string, const VECTOR* > &domain_values,
		unsigned int color)
    {
      _param_values = &param_values;
      _domain_values = &domain_values;
      _color = color;
    }


    /**
     * Accesses the values of the dirichlet data for the tangent problem.
     * This is given by the first derivative of the control-to-dirichlet-values
     *
     * @param p         The point (on the boundary of the domain) where the Dirichlet
     *                  values are evaluated
     * @param component The component of the Dirichlet data
     *
     * @return The component of the Dirichlet data at p, i.e., DD'(q)(p)_{component} 
     */
    double value (const dealii::Point<dealdim>   &p,
		  const unsigned int  component) const
    {
      return _dirichlet_data.Data_Q(
	_param_values,
	_domain_values,
	_color,
	p,
	component);
    }

    /**
     * This Function is used to transfer the current time to the dirichlet data if needed this should be stored.
     *
     * @param time      The current time
     */
    void SetTime(double time) const
    {
      _dirichlet_data.SetTime(time);
    }
  private:
    const DD& _dirichlet_data;
    const std::map<std::string, const dealii::Vector<double>* >* _param_values;
    const std::map<std::string, const VECTOR* >* _domain_values;
    unsigned int _color;
  };
}
#endif
