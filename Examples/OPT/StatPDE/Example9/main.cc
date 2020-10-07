/**
 *
 * Copyright (C) 2012-2018 by the DOpElib authors
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

#include <iostream>
#include <fstream>

#include <deal.II/grid/tria.h>
#include <deal.II/grid/grid_in.h>
#if DEAL_II_VERSION_GTE(9,1,1)
#else
#include <deal.II/grid/tria_boundary_lib.h>
#endif
#include <deal.II/grid/grid_generator.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_dgp.h>
#include <deal.II/fe/fe_nothing.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/numerics/error_estimator.h>
#include <deal.II/grid/grid_refinement.h>
#if DEAL_II_VERSION_GTE(9,0,0)
#include <deal.II/grid/manifold_lib.h>
#endif

#include <opt_algorithms/reducednewtonalgorithm.h>
#include <container/optproblemcontainer.h>
#include <interfaces/functionalinterface.h>
#include <reducedproblems/statreducedproblem.h>
#include <templates/newtonsolver.h>
#include <templates/directlinearsolver.h>
#include <templates/voidlinearsolver.h>
#include <templates/integrator.h>
#include <problemdata/noconstraints.h>
#include <include/solutionextractor.h>

#include <templates/integratormixeddims.h>
#include <templates/newtonsolvermixeddims.h>
#include <include/parameterreader.h>
#include <basic/mol_spacetimehandler.h>
#include <problemdata/simpledirichletdata.h>
#include <container/integratordatacontainer.h>

#include "localpde.h"
#include "localfunctional.h"
#include "functionals.h"
#include "my_functions.h"

using namespace std;
using namespace dealii;
using namespace DOpE;

const static int DIM = 2;
const static int CDIM = 0;

#if DEAL_II_VERSION_GTE(9,3,0)
#define DOFHANDLER false
#else
#define DOFHANDLER DoFHandler
#endif

#define FE FESystem

typedef QGauss<DIM> QUADRATURE;
typedef QGauss<DIM - 1> FACEQUADRATURE;

typedef BlockSparseMatrix<double> MATRIX;
typedef BlockSparsityPattern SPARSITYPATTERN;
typedef BlockVector<double> VECTOR;

#define EDC ElementDataContainer
#define FDC FaceDataContainer

typedef LocalFunctional<EDC, FDC, DOFHANDLER, VECTOR, CDIM, DIM> COSTFUNCTIONAL;
typedef FunctionalInterface<EDC, FDC, DOFHANDLER, VECTOR, CDIM, DIM> FUNCTIONALINTERFACE;
typedef LocalPDE<EDC, FDC, DOFHANDLER, VECTOR, DIM> PDE;

typedef OptProblemContainer<FUNCTIONALINTERFACE, COSTFUNCTIONAL, PDE,
        SimpleDirichletData<VECTOR, DIM>,
        NoConstraints<EDC, FDC, DOFHANDLER, VECTOR, CDIM, DIM>, SPARSITYPATTERN,
        VECTOR, CDIM, DIM> OP;

typedef IntegratorDataContainer<DOFHANDLER, Quadrature<DIM>, Quadrature<1>,
        VECTOR, DIM> IDC;
typedef Integrator<IDC, VECTOR, double, DIM> INTEGRATOR;
typedef IntegratorMixedDimensions<IDC, VECTOR, double, CDIM, DIM> INTEGRATORM;

typedef DirectLinearSolverWithMatrix<BlockSparsityPattern,
        BlockSparseMatrix<double>, VECTOR> LINEARSOLVER;

typedef VoidLinearSolver<VECTOR> VOIDLS;

typedef NewtonSolverMixedDimensions<INTEGRATORM, VOIDLS, VECTOR> NLSM;
typedef NewtonSolver<INTEGRATOR, LINEARSOLVER, VECTOR> NLS;
typedef ReducedNewtonAlgorithm<OP, VECTOR> RNA;
typedef StatReducedProblem<NLSM, NLS, INTEGRATORM, INTEGRATOR, OP, VECTOR, CDIM,
        DIM> RP;

typedef MethodOfLines_SpaceTimeHandler<FE, DOFHANDLER, SPARSITYPATTERN, VECTOR,
        CDIM, DIM> STH;


void
declare_params(ParameterReader &param_reader)
{
  param_reader.SetSubsection("main parameters");
  param_reader.declare_entry("global_refinement", "0", Patterns::Integer(0));
  param_reader.declare_entry("initial_control", "0", Patterns::Double());
  param_reader.declare_entry("solve_or_check", "solve", Patterns::Anything());
}

int
main(int argc, char **argv)
{
  /**
   * In this example we study
   * stationary (nonlinear) FSI optimization. The configuration
   * comes from the original fluid benchmark problem
   * (Schaefer/Turek; 1996)
   * and has been modified to reduce drag around the
   * cylinder and the beam. The gain the solvability of
   * the optimization problem we add a quadratic
   * regularization term to the cost functional.
   */
  
  dealii::Utilities::MPI::MPI_InitFinalize mpi(argc, argv);

  string paramfile = "dope.prm";

  if (argc == 2)
    {
      paramfile = argv[1];
    }
  else if (argc > 2)
    {
      std::cout << "Usage: " << argv[0] << " [ paramfile ] " << std::endl;
      return -1;
    }

  ParameterReader pr;
  RP::declare_params(pr);
  RNA::declare_params(pr);
  PDE::declare_params(pr);
  COSTFUNCTIONAL::declare_params(pr);
  BoundaryParabel::declare_params(pr);
  LocalBoundaryFaceFunctionalDrag<EDC, FDC, DOFHANDLER, VECTOR, CDIM, DIM>::declare_params(
    pr);
  LocalBoundaryFaceFunctionalLift<EDC, FDC, DOFHANDLER, VECTOR, CDIM, DIM>::declare_params(
    pr);

  // Declare parameter for this section
  declare_params(pr);
  pr.read_parameters(paramfile);

  // Parameters for the main file
  pr.SetSubsection("main parameters");
  const int _global_refinement = pr.get_integer("global_refinement");
  const double _initial_control = pr.get_double("initial_control");
  const std::string _solve_or_check = pr.get_string("solve_or_check");
  // Mesh-refinement cycles
  const int niter = 1;

  Triangulation<DIM> triangulation(Triangulation<DIM>::maximum_smoothing);

  GridIn<DIM> grid_in;
  grid_in.attach_triangulation(triangulation);
  std::ifstream input_file("gitter.inp");

  grid_in.read_ucd(input_file);

  Point<DIM> p(0.2, 0.2);
