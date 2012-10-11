#ifndef _LOCALPDE_
#define _LOCALPDE_

#include "pdeinterface.h"
#include "celldatacontainer.h"
#include "facedatacontainer.h"
#include "my_functions.h"

using namespace std;
using namespace dealii;
using namespace DOpE;

template<typename VECTOR, int dealdim>
  class LocalPDE : public PDEInterface<CellDataContainer, FaceDataContainer,
      dealii::DoFHandler<dealdim>, VECTOR, dealdim>
  {
    public:
      LocalPDE(unsigned int order)
          : _exact_solution(order), _state_block_components(1, 0)
      {
      }

      void
      CellEquation(
          const CellDataContainer<dealii::DoFHandler<dealdim>, VECTOR, dealdim>& cdc,
          dealii::Vector<double> &local_cell_vector, double scale, double)
      {
        unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
        unsigned int n_q_points = cdc.GetNQPoints();
        const DOpEWrapper::FEValues<dealdim> &state_fe_values =
            cdc.GetFEValuesState();

        assert(this->_problem_type == "state");

        _ugrads.resize(n_q_points, Tensor<1, dealdim>());
        cdc.GetGradsState("last_newton_solution", _ugrads);

        const FEValuesExtractors::Scalar velocities(0);

        for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
        {
          Tensor<1, 2> vgrads;
          vgrads.clear();
          vgrads[0] = _ugrads[q_point][0];
          vgrads[1] = _ugrads[q_point][1];

          for (unsigned int i = 0; i < n_dofs_per_cell; i++)
          {
            const Tensor<1, 2> phi_i_grads_v =
                state_fe_values[velocities].gradient(i, q_point);

            local_cell_vector(i) += scale * (vgrads * phi_i_grads_v)
                * state_fe_values.JxW(q_point);
          }
        }
      }

      void
      CellMatrix(
          const CellDataContainer<dealii::DoFHandler<dealdim>, VECTOR, dealdim>& cdc,
          FullMatrix<double> &local_entry_matrix, double scale, double)
      {
        unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
        unsigned int n_q_points = cdc.GetNQPoints();
        //unsigned int material_id = cdc.GetMaterialId();
        const DOpEWrapper::FEValues<dealdim> &state_fe_values =
            cdc.GetFEValuesState();

        const FEValuesExtractors::Scalar velocities(0);

        std::vector<Tensor<1, 2> > phi_grads_v(n_dofs_per_cell);

        for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
        {
          for (unsigned int k = 0; k < n_dofs_per_cell; k++)
          {
            phi_grads_v[k] = state_fe_values[velocities].gradient(k, q_point);
          }

          for (unsigned int i = 0; i < n_dofs_per_cell; i++)
          {
            for (unsigned int j = 0; j < n_dofs_per_cell; j++)
            {

              local_entry_matrix(i, j) += scale * phi_grads_v[j]
                  * phi_grads_v[i] * state_fe_values.JxW(q_point);
            }
          }
        }
      }

      void
      CellRightHandSide(
          const CellDataContainer<dealii::DoFHandler<dealdim>, VECTOR, dealdim>& cdc,
          dealii::Vector<double> &local_cell_vector, double scale)
      {
        assert(this->_problem_type == "state");
        unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
        unsigned int n_q_points = cdc.GetNQPoints();
        const DOpEWrapper::FEValues<dealdim> &state_fe_values =
            cdc.GetFEValuesState();

        _fvalues.resize(n_q_points);
        const FEValuesExtractors::Scalar velocities(0);

        for (unsigned int q_point = 0; q_point < n_q_points; ++q_point)
        {
          _fvalues[q_point] = -_exact_solution.laplacian(
              state_fe_values.quadrature_point(q_point));
          for (unsigned int i = 0; i < n_dofs_per_cell; i++)
          {
            local_cell_vector(i) += scale * _fvalues[q_point]
                * state_fe_values[velocities].value(i, q_point)
                * state_fe_values.JxW(q_point);
          }
        } //endfor qpoint
      }

      void
      BoundaryEquation(
          const FaceDataContainer<dealii::DoFHandler<2>, VECTOR, dealdim>& fdc,
          dealii::Vector<double> &local_cell_vector, double scale,
          double /*scale_ico*/)
      {

      }

      void
      BoundaryMatrix(
          const FaceDataContainer<dealii::DoFHandler<2>, VECTOR, dealdim>& fdc,
          dealii::FullMatrix<double> &local_entry_matrix, double /*scale*/,
          double /*scale_ico*/)
      {
      }

      void
      BoundaryRightHandSide(
          const FaceDataContainer<dealii::DoFHandler<2>, VECTOR, dealdim>&,
          dealii::Vector<double> &local_cell_vector __attribute__((unused)),
          double scale __attribute__((unused)))
      {
      }

      UpdateFlags
      GetUpdateFlags() const
      {
        return update_values | update_gradients | update_quadrature_points;
      }

      UpdateFlags
      GetFaceUpdateFlags() const
      {
        return update_values | update_gradients | update_normal_vectors
            | update_quadrature_points;
      }

      unsigned int
      GetStateNBlocks() const
      {
        return 1;
      }
      std::vector<unsigned int>&
      GetStateBlockComponent()
      {
        return _state_block_components;
      }
      const std::vector<unsigned int>&
      GetStateBlockComponent() const
      {
        return _state_block_components;
      }

    private:
      vector<double> _fvalues;

      vector<Tensor<1, dealdim> > _ugrads;

      ExactSolution _exact_solution;

       vector<unsigned int> _state_block_components;
  };
#endif
