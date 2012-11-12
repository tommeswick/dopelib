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

#ifndef _LOCALPDE_
#define _LOCALPDE_

#include "pdeinterface.h"
#include "celldatacontainer.h"
#include "facedatacontainer.h"


using namespace std;
using namespace dealii;
using namespace DOpE;

template<typename VECTOR, int dopedim, int dealdim>
class LocalPDE: public PDEInterface<CellDataContainer,FaceDataContainer,dealii::DoFHandler<dealdim>, VECTOR,dopedim, dealdim>
{
	public:

 LocalPDE():_state_block_components(1, 0)
    {

    }

		// Domain values for cells
  void CellEquation(const CellDataContainer<dealii::DoFHandler<dealdim>, VECTOR, dealdim>& cdc, dealii::Vector<double> &local_cell_vector,double scale, double /*scale_ico*/)
      {
	 assert(this->_problem_type == "state");
			
	 const DOpEWrapper::FEValues<dealdim> & state_fe_values = cdc.GetFEValuesState();
	 unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
	 unsigned int n_q_points = cdc.GetNQPoints();

	 _uvalues.resize(n_q_points);
	 _ugrads.resize(n_q_points);

	 cdc.GetValuesState("last_newton_solution", _uvalues);
	 cdc.GetGradsState("last_newton_solution", _ugrads);


	 for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
	    {

  	     for (unsigned int i = 0; i < n_dofs_per_cell; i++)
		{
		  const Tensor<1, dealdim> phi_i_grads = state_fe_values.shape_grad(i, q_point);

		  local_cell_vector(i) += scale * (_ugrads[q_point]*phi_i_grads) * state_fe_values.JxW(q_point);

		  }
	     }
	}


  void CellMatrix(const CellDataContainer<dealii::DoFHandler<dealdim>, VECTOR, dealdim>& cdc, FullMatrix<double> &local_entry_matrix, double scale, double)
      {
	const DOpEWrapper::FEValues<dealdim> & state_fe_values = cdc.GetFEValuesState();
        unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
	unsigned int n_q_points = cdc.GetNQPoints();


		  
	std::vector<Tensor<1, dealdim> > phi_grads(n_dofs_per_cell);
		  
	for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
	    {
	      for (unsigned int k = 0; k < n_dofs_per_cell; k++)
		  {
		     phi_grads[k] = state_fe_values.shape_grad(k, q_point);
		  }

	      for (unsigned int i = 0; i < n_dofs_per_cell; i++)
		  {
		    for (unsigned int j = 0; j < n_dofs_per_cell; j++)
			{
			  local_entry_matrix(i, j) += scale * (phi_grads[j] * phi_grads[i]) * state_fe_values.JxW(q_point);
			}
		    }
		}
	   }


  void CellRightHandSide(const CellDataContainer<dealii::DoFHandler<dealdim>, VECTOR, dealdim>& /*cdc*/,
			 dealii::Vector<double> &local_cell_vector __attribute__((unused)),
			 double scale __attribute__((unused)))
  {
    assert(this->_problem_type == "state");
    
  }

  void CellTimeEquationExplicit(const CellDataContainer<dealii::DoFHandler<dealdim>, VECTOR, dealdim>& /*cdc*/,
				dealii::Vector<double> &local_cell_vector __attribute__((unused)),
				double scale __attribute__((unused)))
  {
    assert(this->_problem_type == "state");
  }

		void CellTimeEquation(const CellDataContainer<dealii::DoFHandler<dealdim>, VECTOR, dealdim>& cdc,
                           dealii::Vector<double> &local_cell_vector __attribute__((unused)),
		       double scale __attribute__((unused)))
		{
			assert(this->_problem_type == "state");

			const DOpEWrapper::FEValues<dealdim> & state_fe_values = cdc.GetFEValuesState();
			unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
			unsigned int n_q_points = cdc.GetNQPoints();

			_uvalues.resize(n_q_points);

			cdc.GetValuesState("last_newton_solution", _uvalues);

			for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
			{

				for (unsigned int i = 0; i < n_dofs_per_cell; i++)
				{
					const double phi_i = state_fe_values.shape_value(i, q_point);
					local_cell_vector(i) += scale * (_uvalues[q_point] * phi_i)
					* state_fe_values.JxW(q_point);
				}
			}
		}

		void CellTimeMatrixExplicit(const CellDataContainer<dealii::DoFHandler<dealdim>, VECTOR, dealdim>& /*cdc*/,
					    FullMatrix<double> &/*local_entry_matrix*/)
		{
			assert(this->_problem_type == "state");
		}

		void CellTimeMatrix(const CellDataContainer<dealii::DoFHandler<dealdim>, VECTOR, dealdim>& cdc,
			      FullMatrix<double> &local_entry_matrix)
		{
			assert(this->_problem_type == "state");

			const DOpEWrapper::FEValues<dealdim> & state_fe_values = cdc.GetFEValuesState();
			unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
			unsigned int n_q_points = cdc.GetNQPoints();


			std::vector<double> phi(n_dofs_per_cell);

			for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
			{
				for (unsigned int k = 0; k < n_dofs_per_cell; k++)
				{
					phi[k] = state_fe_values.shape_value(k, q_point);
				}

				for (unsigned int i = 0; i < n_dofs_per_cell; i++)
				{
					for (unsigned int j = 0; j < n_dofs_per_cell; j++)
					{
						local_entry_matrix(j, i) += (phi[i] * phi[j]) * state_fe_values.JxW(q_point);
					}
				}
			}

		}


		UpdateFlags GetUpdateFlags() const
		{
			if (this->_problem_type == "state")
      				return update_values | update_gradients | update_quadrature_points;
			else
				throw DOpEException("Unknown Problem Type " + this->_problem_type,
				                    "LocalPDE::GetUpdateFlags");
		}

		UpdateFlags GetFaceUpdateFlags() const
		{
			if (this->_problem_type == "state")
				return update_values | update_gradients | update_normal_vectors | update_quadrature_points;
			else
				throw DOpEException("Unknown Problem Type " + this->_problem_type,
				                    "LocalPDE::GetUpdateFlags");
		}

		unsigned int GetControlNBlocks() const
		{
			return 1;
		}

		unsigned int GetStateNBlocks() const
		{
			return 1;
		}

		std::vector<unsigned int>& GetControlBlockComponent()
		{
			return _block_components;
		}
		const std::vector<unsigned int>& GetControlBlockComponent() const
		{
			return _block_components;
		}
		std::vector<unsigned int>& GetStateBlockComponent()
		{
			return _state_block_components;
		}
		const std::vector<unsigned int>& GetStateBlockComponent() const
		{
			return _state_block_components;
		}

		
 private:
		vector<double> _fvalues;
		vector<double> _uvalues;
		mutable double _my_time;

		vector<Tensor<1, dealdim> > _ugrads;


		vector<unsigned int> _state_block_components;
		vector<unsigned int> _block_components;
		



};
#endif
