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

#ifndef _REDUCED_IPOPT__ALGORITHM_H_
#define _REDUCED_IPOPT__ALGORITHM_H_

#include "reducedalgorithm.h"
#include "parameterreader.h"
#include "ipopt_problem.h"

#ifdef WITH_IPOPT
#include "IpIpoptApplication.hpp"
#endif

#include <iostream>
#include <assert.h>
#include <iomanip>

namespace DOpE
{
  template <typename PROBLEM, typename VECTOR, int dopedim,  int dealdim>
    class Reduced_IpoptAlgorithm : public ReducedAlgorithm<PROBLEM, VECTOR, dopedim,dealdim>
  {
  public:
    Reduced_IpoptAlgorithm(PROBLEM* OP,
			   ReducedProblemInterface<PROBLEM, VECTOR,dopedim,dealdim>* S,
			   std::string vector_behavior,
			   ParameterReader &param_reader,
			   DOpEExceptionHandler<VECTOR>* Except=NULL,
			   DOpEOutputHandler<VECTOR>* Output=NULL,
			   int base_priority=0);
    ~Reduced_IpoptAlgorithm();

    static void declare_params(ParameterReader &param_reader);

    /**
     * This solves an Optimizationproblem in only the control variable
     * using the commercial optimization library ipopt. 
     * To use it you need to define the compiler flag 
     * WITH_IPOPT and ensure that all required ipopt headers and 
     * libraries are within the path or otherwise known.
     *
     * @param q           The initial point.
     * @param global_tol  An optional parameter specifying the required  tolerance.
     *                    The actual tolerance is the maximum of this and the one specified in the param
     *                    file. Its default value is negative, so that it has no influence if not specified.
     */
    virtual int Solve(ControlVector<VECTOR>& q,double global_tol=-1.);

  protected:

  private:
    std::string _postindex;
    std::string _vector_behavior;

    double _tol;
    bool _capture_out;
    std::string _lin_solve;
  };

  /***************************************************************************************/
  /****************************************IMPLEMENTATION*********************************/
  /***************************************************************************************/
  using namespace dealii;

