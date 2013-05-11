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

#ifndef _LOCALFunctional_
#define _LOCALFunctional_

#include "pdeinterface.h"

using namespace std;
using namespace dealii;
using namespace DOpE;

template<
    template<template<int, int> class DH, typename VECTOR, int dealdim> class CDC,
    template<template<int, int> class DH, typename VECTOR, int dealdim> class FDC,
    template<int, int> class DH, typename VECTOR, int dopedim, int dealdim =
        dopedim>
  class LocalFunctional : public FunctionalInterface<CDC, FDC, DH, VECTOR,
      dopedim, dealdim>
  {
    public:

      static void
      declare_params(ParameterReader &param_reader)
      {
        param_reader.SetSubsection("Local PDE parameters");
        param_reader.declare_entry("density_fluid", "0.0", Patterns::Double(0));
        param_reader.declare_entry("viscosity", "0.0", Patterns::Double(0));
      }

      LocalFunctional(ParameterReader &param_reader)
      {
        // Control- and regulraization parameters
        mu_regularization = 1.0e+1;
        upper_bound_for_control_sum = 1.0e-2;

        // Fluid- and material parameters
        param_reader.SetSubsection("Local PDE parameters");
        _density_fluid = param_reader.get_double("density_fluid");
        _viscosity = param_reader.get_double("viscosity");
      }

      double
      BoundaryValue(const FaceDataContainer<DH, VECTOR, dealdim>& fdc)
      {
        const auto & state_fe_face_values = fdc.GetFEFaceValuesState();
        unsigned int n_q_points = fdc.GetNQPoints();
        unsigned int color = fdc.GetBoundaryIndicator();
        double functional_value_J = 0;

        Tensor<1, 2> drag_lift_value;
        drag_lift_value.clear();
        // Asking for boundary color of the cylinder
        if (color == 80)
        {
          _ufacevalues.resize(n_q_points, Vector<double>(3));
          _ufacegrads.resize(n_q_points, vector<Tensor<1, 2> >(3));

          fdc.GetFaceValuesState("state", _ufacevalues);
          fdc.GetFaceGradsState("state", _ufacegrads);

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            Tensor<2, 2> pI;
            pI[0][0] = _ufacevalues[q_point](2);
            pI[1][1] = _ufacevalues[q_point](2);

            Tensor<1, 2> v;
            v.clear();
            v[0] = _ufacevalues[q_point](0);
            v[1] = _ufacevalues[q_point](1);

            Tensor<2, 2> grad_v;
            grad_v[0][0] = _ufacegrads[q_point][0][0];
            grad_v[0][1] = _ufacegrads[q_point][0][1];
            grad_v[1][0] = _ufacegrads[q_point][1][0];
            grad_v[1][1] = _ufacegrads[q_point][1][1];

            // constitutive stress tensors for fluid
            Tensor<2, 2> cauchy_stress_fluid;
            cauchy_stress_fluid =
                500.0
                    * (-pI
                        + _density_fluid * _viscosity
                            * (grad_v + transpose(grad_v)));

            drag_lift_value -= cauchy_stress_fluid
                * state_fe_face_values.normal_vector(q_point)
                * state_fe_face_values.JxW(q_point);
          }

        }
        functional_value_J = drag_lift_value[0];

        // Regularization term for the cost functional
        // defined above
        if (color == 50)
        {
          // Regularization
          _qvalues.reinit(2);
          fdc.GetParamValues("control", _qvalues);

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            functional_value_J += mu_regularization * 0.5
                * (_qvalues(0) * _qvalues(0))
                * state_fe_face_values.JxW(q_point);
          }

        }
        if (color == 51)
        {
          // Regularization
          _qvalues.reinit(2);
          fdc.GetParamValues("control", _qvalues);

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            functional_value_J += mu_regularization * 0.5
                * (_qvalues(1) * _qvalues(1))
                * state_fe_face_values.JxW(q_point);
          }

        }
        return functional_value_J;

      }

      void
      BoundaryValue_U(const FaceDataContainer<DH, VECTOR, dealdim>& fdc,
          dealii::Vector<double> &local_cell_vector, double scale)
      {
        const auto & state_fe_face_values = fdc.GetFEFaceValuesState();
        unsigned int n_dofs_per_cell = fdc.GetNDoFsPerCell();
        unsigned int n_q_points = fdc.GetNQPoints();
        unsigned int color = fdc.GetBoundaryIndicator();
        if (color == 80)
        {
          const FEValuesExtractors::Vector velocities(0);
          const FEValuesExtractors::Scalar pressure(2);

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            for (unsigned int j = 0; j < n_dofs_per_cell; j++)
            {
              const Tensor<2, 2> phi_j_grads_v =
                  state_fe_face_values[velocities].gradient(j, q_point);
              const double phi_j_p = state_fe_face_values[pressure].value(j,
                  q_point);
              Tensor<2, 2> pI_LinP;
              pI_LinP[0][0] = phi_j_p;
              pI_LinP[0][1] = 0.0;
              pI_LinP[1][0] = 0.0;
              pI_LinP[1][1] = phi_j_p;

              Tensor<2, 2> cauchy_stress_fluid;
              cauchy_stress_fluid = -pI_LinP
                  + _density_fluid * _viscosity
                      * (phi_j_grads_v + transpose(phi_j_grads_v));

              Tensor<1, 2> neumann_value = cauchy_stress_fluid
                  * state_fe_face_values.normal_vector(q_point);

              local_cell_vector(j) -= scale * neumann_value[0] * 250
                  * state_fe_face_values.JxW(q_point);
            }
          }

        }
      }

      void
      BoundaryValue_Q(const FaceDataContainer<DH, VECTOR, dealdim>& fdc,
          dealii::Vector<double> &local_cell_vector, double scale)
      {
        const auto & state_fe_face_values = fdc.GetFEFaceValuesState();
        unsigned int n_dofs_per_cell = local_cell_vector.size();
        unsigned int n_q_points = fdc.GetNQPoints();
        unsigned int color = fdc.GetBoundaryIndicator();

        if (color == 50)
        {
          // Regularization
          _qvalues.reinit(2);
          fdc.GetParamValues("control", _qvalues);

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            for (unsigned int j = 0; j < n_dofs_per_cell; j++)
            {
              local_cell_vector(j) += scale * mu_regularization * (_qvalues(j))
                  * state_fe_face_values.JxW(q_point);
            }
          }
        }
        if (color == 51)
        {
          // Regularization
          _qvalues.reinit(2);
          fdc.GetParamValues("control", _qvalues);

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            for (unsigned int j = 0; j < n_dofs_per_cell; j++)
            {
              local_cell_vector(j) += scale * mu_regularization * (_qvalues(j))
                  * state_fe_face_values.JxW(q_point);
            }
          }
        }
      }

      void
      BoundaryValue_QQ(const FaceDataContainer<DH, VECTOR, dealdim>& fdc,
          dealii::Vector<double> &local_cell_vector, double scale)
      {
        const auto & state_fe_face_values = fdc.GetFEFaceValuesState();
        unsigned int n_dofs_per_cell = local_cell_vector.size();
        unsigned int n_q_points = fdc.GetNQPoints();
        unsigned int color = fdc.GetBoundaryIndicator();

        if (color == 50)
        {
          // Regularization
          _dqvalues.reinit(2);
          fdc.GetParamValues("dq", _dqvalues);

          _qvalues.reinit(2);
          fdc.GetParamValues("control", _qvalues);

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            for (unsigned int j = 0; j < n_dofs_per_cell; j++)
            {
              local_cell_vector(j) += scale * mu_regularization * (_dqvalues(j))
                  * state_fe_face_values.JxW(q_point);
            }
          }
        }
        if (color == 51)
        {
          // Regularization
          _dqvalues.reinit(2);
          fdc.GetParamValues("dq", _dqvalues);

          _qvalues.reinit(2);
          fdc.GetParamValues("control", _qvalues);

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            for (unsigned int j = 0; j < n_dofs_per_cell; j++)
            {
              local_cell_vector(j) += scale * mu_regularization * (_dqvalues(j))
                  * state_fe_face_values.JxW(q_point);
            }
          }
        }
      }

      void
      BoundaryValue_UU(const FaceDataContainer<DH, VECTOR, dealdim>& /*fdc*/,
          dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
      {

      }

      void
      BoundaryValue_QU(const FaceDataContainer<DH, VECTOR, dealdim>& /*fdc*/,
          dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
      {

      }

      void
      BoundaryValue_UQ(const FaceDataContainer<DH, VECTOR, dealdim>& /*fdc*/,
          dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
      {

      }

      double
      Value(const CellDataContainer<DH, VECTOR, dealdim>& /*cdc*/)
      {
        return 0.;
      }

      void
      Value_U(const CellDataContainer<DH, VECTOR, dealdim>& /*cdc*/,
          dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
      {

      }

      void
      Value_Q(const CellDataContainer<DH, VECTOR, dealdim>& /*cdc*/,
          dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
      {

      }

      void
      Value_UU(const CellDataContainer<DH, VECTOR, dealdim>& /*cdc*/,
          dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
      {

      }

      void
      Value_QU(const CellDataContainer<DH, VECTOR, dealdim>& /*cdc*/,
          dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
      {
      }

      void
      Value_UQ(const CellDataContainer<DH, VECTOR, dealdim>& /*cdc*/,
          dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
      {
      }

      void
      Value_QQ(const CellDataContainer<DH, VECTOR, dealdim>& /*cdc*/,
          dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
      {

      }

      UpdateFlags
      GetFaceUpdateFlags() const
      {
        return update_values | update_quadrature_points | update_gradients
            | update_normal_vectors;
      }

      string
      GetType() const
      {
        return "boundary";
      }

      string
      GetName() const
      {
        return "cost functional";
      }

    private:
      Vector<double> _qvalues;
      Vector<double> _dqvalues;
      vector<Vector<double> > _ufacevalues;
      vector<Vector<double> > _dufacevalues;

      vector<vector<Tensor<1, dealdim> > > _ufacegrads;
      vector<vector<Tensor<1, dealdim> > > _dufacegrads;


      // Fluid- and material parameters
      double _density_fluid, _viscosity;

      // Control- and regularization parameters
      double mu_regularization;
      double upper_bound_for_control_sum;

  };
#endif
