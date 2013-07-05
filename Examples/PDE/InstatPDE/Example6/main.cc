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

#include "reducednewtonalgorithm.h"
#include "instatoptproblemcontainer.h"
#include "forward_euler_problem.h"
#include "backward_euler_problem.h"
#include "crank_nicolson_problem.h"
#include "shifted_crank_nicolson_problem.h"
#include "fractional_step_theta_problem.h"
#include "functionalinterface.h"
#include "pdeinterface.h"
#include "instatreducedproblem.h"
#include "instat_step_newtonsolver.h"
#include "fractional_step_theta_step_newtonsolver.h"
#include "newtonsolver.h"
#include "gmreslinearsolver.h"
#include "cglinearsolver.h"
#include "directlinearsolver.h"
#include "integrator.h"
#include "parameterreader.h"
#include "mol_spacetimehandler.h"
#include "simpledirichletdata.h"
#include "noconstraints.h"
#include "sparsitymaker.h"
#include "userdefineddofconstraints.h"
#include "integratordatacontainer.h"


#include <iostream>
#include <fstream>

#include <grid/tria.h>
#include <grid/grid_in.h>
#include <grid/tria_boundary_lib.h>
#include <dofs/dof_handler.h>
#include <grid/grid_generator.h>
#include <fe/fe_q.h>
#include <fe/fe_dgp.h>
#include <dofs/dof_tools.h>
#include <base/quadrature_lib.h>
#include <base/function.h>

#include "localpde.h"
#include "localfunctional.h"
#include "functionals.h"
#include "indexsetter.h"

using namespace std;
using namespace dealii;
using namespace DOpE;

// Define dimensions for control- and state problem
#define LOCALDOPEDIM 2
#define LOCALDEALDIM 2
#define VECTOR BlockVector<double>
#define SPARSITYPATTERN BlockSparsityPattern
#define MATRIX BlockSparseMatrix<double>
#define DOFHANDLER hp::DoFHandler
#define FE hp::FECollection
#define QUADRATURE hp::QCollection
#define FACEQUADRATURE hp::QCollection
#define FUNC FunctionalInterface<CellDataContainer,FaceDataContainer,DOFHANDLER,VECTOR,LOCALDOPEDIM,LOCALDEALDIM>
#define PDE PDEInterface<CellDataContainer,FaceDataContainer,DOFHANDLER,VECTOR,LOCALDEALDIM>
#define DD DirichletDataInterface<VECTOR,LOCALDEALDIM>
#define CONS ConstraintInterface<CellDataContainer,FaceDataContainer,DOFHANDLER,VECTOR,LOCALDOPEDIM,LOCALDEALDIM>

typedef OptProblemContainer<FUNC, FUNC, PDE, DD, CONS, SPARSITYPATTERN, VECTOR,
    LOCALDOPEDIM, LOCALDEALDIM> OP_BASE;
#define PROB StateProblem<OP_BASE,PDE,DD,SPARSITYPATTERN,VECTOR,LOCALDOPEDIM,LOCALDEALDIM>

// Typedefs for timestep problem
#define TSP BackwardEulerProblem
//FIXME: This should be a reasonable dual timestepping scheme
#define DTSP BackwardEulerProblem

typedef InstatOptProblemContainer<TSP, DTSP,FUNC, FUNC, PDE, DD, CONS,
				  SPARSITYPATTERN, VECTOR, LOCALDOPEDIM, 
				  LOCALDEALDIM, FE, DOFHANDLER> OP;

#undef TSP
#undef DTSP


typedef IntegratorDataContainer<DOFHANDLER, QUADRATURE<LOCALDEALDIM>,
    QUADRATURE<LOCALDEALDIM - 1>, VECTOR, LOCALDEALDIM> IDC;

typedef Integrator<IDC, VECTOR, double, 2> INTEGRATOR;

typedef DirectLinearSolverWithMatrix<SPARSITYPATTERN, MATRIX, VECTOR> LINEARSOLVER;

typedef NewtonSolver<INTEGRATOR, LINEARSOLVER, VECTOR>
    CNLS;
typedef InstatStepNewtonSolver<INTEGRATOR, LINEARSOLVER, VECTOR> NLS;
typedef ReducedNewtonAlgorithm<OP, VECTOR> RNA;
typedef InstatReducedProblem<CNLS, NLS, INTEGRATOR, INTEGRATOR, OP,
    VECTOR, 2, 2> SSolver;