  /******************************************************/

template <typename PROBLEM, typename VECTOR, int dopedim,int dealdim>
void Reduced_IpoptAlgorithm<PROBLEM, VECTOR, dopedim, dealdim>::declare_params(ParameterReader &param_reader)
  {
    param_reader.SetSubsection("reduced_ipoptalgorithm parameters");
    param_reader.declare_entry("tol","1.e-5",Patterns::Double(0,1),"Tolerance");
    param_reader.declare_entry("capture ipopt output","true",Patterns::Bool(),"Select if the ipopt output should be stored in log file");
    param_reader.declare_entry("ipopt linsolve","ma27",Patterns::Selection("ma27|ma57|ma77|ma86|pardiso|wsmp|mumps"),"Linear Solver to be used in ipopt.");
    ReducedAlgorithm<PROBLEM, VECTOR, dopedim,dealdim>::declare_params(param_reader);
  }

/******************************************************/

template <typename PROBLEM, typename VECTOR, int dopedim,int dealdim>
Reduced_IpoptAlgorithm<PROBLEM, VECTOR, dopedim, dealdim>::Reduced_IpoptAlgorithm(PROBLEM* OP,
										  ReducedProblemInterface<PROBLEM, VECTOR,dopedim, dealdim>* S,
										  std::string vector_behavior,
										  ParameterReader &param_reader,
										  DOpEExceptionHandler<VECTOR>* Except,
										  DOpEOutputHandler<VECTOR>* Output,
										  int base_priority)
  : ReducedAlgorithm<PROBLEM, VECTOR,dopedim, dealdim>(OP,S,param_reader,Except,Output,base_priority)
  {

    param_reader.SetSubsection("reduced_ipoptalgorithm parameters");

    _tol        = param_reader.get_double("tol");
    _capture_out   = param_reader.get_bool("capture ipopt output");
    _lin_solve   = param_reader.get_string("ipopt linsolve");

    _vector_behavior = vector_behavior;
    _postindex = "_"+this->GetProblem()->GetName();

  }

/******************************************************/

template <typename PROBLEM, typename VECTOR, int dopedim,int dealdim>
Reduced_IpoptAlgorithm<PROBLEM, VECTOR, dopedim, dealdim>::~Reduced_IpoptAlgorithm()
  {

  }


/******************************************************/

template <typename PROBLEM, typename VECTOR, int dopedim,int dealdim>
int Reduced_IpoptAlgorithm<PROBLEM, VECTOR, dopedim, dealdim>::Solve(ControlVector<VECTOR>& q,double global_tol)
{
#ifndef WITH_IPOPT
  throw DOpEException("To use this algorithm you need to have IPOPT installed! To use this set the WITH_IPOPT CompilerFlag.","Reduced_IpoptAlgorithm::Solve");
#else 
  q.ReInit();
  
  ControlVector<VECTOR> dq(q);
  ControlVector<VECTOR> q_min(q), q_max(q);
  this->GetReducedProblem()->GetControlBoxConstraints(q_min,q_max);

  ConstraintVector<VECTOR> constraints(this->GetReducedProblem()->GetProblem()->GetSpaceTimeHandler(),_vector_behavior);
  
  unsigned int iter=0;
  double cost=0.;
  double cost_start=0.;
  std::stringstream out;
  this->GetOutputHandler()->InitNewtonOut(out);
  global_tol =  std::max(_tol,global_tol);

  out << "**************************************************\n";
  out << "*        Starting Solution using IPOPT           *\n";
  out << "*   Solving : "<<this->GetProblem()->GetName()<<"\t*\n";
  out << "*  CDoFs : ";
  q.PrintInfos(out);
  out << "*  SDoFs : ";
  this->GetReducedProblem()->StateSizeInfo(out);
  out << "*  Constraints : ";
  constraints.PrintInfos(out);
  out << "**************************************************";
  this->GetOutputHandler()->Write(out,1+this->GetBasePriority(),1,1);

  this->GetOutputHandler()->SetIterationNumber(iter,"Opt_Ipopt"+_postindex);

  this->GetOutputHandler()->Write(q,"Control"+_postindex,"control");

  try
  {
     cost_start = cost = this->GetReducedProblem()->ComputeReducedCostFunctional(q);
  }
  catch(DOpEException& e)
  {
    this->GetExceptionHandler()->HandleCriticalException(e,"Reduced_IpoptAlgorithm::Solve");
  }

  this->GetOutputHandler()->InitOut(out);
  out<< "CostFunctional: " << cost;
  this->GetOutputHandler()->Write(out,2+this->GetBasePriority());
  this->GetOutputHandler()->InitNewtonOut(out);

  /////////////////////////////////DO SOMETHING to Solve.../////////////////////////
  out<<"************************************************\n";
  out<<"*               Calling IPOPT                  *\n";
  if(_capture_out)
    out<<"*  output will be written to logfile only!     *\n";
  else
    out<<"*  output will not be written to logfile!      *\n";
  out<<"************************************************\n\n";
  this->GetOutputHandler()->Write(out,1+this->GetBasePriority());
  
  this->GetOutputHandler()->DisallowAllOutput();
  if(_capture_out)
    this->GetOutputHandler()->StartSaveCTypeOutputToLog();

  int ret_val = -1;
  {
    // Create a new instance of your nlp 
    //  (use a SmartPtr, not raw)
    //Ipopt_Problem<ReducedProblemInterface<PROBLEM,VECTOR, dopedim,dealdim>,VECTOR> 
    //  ip_prob((this->GetReducedProblem()),q,&q_min,&q_max,constraints);
    Ipopt::SmartPtr<Ipopt::TNLP> mynlp = new 
      Ipopt_Problem<ReducedProblemInterface<PROBLEM,VECTOR, dopedim,dealdim>,VECTOR>(
	ret_val, this->GetReducedProblem(),q,&q_min,&q_max,constraints);

    //Ipopt::IpoptApplication app;
    Ipopt::SmartPtr<Ipopt::IpoptApplication> app = IpoptApplicationFactory();
    // Change some options
    // Note: The following choices are only examples, they might not be
    //       suitable for your optimization problem.
    app->Options()->SetNumericValue("tol", _tol);
    app->Options()->SetStringValue("mu_strategy", "adaptive");
    app->Options()->SetStringValue("output_file", this->GetOutputHandler()->GetResultsDir()+"ipopt.out");
    app->Options()->SetStringValue("linear_solver", _lin_solve);
    app->Options()->SetStringValue("hessian_approximation","limited-memory");

    // Intialize the IpoptApplication and process the options
    Ipopt::ApplicationReturnStatus status;
    status = app->Initialize();
    if (status != Ipopt::Solve_Succeeded) {
      this->GetOutputHandler()->Write("\n\n*** Error during initialization!\n",1+this->GetBasePriority());
      abort();
    }

    // Ask Ipopt to solve the problem
    status = app->OptimizeTNLP(mynlp);
  } 
  
  if(_capture_out)
    this->GetOutputHandler()->StopSaveCTypeOutputToLog();
  this->GetOutputHandler()->ResumeOutput();
  out<<"\n************************************************\n";
  out<<"*               IPOPT Finished                 *\n";
  out<<"*          with Exit Code: "<<std::setw(3)<<ret_val;
  if(ret_val == 1)
    out<<" (success)       *\n";
  else
    out<<" (unknown error: "<<ret_val<<") *\n";
  out<<"************************************************\n";
  //FIXME What is the result...
  this->GetOutputHandler()->Write(out,1+this->GetBasePriority());

  iter++;
  this->GetOutputHandler()->SetIterationNumber(iter,"Opt_Ipopt"+_postindex);

  this->GetOutputHandler()->Write(q,"Control"+_postindex,"control");
  try
  {
     cost = this->GetReducedProblem()->ComputeReducedCostFunctional(q);
  }
  catch(DOpEException& e)
  {
    this->GetExceptionHandler()->HandleCriticalException(e,"Reduced_IpoptAlgorithm::Solve");
  }
  //We are done write total evaluation
  this->GetOutputHandler()->InitOut(out);
  out<< "CostFunctional: " << cost;
  this->GetOutputHandler()->Write(out,2+this->GetBasePriority());
  this->GetOutputHandler()->InitNewtonOut(out);
  try
    {
      this->GetReducedProblem()->ComputeReducedFunctionals(q);
    }
    catch(DOpEException& e)
    {
      this->GetExceptionHandler()->HandleCriticalException(e,"Reduced_IpoptAlgorithm::Solve");
    }

  out << "**************************************************\n";
  out << "*        Stopping Solution Using IPOPT           *\n";
  out << "*             Relative reduction in cost functional:"<<std::scientific << std::setw(11) << this->GetOutputHandler()->ZeroTolerance((cost-cost_start)/fabs(0.5*(cost_start+cost)),1.0) <<"          *\n";
  out.precision(7);
  out << "*             Final value: "<<cost<<"                                     *\n";             
  out << "**************************************************";
  this->GetOutputHandler()->Write(out,1+this->GetBasePriority(),1,1);
  return iter;
#endif //Endof ifdef WITH_IPOPT
}

/*****************************END_OF_NAMESPACE_DOpE*********************************/
}
#endif