#if DEAL_II_VERSION_GTE(9,0,0)
  static const SphericalManifold<DIM> boundary(p);
  triangulation.set_all_manifold_ids_on_boundary(80,80);
  triangulation.set_all_manifold_ids_on_boundary(81,81);
  triangulation.set_manifold(80,boundary);
  triangulation.set_manifold(81,boundary);
#else
  double radius = 0.05;
  static const HyperBallBoundary<DIM> boundary(p, radius);
  triangulation.set_boundary(80, boundary);
  triangulation.set_boundary(81, boundary);
#endif
  triangulation.refine_global(_global_refinement);


  FE<DIM> control_fe(FE_Nothing<DIM>(1), 2); //2 Parameter
  FE<DIM> state_fe(FE_Q<DIM>(2), 2, // velocities
                   FE_Q<DIM>(2), 2, // displacements
                   FE_DGP<DIM>(1), 1); // pressure

  QUADRATURE quadrature_formula(3);
  FACEQUADRATURE face_quadrature_formula(3);
  IDC idc(quadrature_formula, face_quadrature_formula);

  PDE LPDE(pr);
  COSTFUNCTIONAL LFunc(pr);

  LocalPointFunctionalDeflectionX<EDC, FDC, DOFHANDLER, VECTOR, CDIM, DIM> LPFDX;
  LocalPointFunctionalDeflectionY<EDC, FDC, DOFHANDLER, VECTOR, CDIM, DIM> LPFDY;
  LocalBoundaryFaceFunctionalDrag<EDC, FDC, DOFHANDLER, VECTOR, CDIM, DIM> LBFD(
    pr);
  LocalBoundaryFaceFunctionalLift<EDC, FDC, DOFHANDLER, VECTOR, CDIM, DIM> LBFL(
    pr);

  STH DOFH(triangulation, control_fe, state_fe, DOpEtypes::stationary);

  NoConstraints<EDC, FDC, DOFHANDLER, VECTOR, CDIM,
                DIM> Constraints;

  OP P(LFunc, LPDE, Constraints, DOFH);

  P.AddFunctional(&LPFDX);
  P.AddFunctional(&LPFDY);
  P.AddFunctional(&LBFD);
  P.AddFunctional(&LBFL);

  // fuer Drag und Lift Auswertung am Zylinder
  P.SetBoundaryFunctionalColors(80);
  P.SetBoundaryFunctionalColors(81);

  // Due to regularization
  P.SetBoundaryFunctionalColors(50);
  P.SetBoundaryFunctionalColors(51);

  DOpEWrapper::ZeroFunction<DIM> zf(5);
  SimpleDirichletData<VECTOR, DIM> DD1(zf);

  BoundaryParabel boundary_parabel(pr);
  SimpleDirichletData<VECTOR, DIM> DD2(boundary_parabel);

  std::vector<bool> comp_mask(5, true);
  comp_mask[4] = false;

  P.SetDirichletBoundaryColors(0, comp_mask, &DD2); // flow by Dirichlet data
  P.SetDirichletBoundaryColors(2, comp_mask, &DD1);
  P.SetDirichletBoundaryColors(80, comp_mask, &DD1);
  P.SetDirichletBoundaryColors(81, comp_mask, &DD1); // only for FSI

  P.SetBoundaryEquationColors(1); // do-nothing at outflow boundary
  P.SetBoundaryEquationColors(50); // upper control bc \Gamma_q1
  P.SetBoundaryEquationColors(51); // lower control bc \Gamma_q2

  // We need these functions to evaluate
  // BoundaryEquation_Q, etc.
  P.SetControlBoundaryEquationColors(50); // upper control bc \Gamma_q1
  P.SetControlBoundaryEquationColors(51); // lower control bc \Gamma_q2

  RP solver(&P, DOpEtypes::VectorStorageType::fullmem, pr, idc);
  RNA Alg(&P, &solver, pr);

  std::string cases = _solve_or_check.c_str();

  Vector<double> solution;
  Alg.ReInit();
  ControlVector<VECTOR> q(&DOFH, DOpEtypes::VectorStorageType::fullmem,pr);
  q = _initial_control;

  for (int i = 0; i < niter; i++)
    {
      try
        {

          ControlVector<VECTOR> dq(q);
          // Initialization of the control
          if (cases == "check")
            {

              ControlVector<VECTOR> dq(q);
              //dq = 1.0;
              // eps: step size for difference quotient
              // choose: 1.0, 0.1, 0.01, etc.
              double eps_diff = 1.0;
              Alg.CheckGrads(eps_diff, q, dq, 3);
              Alg.CheckHessian(eps_diff, q, dq, 3);
            }
          else
            {
              //Alg.SolveForward(q);  // just solves the forward problem
              Alg.Solve(q);
            }
        }
      catch (DOpEException &e)
        {
          std::cout
              << "Warning: During execution of `" + e.GetThrowingInstance()
              + "` the following Problem occurred!" << std::endl;
          std::cout << e.GetErrorMessage() << std::endl;
        }
      if (i != niter - 1)
        {
          SolutionExtractor<RP, VECTOR> a(solver);
          const StateVector<VECTOR> &gu = a.GetU();
          solution = 0;
          solution = gu.GetSpacialVector();
          Vector<float> estimated_error_per_element(triangulation.n_active_cells());

          std::vector<bool> component_mask(5, true);

#if DEAL_II_VERSION_GTE(9,1,1)
          KellyErrorEstimator<DIM>::estimate(
            static_cast<const DoFHandler<DIM>&>(DOFH.GetStateDoFHandler()),
            QGauss<1>(2), std::map<types::boundary_id, const Function<DIM> *>(), solution,
            estimated_error_per_element, component_mask);
#else
          KellyErrorEstimator<DIM>::estimate(
            static_cast<const DoFHandler<DIM>&>(DOFH.GetStateDoFHandler()),
	    QGauss<1>(2), FunctionMap<DIM>::type(), solution,
            estimated_error_per_element, component_mask);
#endif
	  RefineFixedNumber ref_cont(estimated_error_per_element,0.3,0.0);
	  DOFH.RefineSpace(ref_cont);
        }
    }

  return 0;
}
#undef FDC
#undef EDC
#undef FE
#undef DOFHANDLER
