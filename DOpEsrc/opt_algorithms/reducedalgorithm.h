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

#ifndef REDUCED_ALGORITHM_H_
#define REDUCED_ALGORITHM_H_

#include "optproblemcontainer.h"
#include "reducedprobleminterface.h"
#include "dopeexceptionhandler.h"
#include "outputhandler.h"
#include "controlvector.h"

#include <lac/vector.h>

#include <iostream>
#include <assert.h>
#include <iomanip>

namespace DOpE
{

  /**
   * @class ReducedAlgorithm
   *
   * This class is the base class for solvers for equality constrained optimization 
   * problems in reduced form, i.e., the dependent variable is 
   * assumed to be eliminated by solving the equation. I.e.,
   * we solve the problem min j(q)
   *
   * @tparam <PROBLEM>    The problem container. See, e.g., OptProblemContainer
   * @tparam <VECTOR>     The vector type of the solution.
   */
  template<typename PROBLEM, typename VECTOR>
    class ReducedAlgorithm
  {
  public:
    /**
     * The constructor for the algorithm
     *
     * @param OP              A pointer to the problem container
     * @param S               The reduced problem. This object handles the equality 
     *                        constraint. For the interface see ReducedProblemInterface.
     * @param param_reader    A parameter reader to access user given runtime parameters.
     * @param Except          The DOpEExceptionHandler. This is used to handle the output 
     *                        by all exception.
     * @param Output          The DOpEOutputHandler. This takes care of all output 
     *                        generated by the problem.
     * @param base_priority   An offset for the priority of the output generated by the algorithm.
     */
    ReducedAlgorithm(PROBLEM* OP,
		     ReducedProblemInterface<PROBLEM,VECTOR>* S,
		     ParameterReader &param_reader,
		     DOpEExceptionHandler<VECTOR>* Except = NULL,
		     DOpEOutputHandler<VECTOR>* Output = NULL, int base_priority = 0);
    virtual ~ReducedAlgorithm();
    
    /**
     * Used to declare run time parameters. This is needed to declare all
     * parameters a startup without the need for an object to be already 
     * declared.
     */
    static void
      declare_params(ParameterReader &param_reader);
    
    /**
     * Needs to be called once after changing the discretization,
     * e.g., due to mesh changes, to reinitialize all dependent objects.
     */
    virtual void
      ReInit()
    {
      this->GetReducedProblem()->ReInit();
      if (rem_output_)
      {
	this->GetOutputHandler()->ReInit();
      }
    }
    
    /**
     * Common interface for all algorithms solving a reduced problem.
     *
     * @param q           The initial point.
     * @param global_tol  An optional parameter specifying the required  tolerance.
     *                    The actual tolerance is the maximum of this and the one specified in the param
     *                    file. Its default value is negative, so that it has no influence if not specified.
     */
    virtual int
      Solve(ControlVector<VECTOR>& q, double global_tol = -1.)=0;
    
   /**
     * This just evaluates j(q). And can be used to solve PDEs that do not
     * require any control. 
     *
     * @param q           The initial point.
     */
    virtual void
      SolveForward(ControlVector<VECTOR>& q);
    
    /**
     * This function calculates j'(q)dq, i.e., the directional derivative
     * and difference quotients for comparison to allow checking of the 
     * implementation of all things related to the first derivative of the 
     * functional once it is confirmed that functional evaluations are correct.
     *
     * @double c          The direction will be initialized to the constant value c.
     * @param q           The point at which we calculate the derivative.
     * @param dq          Storage or the direction.
     * @param niter       Number of difference quotient evaluations, i.e., how many times
     *                    eps is reduced.
     * @param eps         The initial value for eps in the difference quotients.
     */
     virtual void
      CheckGrads(double c, ControlVector<VECTOR>& q,
		 ControlVector<VECTOR>& dq, unsigned int niter = 1, double eps = 1.);
    /**
     * Is called to calculate first difference quotients
     *
     * @double exact      The value of j'(q)
     * @param eps         The value for eps in the difference quotient .
     * @param q           The point at which we try to approximate the derivative.
     * @param dq          The direction.
     */
     virtual void
      FirstDifferenceQuotient(double exact, double eps,
			      const ControlVector<VECTOR>& q, const ControlVector<VECTOR>& dq);
    /**
     * This function calculates dq*H(q)dq , i.e., some diagonal entry of the hessian
     * and corresponting difference quotients for comparison.
     *
     * @double c          The direction will be initialized to the constant value c.
     * @param q           The point at which we calculate the derivative.
     * @param dq          Storage or the direction.
     * @param niter       Number of difference quotient evaluations, i.e., how many times
     *                    eps is reduced.
     * @param eps         The initial value for eps in the difference quotients.
     */
    virtual void
      CheckHessian(double c, ControlVector<VECTOR>& q,
		   ControlVector<VECTOR>& dq, unsigned int niter = 1, double eps = 1.);
    /**
     * Is called to calculate second difference quotients
     *
     * @double exact      The value of j'(q)
     * @param eps         The value for eps in the difference quotient .
     * @param q           The point at which we try to approximate the derivative.
     * @param dq          The direction.
     */
    virtual void
      SecondDifferenceQuotient(double exact, double eps,
			       const ControlVector<VECTOR>& q, const ControlVector<VECTOR>& dq);
    
