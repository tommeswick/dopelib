#ifndef _MOL_STATESPACE_TIME_HANDLER_H_
#define _MOL_STATESPACE_TIME_HANDLER_H_

#include "statespacetimehandler.h"
#include "constraints.h"
#include "sparsitymaker.h"
#include "constraintsmaker.h"
#include "sth_internals.h"

#include <dofs/dof_handler.h>
#include <dofs/dof_renumbering.h>
#include <dofs/dof_tools.h>
#include <lac/constraint_matrix.h>
#include <deal.II/numerics/solution_transfer.h>
#include <deal.II/grid/grid_refinement.h>

namespace DOpE
{
  /**
   * Implements a Space Time Handler with a Method of Lines discretization.
   * This means there is only one fixed mesh for the spatial domain.
   */
  template<typename FE, typename DOFHANDLER, typename SPARSITYPATTERN,
      typename VECTOR, typename SPARSITYMAKER, typename CONSTRAINTSMAKER,
      int dealdim>
    class MethodOfLines_StateSpaceTimeHandler : public StateSpaceTimeHandler<
        FE, DOFHANDLER, SPARSITYPATTERN, VECTOR, dealdim>
    {
      public:
        MethodOfLines_StateSpaceTimeHandler(
            dealii::Triangulation<dealdim>& triangulation, const FE& state_fe,
            const ActiveFEIndexSetterInterface<dealdim>& index_setter =
                ActiveFEIndexSetterInterface<dealdim> ()) :
          StateSpaceTimeHandler<FE, DOFHANDLER, SPARSITYPATTERN, VECTOR,
              dealdim> (index_setter), _sparse_mkr_dynamic(true),
              _constr_mkr_dynamic(true), _triangulation(triangulation),
              _state_dof_handler(_triangulation), _state_fe(&state_fe)
        {
          _sparsitymaker = new SPARSITYMAKER;
          _constraintsmaker = new CONSTRAINTSMAKER;
        }
        MethodOfLines_StateSpaceTimeHandler(
            dealii::Triangulation<dealdim>& triangulation, const FE& state_fe,
            const dealii::Triangulation<1> & times,
            const ActiveFEIndexSetterInterface<dealdim>& index_setter =
                ActiveFEIndexSetterInterface<dealdim> ()) :
          StateSpaceTimeHandler<FE, DOFHANDLER, SPARSITYPATTERN, VECTOR,
              dealdim> (times, index_setter), _sparse_mkr_dynamic(true),
              _constr_mkr_dynamic(true), _triangulation(triangulation),
              _state_dof_handler(_triangulation), _state_fe(&state_fe)
        {
          _sparsitymaker = new SPARSITYMAKER;
          _constraintsmaker = new CONSTRAINTSMAKER;
        }

        virtual ~MethodOfLines_StateSpaceTimeHandler()
        {
          _state_dof_handler.clear();

          if (_sparsitymaker != NULL && _sparse_mkr_dynamic == true)
          {
            delete _sparsitymaker;
          }
          if (_constraintsmaker != NULL && _constr_mkr_dynamic == true)
          {
            delete _constraintsmaker;
          }
        }

        /**
         * Implementation of virtual function in StateSpaceTimeHandler
         */
        void
        ReInit(unsigned int state_n_blocks,
            const std::vector<unsigned int>& state_block_component)
        {

          StateSpaceTimeHandler<FE, DOFHANDLER, SPARSITYPATTERN, VECTOR,
              dealdim>::SetActiveFEIndicesState(_state_dof_handler);
          _state_dof_handler.distribute_dofs(GetFESystem("state"));
          DoFRenumbering::component_wise(
              static_cast<DOFHANDLER&> (_state_dof_handler),
              state_block_component);

          GetConstraintsMaker()->MakeConstraints(_state_dof_handler,
              _state_hanging_node_constraints);

          _state_dofs_per_block.resize(state_n_blocks);
          DoFTools::count_dofs_per_block(
              static_cast<DOFHANDLER&> (_state_dof_handler),
              _state_dofs_per_block, state_block_component);

          _support_points.clear();

          //Initialize also the timediscretization.
          this->ReInitTime();

          //There where changes invalidate tickets
          this->IncrementStateTicket();
        }

        /**
         * Implementation of virtual function in StateSpaceTimeHandler
         */
        const DOpEWrapper::DoFHandler<dealdim, DOFHANDLER>&
        GetStateDoFHandler() const
        {
          //There is only one mesh, hence always return this
          return _state_dof_handler;
        }

        /**
         * Implementation of virtual function in StateSpaceTimeHandler
         */
        unsigned int
        GetStateDoFsPerBlock(unsigned int b, int /*time_point*/= -1) const
        {
          return _state_dofs_per_block[b];
        }

        /**
         * Implementation of virtual function in StateSpaceTimeHandlerBase
         */
        const std::vector<unsigned int>&
        GetStateDoFsPerBlock(int /*time_point*/= -1) const
        {
          return _state_dofs_per_block;
        }

        /**
         * Implementation of virtual function in StateSpaceTimeHandler
         */
        const dealii::ConstraintMatrix&
        GetStateHangingNodeConstraints() const
        {
          return _state_hanging_node_constraints;
        }

        /**
         * Implementation of virtual function in StateSpaceTimeHandlerBase
         */
        virtual void
        InterpolateState(VECTOR& result,
            const std::vector<VECTOR*> & local_vectors, double t,
            const TimeIterator& it) const
        {
          assert(it.get_left() <= t);
          assert(it.get_right() >= t);
          if (local_vectors.size() != 2)
            throw DOpEException(
                "This function is currently not implemented for anything other than"
                  " linear interpolation of 2 DoFs.",
                "MethodOfLine_SpaceTimeHandler::InterpolateState");

          double lambda_l = (it.get_right() - t) / it.get_k();
          double lambda_r = (t - it.get_left()) / it.get_k();

          //Here we assume that the numbering of dofs goes from left to right!
          result = *local_vectors[0];

          result.sadd(lambda_l, lambda_r, *local_vectors[1]);
        }

        /**
         * Implementation of virtual function in StateSpaceTimeHandlerBase
         */
        unsigned int
        GetStateNDoFs(int /*time_point*/= -1) const
        {
          return GetStateDoFHandler().n_dofs();
        }

        /**
         * Implementation of virtual function in StateSpaceTimeHandler
         */
        const std::vector<Point<dealdim> >&
        GetMapDoFToSupportPoints()
        {
          _support_points.resize(GetStateNDoFs());
          //TODO should get Mapping used for  the FE.
          DOpE::STHInternals::MapDoFsToSupportPoints<
              std::vector<Point<dealdim> >, dealdim>(GetStateDoFHandler(),
              _support_points);
          return _support_points;
        }

        /******************************************************/
        void
        ComputeStateSparsityPattern(SPARSITYPATTERN & sparsity) const
        {
          this->GetSparsityMaker()->ComputeSparsityPattern(
              this->GetStateDoFHandler(), sparsity,
              this->GetStateHangingNodeConstraints(),
              this->GetStateDoFsPerBlock());
        }

        /******************************************************/

        /**
         * Implementation of virtual function in StateSpaceTimeHandler
         */
        const FE&
        GetFESystem(std::string name) const
        {
          if (name == "state")
          {
            return *_state_fe;
          }
          else
          {
            abort();
            throw DOpEException("Not implemented for name =" + name,
                "MethodOfLines_StateSpaceTimeHandler::GetFESystem");
          }

        }

        /******************************************************/

        /**
         * This Function is used to refine the spatial mesh.
         * After calling a refinement function a reinitialization is required!
         *
         * @param ref_type          A string telling how to refine, feasible values are at present
         *                          'global', 'fixedfraction', 'fixednumber', 'optimized'
         * @param indicators        A set of positive values, used to guide refinement.
         * @param topfraction       In a fixed fraction strategy, wich part should be refined
         * @param bottomfraction    In a fixed fraction strategy, wich part should be coarsened
         */
        void
        RefineSpace(std::string ref_type, const Vector<float>* indicators =
            NULL, double topfraction = 0.1, double bottomfraction = 0.0)
        {
          assert(bottomfraction == 0.0);

          if ("global" == ref_type)
          {
            _triangulation.set_all_refine_flags();
          }
          else if ("fixednumber" == ref_type)
          {
            assert(indicators != NULL);
            GridRefinement::refine_and_coarsen_fixed_number(_triangulation,
                *indicators, topfraction, bottomfraction);
          }
          else if ("fixedfraction" == ref_type)
          {
            assert(indicators != NULL);
            GridRefinement::refine_and_coarsen_fixed_fraction(_triangulation,
                *indicators, topfraction, bottomfraction);
          }
          else if ("optimized" == ref_type)
          {
            assert(indicators != NULL);
            GridRefinement::refine_and_coarsen_optimize(_triangulation,
                *indicators);
          }
          else
          {
            throw DOpEException("Not implemented for name =" + ref_type,
                "MethodOfLines_StateSpaceTimeHandler::RefineStateSpace");
          }
          _triangulation.prepare_coarsening_and_refinement();

          _triangulation.execute_coarsening_and_refinement();
        }
        /******************************************************/

        /**
         * Implementation of virtual function in StateSpaceTimeHandlerBase
         */
        unsigned int
        NewTimePointToOldTimePoint(unsigned int t) const
        {
          //TODO this has to be implemented when temporal refinement is possible!
          //At present the temporal grid can't be refined
          return t;
        }

        /******************************************************/
        /**
         * Through this function one commits a constraints_maker
         * to the class. With the help of the constraints_maker
         * one has the capability to  impose additional constraints
         * on the state-dofs (for example a pressure filter for the
         * stokes problem). This function must be called prior to
         * ReInit.
         */
        void
        SetConstraintsMaker(CONSTRAINTSMAKER& constraints_maker)
        {
          if (_constraintsmaker != NULL && _constr_mkr_dynamic)
            delete _constraintsmaker;
          _constraintsmaker = &constraints_maker;
          _constr_mkr_dynamic = false;
        }
        /******************************************************/
        /**
         * Through this function one commits a sparsity_maker
         * to the class. With the help of the sparsity_maker
         * one has the capability to create non-standard sparsity
         * patterns. This function must be called prior to
         * ReInit.
         */
        void
        SetSparsityMaker(SPARSITYMAKER& sparsity_maker)
        {
          if (_sparsitymaker != NULL && _sparse_mkr_dynamic)
            delete _sparsitymaker;
          _sparsitymaker = &sparsity_maker;
          _sparse_mkr_dynamic = false;
        }

      private:
        const SPARSITYMAKER*
        GetSparsityMaker() const
        {
          return _sparsitymaker;
        }
        const CONSTRAINTSMAKER*
        GetConstraintsMaker() const
        {
          return _constraintsmaker;
        }
        SPARSITYMAKER* _sparsitymaker;
        CONSTRAINTSMAKER* _constraintsmaker;
        bool _sparse_mkr_dynamic, _constr_mkr_dynamic;

        dealii::Triangulation<dealdim>& _triangulation;
        DOpEWrapper::DoFHandler<dealdim, DOFHANDLER> _state_dof_handler;

        std::vector<unsigned int> _state_dofs_per_block;

        dealii::ConstraintMatrix _state_hanging_node_constraints;

        const dealii::SmartPointer<const FE> _state_fe;//TODO is there a reason that this is not a reference?

        std::vector<Point<dealdim> > _support_points;

    };

}
#endif