int
main(int argc, char **argv)
{
  /**
   * First version of quasi-static Biot-Lame_Navier problem
   */

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
  SSolver::declare_params(pr);
  RNA::declare_params(pr);
  LocalPDE<DOFHANDLER, VECTOR,  2>::declare_params(pr);
  pr.read_parameters(paramfile);

  std::string cases = "solve";

  Triangulation<2> triangulation;

  GridIn<2> grid_in;
  grid_in.attach_triangulation(triangulation);

  // Grid for Benchmark with flag
  std::ifstream input_file("rectangle_mandel_elasticity.inp");

  grid_in.read_ucd(input_file);

  FESystem<2> control_fe(FE_Q<2>(1), 1);
  hp::FECollection < 2 > control_fe_collection(control_fe);
  control_fe_collection.push_back(control_fe); //Need same number of entries in FECollection as state

  // FE for the state equation: u,p
  FESystem<2> state_fe(FE_Q<2>(2), 2, 
		       FE_Q<2>(1), 1);
  FESystem<2> state_fe_2(FE_Q<2>(2), 2, 
			 FE_Nothing<2>(1), 1);
  hp::FECollection < 2 > state_fe_collection(state_fe);
  state_fe_collection.push_back(state_fe_2);

  QGauss<2> quadrature_formula(3);
  QGauss<1> face_quadrature_formula(3);
  hp::QCollection<2> q_coll(quadrature_formula);
  q_coll.push_back(quadrature_formula);
  hp::QCollection<1> face_q_coll(face_quadrature_formula);
  face_q_coll.push_back(face_quadrature_formula);

  IDC idc(q_coll,face_q_coll);

  LocalPDE<DOFHANDLER,VECTOR,  2> LPDE(pr);
  LocalFunctional<DOFHANDLER,VECTOR, 2, 2> LFunc;

  LocalPointFunctionalP1<DOFHANDLER,VECTOR, 2, 2> LPFP1;
  LocalPointFunctionalP2<DOFHANDLER,VECTOR, 2, 2> LPFP2;

  //Time grid of [0,25]
  Triangulation<1> times;
  GridGenerator::subdivided_hyper_cube(times, 100, 0, 100000);

  triangulation.refine_global(3);
  ActiveFEIndexSetter<2> indexsetter(pr);
  MethodOfLines_SpaceTimeHandler<FE, DOFHANDLER, SPARSITYPATTERN, VECTOR,
      LOCALDOPEDIM, LOCALDEALDIM> DOFH(triangulation, 
				       control_fe_collection, 
				       state_fe_collection,
				       times, DOpEtypes::undefined,
				       indexsetter);

  NoConstraints<CellDataContainer, FaceDataContainer, DOFHANDLER, VECTOR,
      LOCALDOPEDIM, LOCALDEALDIM> Constraints;

  OP P(LFunc, LPDE, Constraints, DOFH);

  //  P.HasFaces();
  P.AddFunctional(&LPFP1); // p1
  P.AddFunctional(&LPFP2); // p2


  std::vector<bool> comp_mask(3);
  DOpEWrapper::ZeroFunction<2> zf(3);
  SimpleDirichletData<VECTOR, LOCALDEALDIM> DD1(zf);


  comp_mask[0] = true; // ux
  comp_mask[1] = false; // uy
  comp_mask[2] = false; // p

  P.SetDirichletBoundaryColors(0, comp_mask, &DD1); 

  comp_mask[0] = false; // ux
  comp_mask[1] = true; // uy
  comp_mask[2] = false; // p

  P.SetDirichletBoundaryColors(2, comp_mask, &DD1); 

  comp_mask[0] = false; // ux
  comp_mask[1] = false; // uy
  comp_mask[2] = true; // p

  P.SetDirichletBoundaryColors(1, comp_mask, &DD1); 

  comp_mask[0] = false; // ux
  comp_mask[1] = false; // uy
  comp_mask[2] = true; // p

  P.SetDirichletBoundaryColors(11, comp_mask, &DD1); 

  comp_mask[0] = false; // ux
  comp_mask[1] = false; // uy
  comp_mask[2] = false; // p

  P.SetDirichletBoundaryColors(3, comp_mask, &DD1); 



  P.SetBoundaryEquationColors(3); // top boundary

  P.SetInitialValues(&zf);


  SSolver solver(&P, "fullmem", pr, idc);
  RNA Alg(&P, &solver, pr);

  // Mesh-refinement cycles
  int niter = 1;
  Alg.ReInit();
  ControlVector<VECTOR> q(&DOFH, "fullmem");

  for (int i = 0; i < niter; i++)
    {
      try
        {
          if (cases == "check")
            {
              ControlVector<VECTOR> dq(q);
              Alg.CheckGrads(1., q, dq, 2);
              Alg.CheckHessian(1., q, dq, 2);
            }
          else
            {
              Alg.SolveForward(q);
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
          //triangulation.refine_global (1);
          DOFH.RefineSpace();
          Alg.ReInit();
        }
    }

  return 0;
}
#undef LOCALDOPEDIM
#undef LOCALDEALDIM