    DOpEExceptionHandler<VECTOR>*
      GetExceptionHandler()
    {
      return ExceptionHandler_;
    }
    DOpEOutputHandler<VECTOR>*
      GetOutputHandler()
    {
      return OutputHandler_;
        }
  protected:
    /**
     * Grants access to the optimization problem container
     */
    PROBLEM*
      GetProblem()
    {
      return OP_;
    }
    /**
     * Grants access to the optimization problem container
     */
    const PROBLEM*
      GetProblem() const
    {
      return OP_;
    }
    /**
     * Grants access to the reduced optimization problem
     */
    const  ReducedProblemInterface<PROBLEM,VECTOR>*
      GetReducedProblem() const
    {
      return Solver_;
    }
    /**
     * Grants access to the reduced optimization problem
     */
    ReducedProblemInterface<PROBLEM,VECTOR>*
      GetReducedProblem()
    {
          return Solver_;
    }
    
    int
      GetBasePriority() const
    {
      return base_priority_;
    }
    
  private:
    PROBLEM* OP_;
    ReducedProblemInterface<PROBLEM,VECTOR>* Solver_;
    DOpEExceptionHandler<VECTOR>* ExceptionHandler_;
    DOpEOutputHandler<VECTOR>* OutputHandler_;
    bool rem_exception_;
    bool rem_output_;
    int base_priority_;
  };
  
  /***************************************************************************************/
  /****************************************IMPLEMENTATION*********************************/
  /***************************************************************************************/
  using namespace dealii;

  /******************************************************/

  template<typename PROBLEM, typename VECTOR>
    void
    ReducedAlgorithm<PROBLEM, VECTOR>::declare_params(
        ParameterReader &param_reader)
    {
      DOpEOutputHandler<VECTOR>::declare_params(param_reader);
    }

  /******************************************************/

  template<typename PROBLEM, typename VECTOR>
    ReducedAlgorithm<PROBLEM, VECTOR>::ReducedAlgorithm(
        PROBLEM* OP,  ReducedProblemInterface<PROBLEM,VECTOR>* S,
        ParameterReader &param_reader, DOpEExceptionHandler<VECTOR>* Except,
        DOpEOutputHandler<VECTOR>* Output, int base_priority)
    {
      assert(OP);
      assert(S);

      OP_ = OP;
      Solver_ = S;
      if (Output == NULL)
      {
        OutputHandler_ = new DOpEOutputHandler<VECTOR>(S, param_reader);
        rem_output_ = true;
      }
      else
      {
        OutputHandler_ = Output;
        rem_output_ = false;
      }
      if (Except == NULL)
      {
        ExceptionHandler_ = new DOpEExceptionHandler<VECTOR>(OutputHandler_);
        rem_exception_ = true;
      }
      else
      {
        ExceptionHandler_ = Except;
        rem_exception_ = false;
      }
      OP_->RegisterOutputHandler(OutputHandler_);
      OP_->RegisterExceptionHandler(ExceptionHandler_);
      Solver_->RegisterOutputHandler(OutputHandler_);
      Solver_->RegisterExceptionHandler(ExceptionHandler_);

      base_priority_ = base_priority;
    }

  /******************************************************/

  template<typename PROBLEM, typename VECTOR>
    ReducedAlgorithm<PROBLEM, VECTOR>::~ReducedAlgorithm()
    {
      if (ExceptionHandler_ && rem_exception_)
      {
        delete ExceptionHandler_;
      }
      if (OutputHandler_ && rem_output_)
      {
        delete OutputHandler_;
      }
    }
  /******************************************************/

  template<typename PROBLEM, typename VECTOR>
    void
    ReducedAlgorithm<PROBLEM, VECTOR>::SolveForward(
        ControlVector<VECTOR>& q)
    {
      q.ReInit();

      //Solve j'(q) = 0
      double cost = 0.;
      std::stringstream out;

      out << "**************************************************\n";
      out << "*             Starting Forward Solver            *\n";
      out << "*   Solving : " << this->GetProblem()->GetName() << "\t*\n";
      out << "*  CDoFs : ";
      q.PrintInfos(out);
      out << "*  SDoFs : ";
      this->GetReducedProblem()->StateSizeInfo(out);
      out << "**************************************************";
      this->GetOutputHandler()->Write(out, 1 + this->GetBasePriority(), 1, 1);

      try
      {
        cost = this->GetReducedProblem()->ComputeReducedCostFunctional(q);
      }
      catch (DOpEException& e)
      {
        this->GetExceptionHandler()->HandleCriticalException(e);
      }

      out << "CostFunctional: " << cost;
      this->GetOutputHandler()->Write(out, 2 + this->GetBasePriority());

      try
      {
        this->GetReducedProblem()->ComputeReducedFunctionals(q);
      }
      catch (DOpEException& e)
      {
        this->GetExceptionHandler()->HandleCriticalException(e);
      }

    }
  /******************************************************/

