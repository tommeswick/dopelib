#include "pdeproblemcontainer.h"
#include "functionalinterface.h"
#include "pdeinterface.h"
#include "statpdeproblem.h"
#include "newtonsolver.h"
#include "directlinearsolver.h"
#include "userdefineddofconstraints.h"
#include "sparsitymaker.h"
#include "integratordatacontainer.h"

#include "integrator.h"
#include "parameterreader.h"

#include "mol_statespacetimehandler.h"
#include "simpledirichletdata.h"
#include "active_fe_index_setter_interface.h"

#include <iostream>
#include <fstream>

#include <grid/tria.h>
#include <grid/grid_in.h>
#include <grid/tria_boundary_lib.h>
#include <dofs/dof_handler.h>
#include <grid/grid_generator.h>
#include <fe/fe_q.h>
#include <fe/fe_nothing.h>
#include <dofs/dof_tools.h>
#include <base/quadrature_lib.h>
#include <base/function.h>
#include <deal.II/numerics/vectors.h>

#include "localpde.h"
#include "functionals.h"
#include "higher_order_dwrc.h"
#include "myfunctions.h"

using namespace std;
using namespace dealii;
using namespace DOpE;

#define VECTOR dealii::Vector<double>
#define MATRIX dealii::SparseMatrix<double>
#define SPARSITYPATTERN dealii::SparsityPattern
#define DOFHANDLER dealii::DoFHandler<2>
#define FE DOpEWrapper::FiniteElement<2>
#define FACEDATACONTAINER FaceDataContainer<DOFHANDLER, VECTOR, 2>

typedef PDEProblemContainer<
    PDEInterface<CellDataContainer, FaceDataContainer, DOFHANDLER, VECTOR, 2>,
    DirichletDataInterface<VECTOR, 2>, SPARSITYPATTERN, VECTOR, 2> OP;
typedef IntegratorDataContainer<DOFHANDLER, dealii::Quadrature<2>,
    dealii::Quadrature<1>, VECTOR, 2> IDC;
typedef Integrator<IDC, VECTOR, double, 2> INTEGRATOR;
//********************Linearsolver**********************************
typedef DirectLinearSolverWithMatrix<SPARSITYPATTERN, MATRIX, VECTOR, 2> LINEARSOLVER;
//********************Linearsolver**********************************

typedef NewtonSolver<INTEGRATOR, LINEARSOLVER, VECTOR, 2> NLS;
typedef StatPDEProblem<NLS, INTEGRATOR, OP, VECTOR, 2> SSolver;
typedef MethodOfLines_StateSpaceTimeHandler<FE, DOFHANDLER, SPARSITYPATTERN,
    VECTOR, 2> STH;
typedef CellDataContainer<DOFHANDLER, VECTOR, 2> CDC;
typedef FaceDataContainer<DOFHANDLER, VECTOR, 2> FDC;
typedef HigherOrderDWRContainer<STH, IDC, CDC, FDC, VECTOR> HO_DWRC;

void
declare_params(ParameterReader &param_reader)
{
  param_reader.SetSubsection("main parameters");
  param_reader.declare_entry("max_iter", "1", Patterns::Integer(0),
      "How many iterations?");
  param_reader.declare_entry("quad order", "2", Patterns::Integer(1),
      "Order of the quad formula?");
  param_reader.declare_entry("facequad order", "2", Patterns::Integer(1),
      "Order of the face quad formula?");
  param_reader.declare_entry("order fe", "2", Patterns::Integer(1),
      "Order of the finite element?");
  param_reader.declare_entry("prerefine", "1", Patterns::Integer(1),
      "How often should we refine the coarse grid?");
}

