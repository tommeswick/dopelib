#ifndef _LOCALPDE_
#define _LOCALPDE_

#include "pdeinterface.h"
#include "celldatacontainer.h"
#include "facedatacontainer.h"

using namespace std;
using namespace dealii;
using namespace DOpE;

template<typename VECTOR,int dealdim>
  class LocalPDE : public PDEInterface<CellDataContainer,FaceDataContainer,dealii::DoFHandler<dealdim>, VECTOR,dealdim>
  {
  public:
  LocalPDE() : _state_block_components(2,0)
      {
      }


    // Domain values for cells
    void CellEquation (const CellDataContainer<DoFHandler<dealdim>, VECTOR, dealdim>& cdc,
		       dealii::Vector<double> &local_cell_vector,
		       double scale, double)
    {
      assert(this->_problem_type == "state");

      const DOpEWrapper::FEValues<dealdim> & state_fe_values = cdc.GetFEValuesState();
      unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
      unsigned int n_q_points = cdc.GetNQPoints();

      _uvalues.resize(n_q_points,Vector<double>(2));
      _ugrads.resize(n_q_points,vector<Tensor<1,2> >(2));

      cdc.GetValuesState("last_newton_solution",_uvalues);
      cdc.GetGradsState("last_newton_solution",_ugrads);

      const FEValuesExtractors::Vector displacements (0);

      const double mu = 80193.800283;
      const double kappa = 271131.389455;
      const double lambda = 110743.788889;
      const double rho = 190937.589172;

      const double sigma = sqrt(2./3.)* 450.0;
      double norm = 0.;
      double factor = 0.;

      for (unsigned int q_point=0;q_point<n_q_points;q_point++)
	{
	  norm = 0.;
//          factor = 0.;

	  Tensor<2,2> vgrads;
	  vgrads.clear();
	  vgrads[0][0] = _ugrads[q_point][0][0];
	  vgrads[0][1] = _ugrads[q_point][0][1];
	  vgrads[1][0] = _ugrads[q_point][1][0];
	  vgrads[1][1] = _ugrads[q_point][1][1];

	  Tensor<2,2> realgrads;
	  realgrads.clear();
	  realgrads[0][0] = kappa * vgrads[0][0] + lambda * vgrads[1][1];
	  realgrads[0][1] = mu * vgrads[0][1] + mu * vgrads[1][0];
	  realgrads[1][0] = mu * vgrads[0][1] + mu * vgrads[1][0];
	  realgrads[1][1] = kappa * vgrads[1][1] + lambda * vgrads[0][0];

	  Tensor<2,2> deviator;
	  deviator.clear();
	  deviator[0][0] = realgrads[0][0] - rho * vgrads[0][0] - rho * vgrads[1][1];
	  deviator[0][1] = realgrads[0][1];
	  deviator[1][0] = realgrads[1][0];
	  deviator[1][1] = realgrads[1][1] - rho * vgrads[0][0] - rho * vgrads[1][1];

          norm = sqrt(deviator[0][0]*deviator[0][0] + deviator[0][1]*deviator[0][1] + deviator[1][0]*deviator[1][0] + deviator[1][1]*deviator[1][1]);

          factor = sigma/norm;

	  Tensor<2,2> projector;
	  projector.clear();
	  projector[0][0] = factor * deviator[0][0] + rho * vgrads[0][0] + rho * vgrads[1][1];
	  projector[0][1] = factor * deviator[0][1];
	  projector[1][0] = factor * deviator[1][0];
	  projector[1][1] = factor * deviator[1][1] + rho * vgrads[0][0] + rho * vgrads[1][1];

	  if (norm <= sigma)
	  {

	    for (unsigned int i=0;i<n_dofs_per_cell;i++)
	    {
	      const Tensor<2,2> phi_i_grads_v = state_fe_values[displacements].gradient (i, q_point);
	      const Tensor<2,2> phi_i_grads = 0.5 * phi_i_grads_v + 0.5 * transpose(phi_i_grads_v);

	      local_cell_vector(i) +=  scale * scalar_product(realgrads ,phi_i_grads)
		* state_fe_values.JxW(q_point);
	    }
	  }
	  else
	  {
	    for (unsigned int i=0;i<n_dofs_per_cell;i++)
	    {
	      const Tensor<2,2> phi_i_grads_v = state_fe_values[displacements].gradient (i, q_point);
	      const Tensor<2,2> phi_i_grads = 0.5 * phi_i_grads_v + 0.5 * transpose(phi_i_grads_v);

	      local_cell_vector(i) +=  scale * scalar_product(projector ,phi_i_grads)
		* state_fe_values.JxW(q_point);
	    }
	  }
	}
    }

    void CellMatrix(const CellDataContainer<DoFHandler<dealdim>, VECTOR, dealdim>& cdc,
		     FullMatrix<double> &local_entry_matrix, double scale, double)
    {
      assert(this->_problem_type == "state");

      const DOpEWrapper::FEValues<dealdim> & state_fe_values = cdc.GetFEValuesState();
      unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
      unsigned int n_q_points = cdc.GetNQPoints();

      _uvalues.resize(n_q_points,Vector<double>(2));
      _ugrads.resize(n_q_points,vector<Tensor<1,2> >(2));

      cdc.GetValuesState("last_newton_solution",_uvalues);
      cdc.GetGradsState("last_newton_solution",_ugrads);

      const FEValuesExtractors::Vector displacements (0);

      const double mu = 80193.800283;
      const double kappa = 271131.389455;
      const double lambda = 110743.788889;
      const double rho = 190937.589172;

      const double sigma = sqrt(2./3.)* 450.0;
      double norm = 0.;
      double factor = 0.;

      for(unsigned int q_point = 0; q_point < n_q_points; q_point++)
	{
	  norm = 0.;
          factor = 0.;

	  Tensor<2,2> vgrads;
	  vgrads.clear();
	  vgrads[0][0] = _ugrads[q_point][0][0];
	  vgrads[0][1] = _ugrads[q_point][0][1];
	  vgrads[1][0] = _ugrads[q_point][1][0];
	  vgrads[1][1] = _ugrads[q_point][1][1];

	  Tensor<2,2> realgrads;
	  realgrads.clear();
	  realgrads[0][0] = kappa * vgrads[0][0] + lambda * vgrads[1][1];
	  realgrads[0][1] = mu * vgrads[0][1] + mu * vgrads[1][0];
	  realgrads[1][0] = mu * vgrads[0][1] + mu * vgrads[1][0];
	  realgrads[1][1] = kappa * vgrads[1][1] + lambda * vgrads[0][0];

	  Tensor<2,2> deviator;
	  deviator.clear();
	  deviator[0][0] = realgrads[0][0] - rho * vgrads[0][0] - rho * vgrads[1][1];
	  deviator[0][1] = realgrads[0][1];
	  deviator[1][0] = realgrads[1][0];
	  deviator[1][1] = realgrads[1][1] - rho * vgrads[0][0] - rho * vgrads[1][1];

          norm = sqrt(deviator[0][0]*deviator[0][0] + deviator[0][1]*deviator[0][1] + deviator[1][0]*deviator[1][0] + deviator[1][1]*deviator[1][1]);

          factor = sigma/norm;

	  for(unsigned int j = 0; j < n_dofs_per_cell; j++)
	  {
	    const Tensor<2,2> phi_j_grads_v = state_fe_values[displacements].gradient (j, q_point);

	    Tensor<2,2> phi_j_grads_real;

	    phi_j_grads_real[0][0] = kappa * phi_j_grads_v[0][0] + lambda * phi_j_grads_v[1][1];
	    phi_j_grads_real[0][1] = mu * phi_j_grads_v[0][1] + mu * phi_j_grads_v[1][0];
	    phi_j_grads_real[1][0] = mu * phi_j_grads_v[0][1] + mu * phi_j_grads_v[1][0];
	    phi_j_grads_real[1][1] = kappa * phi_j_grads_v[1][1] + lambda * phi_j_grads_v[0][0];

	    Tensor<2,2> phi_j_grads_dev;

	    phi_j_grads_dev[0][0] = phi_j_grads_real[0][0] - rho * phi_j_grads_v[0][0] - rho * phi_j_grads_v[1][1];
	    phi_j_grads_dev[0][1] = phi_j_grads_real[0][1];
	    phi_j_grads_dev[1][0] = phi_j_grads_real[1][0];
	    phi_j_grads_dev[1][1] = phi_j_grads_real[1][1] - rho * phi_j_grads_v[0][0] - rho * phi_j_grads_v[1][1];

	    Tensor<2,2> dev = deviator;

	    double newnorm = norm;

	    double prod = scalar_product(dev,phi_j_grads_dev);

	    Tensor<2,2> traceterm;
	    traceterm[0][0] = 0.5 * (phi_j_grads_real[0][0] + phi_j_grads_real[1][1]);
	    traceterm[0][1] = 0;
	    traceterm[1][0] = 0;
	    traceterm[1][1] = 0.5 * (phi_j_grads_real[0][0] + phi_j_grads_real[1][1]);


	    Tensor<2,2> fullderivative = - sigma/(newnorm*newnorm*newnorm) * prod * dev + sigma/newnorm * phi_j_grads_dev + traceterm;

	    for(unsigned int i = 0; i < n_dofs_per_cell; i++)
	    {
	      const Tensor<2,2> phi_i_grads_v = state_fe_values[displacements].gradient (i, q_point);
	      const Tensor<2,2> phi_i_grads_test = 0.5 * phi_i_grads_v + 0.5 * transpose(phi_i_grads_v);

	      if (norm <= sigma)
	      {
		local_entry_matrix(i,j) += scale * scalar_product(phi_j_grads_real, phi_i_grads_test)
		  * state_fe_values.JxW(q_point);
	      }
	      else
	      {
		local_entry_matrix(i,j) +=scale *  scalar_product(fullderivative, phi_i_grads_test)
		  * state_fe_values.JxW(q_point);
	      }
	    }
	  }
	}
    }

    void CellRightHandSide(const CellDataContainer<DoFHandler<dealdim>, VECTOR, dealdim>& /*cdc*/,
                           dealii::Vector<double> &local_cell_vector __attribute__((unused)),
                           double scale __attribute__((unused)))
    {
      assert(this->_problem_type == "state");
    }


    // Values for boundary integrals
    void BoundaryEquation (const FaceDataContainer<DoFHandler<dealdim>, VECTOR, dealdim>& fdc,
			   dealii::Vector<double> &local_cell_vector,
			   double scale)
    {

      assert(this->_problem_type == "state");

      const auto & state_fe_face_values = fdc.GetFEFaceValuesState();
      unsigned int n_dofs_per_cell = fdc.GetNDoFsPerCell();
      unsigned int n_q_points = fdc.GetNQPoints();
      unsigned int color = fdc.GetBoundaryIndicator();

      //traction on the upper boundary segment realized as Neumann condition
      if (color == 3)
	{
 	  const FEValuesExtractors::Vector displacements (0);

	  Tensor<1,2> g;
	  g[0] = 0;
	  g[1] = 400;

	  for (unsigned int q_point=0;q_point<n_q_points;q_point++)
	  {
	    for (unsigned int i=0;i<n_dofs_per_cell;i++)
	    {
	      const Tensor<1,2> phi_i_v = state_fe_face_values[displacements].value (i, q_point);

	      local_cell_vector(i) += -scale * (g * phi_i_v ) * state_fe_face_values.JxW(q_point);
	    }
	  }
	}
    }

    void BoundaryMatrix (const FaceDataContainer<DoFHandler<dealdim>, VECTOR, dealdim>& /*fdc*/,
			 dealii::FullMatrix<double> &/*local_entry_matrix*/)
    {
      assert(this->_problem_type == "state");
    }

    void BoundaryRightHandSide (const FaceDataContainer<DoFHandler<dealdim>, VECTOR, dealdim>& /*fdc*/,
				dealii::Vector<double> &/*local_cell_vector*/,
				double /*scale*/)
     {
       	assert(this->_problem_type == "state");
     }




    UpdateFlags GetUpdateFlags() const
    {
	return update_values | update_gradients  | update_quadrature_points;
    }

    UpdateFlags GetFaceUpdateFlags() const
    {
 	return update_values | update_gradients  | update_normal_vectors | update_quadrature_points;
    }

    unsigned int GetStateNBlocks() const
    {
      return 1;
    }
    std::vector<unsigned int>& GetStateBlockComponent(){ return _state_block_components; }
    const std::vector<unsigned int>& GetStateBlockComponent() const{ return _state_block_components; }

  protected:
    inline void GetValues(const DOpEWrapper::FEValues<dealdim>& fe_values,
			  const map<string, const BlockVector<double>* >& domain_values,string name,
			  vector<Vector<double> >& values)
    {
      typename map<string, const BlockVector<double>* >::const_iterator it = domain_values.find(name);
      if(it == domain_values.end())
	{
	  throw DOpEException("Did not find " + name,"LocalPDE::GetValues");
	}
      fe_values.get_function_values(*(it->second),values);
    }

    inline void GetGrads(const DOpEWrapper::FEValues<dealdim>& fe_values,
			 const map<string, const BlockVector<double>* >& domain_values,string name,
			 vector<vector<Tensor<1,dealdim> > >& values)
    {
      typename map<string, const BlockVector<double>* >::const_iterator it = domain_values.find(name);
      if(it == domain_values.end())
	{
	  throw DOpEException("Did not find " + name,"LocalPDE::GetGrads");
	}
      fe_values.get_function_grads(*(it->second),values);
    }

  private:
    vector<Vector<double> > _uvalues;

    vector<vector<Tensor<1,dealdim> > > _ugrads;

    vector<unsigned int> _state_block_components;
  };
#endif