  template<typename PROBLEM, typename VECTOR>
    void
    ReducedAlgorithm<PROBLEM, VECTOR>::CheckGrads(double c,
        ControlVector<VECTOR>& q, ControlVector<VECTOR>& dq, unsigned int niter,
        double eps)
    {
      q.ReInit();
      dq.ReInit();

      dq = c;

      ControlVector<VECTOR> point(q);
      point = q;
      std::stringstream out;

      ControlVector<VECTOR> gradient(q), gradient_transposed(q);

      this->GetReducedProblem()->ComputeReducedCostFunctional(point);
      this->GetReducedProblem()->ComputeReducedGradient(point, gradient,
          gradient_transposed);
      double cost_diff = gradient * dq;
      out << "Checking Gradients...." << std::endl;
      out << " Epsilon \t Exact \t Diff.Quot. \t Rel. Error ";
      this->GetOutputHandler()->Write(out, 3 + this->GetBasePriority());

      for (unsigned int i = 0; i < niter; i++)
      {
        FirstDifferenceQuotient(cost_diff, eps, q, dq);
        eps /= 10.;
      }
    }
  /******************************************************/

  template<typename PROBLEM, typename VECTOR>
    void
    ReducedAlgorithm<PROBLEM, VECTOR>::FirstDifferenceQuotient(
        double exact, double eps, const ControlVector<VECTOR>& q,
        const ControlVector<VECTOR>& dq)
    {
      ControlVector<VECTOR> point(q);
      point = q;

      std::stringstream out;

      point.add(eps, dq);

      double cost_right = 0.;
      //Differenzenquotient
      cost_right = this->GetReducedProblem()->ComputeReducedCostFunctional(
          point);

      point.add(-2. * eps, dq);

      double cost_left = 0.;
      //Differenzenquotient
      cost_left = this->GetReducedProblem()->ComputeReducedCostFunctional(
          point);

      double diffquot = (cost_right - cost_left) / (2. * eps);
      out << eps << "\t" << exact << "\t" << diffquot << "\t"
          << (exact - diffquot) / exact << std::endl;
      this->GetOutputHandler()->Write(out, 3 + this->GetBasePriority());
    }

  /******************************************************/

  template<typename PROBLEM, typename VECTOR>
    void
    ReducedAlgorithm<PROBLEM, VECTOR>::CheckHessian(double c,
        ControlVector<VECTOR>& q, ControlVector<VECTOR>& dq, unsigned int niter,
        double eps)
    {
      q.ReInit();
      dq.ReInit();

      dq = c;

      ControlVector<VECTOR> point(q);
      point = q;
      std::stringstream out;

      ControlVector<VECTOR> gradient(q), gradient_transposed(q), hessian(q),
          hessian_transposed(q);

      this->GetReducedProblem()->ComputeReducedCostFunctional(point);
      this->GetReducedProblem()->ComputeReducedGradient(point, gradient,
          gradient_transposed);

      this->GetReducedProblem()->ComputeReducedHessianVector(point, dq, hessian,
          hessian_transposed);

      double cost_diff = hessian * dq;
      out << "Checking Hessian...." << std::endl;
      out << " Epsilon \t Exact \t Diff.Quot. \t Rel. Error ";
      this->GetOutputHandler()->Write(out, 3 + this->GetBasePriority());

      for (unsigned int i = 0; i < niter; i++)
      {
        SecondDifferenceQuotient(cost_diff, eps, q, dq);
        eps /= 10.;
      }
    }
  /******************************************************/

  template<typename PROBLEM, typename VECTOR>
    void
    ReducedAlgorithm<PROBLEM, VECTOR>::SecondDifferenceQuotient(
        double exact, double eps, const ControlVector<VECTOR>& q,
        const ControlVector<VECTOR>& dq)
    {
      ControlVector<VECTOR> point(q);
      point = q;
      std::stringstream out;

      double cost_mid = 0.;
      //Differenzenquotient
      cost_mid = this->GetReducedProblem()->ComputeReducedCostFunctional(point);

      point.add(eps, dq);

      double cost_right = 0.;
      //Differenzenquotient
      cost_right = this->GetReducedProblem()->ComputeReducedCostFunctional(
          point);

      point.add(-2. * eps, dq);

      double cost_left = 0.;
      //Differenzenquotient
      cost_left = this->GetReducedProblem()->ComputeReducedCostFunctional(
          point);

      double diffquot = (cost_left - 2. * cost_mid + cost_right) / (eps * eps);

      out << eps << "\t" << exact << "\t" << diffquot << "\t"
          << (exact - diffquot) / exact << std::endl;
      this->GetOutputHandler()->Write(out, 3 + this->GetBasePriority());
    }

/******************************************************/

}
#endif