int
main(int argc, char **argv)
{
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
  DOpEOutputHandler<VECTOR>::declare_params(pr);
  declare_params(pr);

  pr.read_parameters(paramfile);

  //************************************************
  //define some constants
  pr.SetSubsection("main parameters");
  int max_iter = pr.get_integer("max_iter");
  int prerefine = pr.get_integer("prerefine");

  //*************************************************

  //Make triangulation *************************************************
  const Point<2> center(0, 0);
  const HyperShellBoundary<2> boundary_description(center);
  Triangulation<2> triangulation(
      Triangulation<2>::MeshSmoothing::patch_level_1);
  dealii::GridGenerator::hyper_cube_with_cylindrical_hole(triangulation, 0.5,
      2., 1, 1);
  triangulation.set_boundary(1, boundary_description);
  triangulation.refine_global(1);  //because we need the face located at x==0;
  for (auto it = triangulation.begin_active(); it != triangulation.end(); it++)
    if (it->center()[1] <= 0)
    {
      if (it->center()[0] < 0)
      {
        it->set_material_id(1);
      }
      else
      {
        it->set_material_id(2);
      }
    }
  if (prerefine > 0)
    triangulation.refine_global(prerefine);
  //*************************************************************

  //FiniteElemente*************************************************
  pr.SetSubsection("main parameters");
  DOpEWrapper::FiniteElement<2> state_fe(FE_Q<2>(pr.get_integer("order fe")));

  //Quadrature formulas*************************************************
  pr.SetSubsection("main parameters");
  QGauss<2> quadrature_formula(pr.get_integer("quad order"));
  QGauss<1> face_quadrature_formula(pr.get_integer("facequad order"));
  IDC idc(quadrature_formula, face_quadrature_formula);
  //**************************************************************************

  //Functionals*************************************************
  LocalFaceFunctional<VECTOR, FACEDATACONTAINER, 2> LFF;
  LocalPDELaplace<VECTOR, 2> LPDE;
  //*************************************************

  //space time handler***********************************/
  STH DOFH(triangulation, state_fe);
  /***********************************/

  OP P(LPDE, DOFH);
  P.AddFunctional(&LFF);
  //Boundary conditions************************************************
  std::vector<bool> comp_mask(1);
  comp_mask[0] = true;

  ExactSolution ex_sol;

  SimpleDirichletData<VECTOR, 2> DD1(ex_sol);
  //Set dirichlet boundary values all around
  P.SetDirichletBoundaryColors(0, comp_mask, &DD1);
  P.SetDirichletBoundaryColors(1, comp_mask, &DD1);
  /************************************************/
  SSolver solver(&P, "fullmem", pr, idc);

  //Only needed for pure PDE Problems
  DOpEOutputHandler<VECTOR> out(&solver, pr);
  DOpEExceptionHandler<VECTOR> ex(&out);
  P.RegisterOutputHandler(&out);
  P.RegisterExceptionHandler(&ex);
  solver.RegisterOutputHandler(&out);
  solver.RegisterExceptionHandler(&ex);
  /**********************************************************************/
  //DWR**********************************************************************/
  //Set dual functional for ee
  P.SetFunctionalForErrorEstimation(LFF.GetName());
  //FiniteElemente for DWR*************************************************
  pr.SetSubsection("main parameters");
  DOpEWrapper::FiniteElement<2> state_fe_high(
      FE_Q<2>(2 * pr.get_integer("order fe")));
  //Quadrature formulas for DWR*************************************************
  pr.SetSubsection("main parameters");
  QGauss<2> quadrature_formula_high(pr.get_integer("quad order") + 1);
  QGauss<1> face_quadrature_formula_high(pr.get_integer("facequad order") + 1);
  IDC idc_high(quadrature_formula, face_quadrature_formula);
  STH DOFH_higher_order(triangulation, state_fe_high);
  HO_DWRC dwrc(DOFH_higher_order, idc_high, "fullmem", pr,
      DOpEtypes::primal_only);
  solver.InitializeHigherOrderDWRC(dwrc);
  //**************************************************************************************************

  for (int i = 0; i < max_iter; i++)
  {
    try
    {
      solver.ReInit();
      out.ReInit();
      stringstream outp;

      outp << "**************************************************\n";
      outp << "*             Starting Forward Solve             *\n";
      outp << "*   Solving : " << P.GetName() << "\t*\n";
      outp << "*   SDoFs   : ";
      solver.StateSizeInfo(outp);
      outp << "**************************************************";
      out.Write(outp, 1, 1, 1);

      solver.ComputeReducedFunctionals();
      solver.ComputeRefinementIndicators(dwrc);

      double exact_value = 0.441956231972232;

      double error = exact_value - solver.GetFunctionalValue(LFF.GetName());
      outp << "Mean value error: " << error << "  Ieff (eh/e)= "
          << dwrc.GetError() / error << std::endl;
      out.Write(outp, 1, 1, 1);
    }
    catch (DOpEException &e)
    {
      std::cout
          << "Warning: During execution of `" + e.GetThrowingInstance()
              + "` the following Problem occurred!" << std::endl;
      std::cout << e.GetErrorMessage() << std::endl;
    }
    if (i != max_iter - 1)
    {
//      DOFH.RefineSpace("global");
      Vector<float> error_ind(dwrc.GetErrorIndicators());
      for (unsigned int i = 0; i < error_ind.size(); i++)
        error_ind[i] = std::fabs(error_ind[i]);
      DOFH.RefineSpace("optimized", &error_ind);
//      DOFH.RefineSpace("fixednumber", &error_ind, 0.4);
//      DOFH.RefineSpace("fixedfraction", &error_ind, 0.8);
    }
  }
  return 0;
}