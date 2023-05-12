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

#ifndef REDUCEDNEWTON__ALGORITHM_H_
#define REDUCEDNEWTON__ALGORITHM_H_

#include <opt_algorithms/reducedalgorithm.h>
#include <include/parameterreader.h>

#include <iostream>
#include <assert.h>
#include <iomanip>
namespace DOpE
{
  /**
   * @class ReducedNewtonAlgorithm
   *
   * This class provides a solver for equality constrained optimization
   * problems in reduced form, i.e., the dependent variable is
   * assumed to be eliminated by solving the equation. I.e.,
   * we solve the problem min j(q)
   *
   * The solution is done with a linesearch algorithm, see, e.g.,
   * Nocedal & Wright.
   *
   * @tparam <PROBLEM>    The problem container. See, e.g., OptProblemContainer
   * @tparam <VECTOR>     The vector type of the solution.
   */
  template <typename PROBLEM, typename VECTOR>
  class ReducedNewtonAlgorithm : public ReducedAlgorithm<PROBLEM, VECTOR>
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
    ReducedNewtonAlgorithm(PROBLEM *OP,
                           ReducedProblemInterface<PROBLEM, VECTOR> *S,
                           ParameterReader &param_reader,
                           DOpEExceptionHandler<VECTOR> *Except=NULL,
                           DOpEOutputHandler<VECTOR> *Output=NULL,
                           int base_priority=0);
    virtual ~ReducedNewtonAlgorithm();

    /**
     * Used to declare run time parameters. This is needed to declare all
     * parameters a startup without the need for an object to be already
     * declared.
     */
    static void declare_params(ParameterReader &param_reader);

    /**
     * This solves an Optimizationproblem in only the control variable
     * by a newtons method.
     *
     * @param q           The initial point.
     * @param global_tol  An optional parameter specifying the required  tolerance.
     *                    The actual tolerance is the maximum of this and the one specified in the param
     *                    file. Its default value is negative, so that it has no influence if not specified.
     */
    virtual int Solve(ControlVector<VECTOR> &q,double global_tol=-1.);
    /**
     * This returns the natural norm of the newton residual. This means the norm of the gradient of the
     * reduced cost functional.
     *
     * @param q           The initial point.
     */
    double NewtonResidual(const ControlVector<VECTOR> &q);

  protected:
    /**
     * Solves the linear system corresponding to the unconstrained quadratic
     * model \min_p j(q) + j'(q)p + 1/2 p^TH(q)p
     *
     * The values for j'(q) need to be provided. The hessian is not required, but
     * multiplications H(q)*d are necessary since the linearsystem is solved by
     * a CG-algorithm.
     *
     * @param q      The fixed point where j, j' is evaluated and H needs to be calculated.
     * @param gradient              The l^2 gradient of the costfunctional at q,
     *                              i.e., the gradient_i = \delta_{q_i} j(q)
     *                              where q_i denotes the i-th DoF for the control.
     * @param gradient_transposed   The transposed of the gradient. This is assumed
     *                              to be such that if q lives in a Hilbert space Q, then
     *                              (gradient_transposed,gradient)_{l^2} = \|j'(q)\|_Q^2
     * @param dq                    The solution of the model minimization, i.e.,
     *                              H(q)dq = - j'(q).
     */
    virtual int SolveReducedLinearSystem(const ControlVector<VECTOR> &q,
                                         const ControlVector<VECTOR> &gradient,
                                         const ControlVector<VECTOR> &gradient_transposed,
                                         ControlVector<VECTOR> &dq);

    /**
     * Performs an Armijo-type linesearch to find a point of sufficient descent
     * for the functional j along the direction dq.
     *
     *
     * @param dq                    The search direction.
     * @param gradient              The l^2 gradient of the costfunctional at q,
     *                              i.e., the gradient_i = \delta_{q_i} j(q)
     *                              where q_i denotes the i-th DoF for the control.
     * @param q                     The control. Needs to be the last evaluation point of
     *                              j in the begining and is at the end the updated
     *                              control q+\alpha dq.
     */
    virtual int ReducedNewtonLineSearch(const ControlVector<VECTOR> &dq,
                                        const ControlVector<VECTOR> &gradient,
                                        double &cost,
                                        ControlVector<VECTOR> &q);
    /**
     * Evaluates the squared residual, i.e., the scalar product gradient*gradient_transposed
     */
    virtual double Residual(const ControlVector<VECTOR> &gradient,
                            const ControlVector<VECTOR> &gradient_transposed)
    {
      return  gradient*gradient_transposed;
    }
  private:
    unsigned int nonlinear_maxiter_, linear_maxiter_, line_maxiter_;
    double       nonlinear_tol_, nonlinear_global_tol_, linear_tol_, linear_global_tol_, lineasearch_rho_, linesearch_c_;
    bool         compute_functionals_in_every_step_;
    std::string postindex_;
  };

  /***************************************************************************************/
  /****************************************IMPLEMENTATION*********************************/
  /***************************************************************************************/
  using namespace dealii;

  /******************************************************/

  template <typename PROBLEM, typename VECTOR>
  void ReducedNewtonAlgorithm<PROBLEM, VECTOR>::declare_params(ParameterReader &param_reader)
  {
    param_reader.SetSubsection("reducednewtonalgorithm parameters");
    param_reader.declare_entry("nonlinear_maxiter", "10",Patterns::Integer(0));
    param_reader.declare_entry("nonlinear_tol", "1.e-7",Patterns::Double(0));
    param_reader.declare_entry("nonlinear_global_tol", "1.e-11",Patterns::Double(0));

    param_reader.declare_entry("linear_maxiter", "40",Patterns::Integer(0));
    param_reader.declare_entry("linear_tol", "1.e-10",Patterns::Double(0));
    param_reader.declare_entry("linear_global_tol", "1.e-12",Patterns::Double(0));

    param_reader.declare_entry("line_maxiter", "4",Patterns::Integer(0));
    param_reader.declare_entry("linesearch_rho", "0.9",Patterns::Double(0));
    param_reader.declare_entry("linesearch_c", "0.1",Patterns::Double(0));

    param_reader.declare_entry("compute_functionals_in_every_step", "false",Patterns::Bool());

    ReducedAlgorithm<PROBLEM, VECTOR>::declare_params(param_reader);
  }
  /******************************************************/

  template <typename PROBLEM, typename VECTOR>
  ReducedNewtonAlgorithm<PROBLEM, VECTOR>::ReducedNewtonAlgorithm(PROBLEM *OP,
      ReducedProblemInterface<PROBLEM, VECTOR> *S,
      ParameterReader &param_reader,
      DOpEExceptionHandler<VECTOR> *Except,
      DOpEOutputHandler<VECTOR> *Output,
      int base_priority)
    : ReducedAlgorithm<PROBLEM, VECTOR>(OP,S,param_reader,Except,Output,base_priority)
  {

    param_reader.SetSubsection("reducednewtonalgorithm parameters");
    nonlinear_maxiter_    = param_reader.get_integer ("nonlinear_maxiter");
    nonlinear_tol_        = param_reader.get_double ("nonlinear_tol");
    nonlinear_global_tol_ = param_reader.get_double ("nonlinear_global_tol");

    linear_maxiter_       = param_reader.get_integer ("linear_maxiter");
    linear_tol_           = param_reader.get_double ("linear_tol");
    linear_global_tol_    = param_reader.get_double ("linear_global_tol");

    line_maxiter_         = param_reader.get_integer ("line_maxiter");
    lineasearch_rho_       = param_reader.get_double ("linesearch_rho");
    linesearch_c_         = param_reader.get_double ("linesearch_c");

    compute_functionals_in_every_step_  = param_reader.get_bool ("compute_functionals_in_every_step");

    postindex_ = "_"+this->GetProblem()->GetName();
  }

  /******************************************************/

  template <typename PROBLEM, typename VECTOR>
  ReducedNewtonAlgorithm<PROBLEM, VECTOR>::~ReducedNewtonAlgorithm()
  {

  }

  /******************************************************/

  template <typename PROBLEM, typename VECTOR>
  double ReducedNewtonAlgorithm<PROBLEM, VECTOR>::NewtonResidual(const ControlVector<VECTOR> &q)
  {
    //Solve j'(q) = 0
    ControlVector<VECTOR> gradient(q), gradient_transposed(q);

    try
      {
        this->GetReducedProblem()->ComputeReducedCostFunctional(q);
      }
    catch (DOpEException &e)
      {
        this->GetExceptionHandler()->HandleCriticalException(e,"ReducedNewtonAlgorithm::NewtonResidual");
      }

    try
      {
        this->GetReducedProblem()->ComputeReducedGradient(q,gradient,gradient_transposed);
      }
    catch (DOpEException &e)
      {
        this->GetExceptionHandler()->HandleCriticalException(e,"ReducedNewtonAlgorithm::NewtonResidual");
      }

    return sqrt(Residual(gradient,gradient_transposed));
  }

  /******************************************************/

  template <typename PROBLEM, typename VECTOR>
  int ReducedNewtonAlgorithm<PROBLEM, VECTOR>::Solve(ControlVector<VECTOR> &q,double global_tol)
  {

    q.ReInit();
    //Solve j'(q) = 0
    ControlVector<VECTOR> dq(q), gradient(q), gradient_transposed(q);

    unsigned int iter=0;
    double cost=0.;
    std::stringstream out;
    this->GetOutputHandler()->InitNewtonOut(out);

    out << "**************************************************\n";
    out << "*        Starting Reduced Newton Algorithm       *\n";
    out << "*   Solving : "<<this->GetProblem()->GetName()<<"\t*\n";
    out << "*  CDoFs : ";
    q.PrintInfos(out);
    out << "*  SDoFs : ";
    this->GetReducedProblem()->StateSizeInfo(out);
    out << "**************************************************";
    this->GetOutputHandler()->Write(out,1+this->GetBasePriority(),1,1);

    this->GetOutputHandler()->SetIterationNumber(iter,"OptNewton"+postindex_);

    this->GetOutputHandler()->Write(q,"Control"+postindex_,"control");

    try
      {
        cost = this->GetReducedProblem()->ComputeReducedCostFunctional(q);
      }
    catch (DOpEException &e)
      {
        this->GetExceptionHandler()->HandleCriticalException(e,"ReducedNewtonAlgorithm::Solve");
      }

    out<< "CostFunctional: " << cost;
    this->GetOutputHandler()->Write(out,2+this->GetBasePriority());

    if (compute_functionals_in_every_step_ == true)
      {
        try
          {
            this->GetReducedProblem()->ComputeReducedFunctionals(q);
          }
        catch (DOpEException &e)
          {
            this->GetExceptionHandler()->HandleCriticalException(e);
          }
      }

    try
      {
        this->GetReducedProblem()->ComputeReducedGradient(q,gradient,gradient_transposed);
      }
    catch (DOpEException &e)
      {
        this->GetExceptionHandler()->HandleCriticalException(e,"ReducedNewtonAlgorithm::Solve");
      }

    double res = Residual(gradient,gradient_transposed);//gradient*gradient_transposed;
    double firstres = res;

    assert(res >= 0);

    this->GetOutputHandler()->Write(gradient,"NewtonResidual"+postindex_,"control");
    out<< "\t Newton step: " <<iter<<"\t Residual (abs.): "<<sqrt(res)<<"\n";
    out<< "\t Newton step: " <<iter<<"\t Residual (rel.): "<<std::scientific<<sqrt(res)/sqrt(res)<<"\n";
    this->GetOutputHandler()->Write(out,3+this->GetBasePriority());
    int liniter = 0;
    int lineiter =0;
    unsigned int miniter = 0;
    if (global_tol > 0.)
      miniter = 1;

    global_tol =  std::max(nonlinear_global_tol_,global_tol);
    while (( (res >= global_tol*global_tol) && (res >= nonlinear_tol_*nonlinear_tol_*firstres) ) ||  iter < miniter )
      {
        iter++;
        this->GetOutputHandler()->SetIterationNumber(iter,"OptNewton"+postindex_);

        if (iter > nonlinear_maxiter_)
          {
            throw DOpEIterationException("Iteration count exceeded bounds!","ReducedNewtonAlgorithm::Solve");
          }

        //Compute a search direction
        try
          {
            liniter = SolveReducedLinearSystem(q,gradient,gradient_transposed, dq);
          }
        catch (DOpEIterationException &e)
          {
            //Seems uncritical too many linear solves, it'll probably work
            //So only write a warning, and continue.
            this->GetExceptionHandler()->HandleException(e,"ReducedNewtonAlgorithm::Solve");
            liniter = -1;
	    //However, if in this case the step is inconveniently large, i.e., it might
	    //be an almost singular Hessian, we take the negative gradient instead.
	    if (dq.Norm("infty") > 10000.*gradient_transposed.Norm("infty"))
	    {
   	      this->GetOutputHandler()->WriteError("Step discarded, taking negative Gradient instead.");
	      dq = gradient_transposed;
	      dq *= -1.;
	    }
          }
        catch (DOpENegativeCurvatureException &e)
          {
            this->GetExceptionHandler()->HandleException(e,"ReducedNewtonAlgorithm::Solve");
            liniter = -2;
          }
        catch (DOpEException &e)
          {
            this->GetExceptionHandler()->HandleCriticalException(e,"ReducedNewtonAlgorithm::Solve");
          }
        //Linesearch
        try
          {
	    //Check if dq is a descent direction
	    double reduction = gradient*dq;
	    if(reduction > 0)
	    {
	      this->GetOutputHandler()->WriteError("Waring: computed direction doesn't seem to be a descend direction! Trying negative gradient instead.");
	      dq = gradient_transposed;
	      dq *= -1.;
	    }
            lineiter = ReducedNewtonLineSearch(dq,gradient,cost,q);
          }
        catch (DOpEIterationException &e)
          {
            //Seems uncritical too many line search steps, it'll probably work
            //So only write a warning, and continue.
            this->GetExceptionHandler()->HandleException(e,"ReducedNewtonAlgorithm::Solve");
            lineiter = -1;
          }
        //catch(DOpEException& e)
        //{
        //  this->GetExceptionHandler()->HandleCriticalException(e);
        //}

        out<< "CostFunctional: " << cost;
        this->GetOutputHandler()->Write(out,3+this->GetBasePriority());

        if (compute_functionals_in_every_step_ == true)
          {
            try
              {
                this->GetReducedProblem()->ComputeReducedFunctionals(q);
              }
            catch (DOpEException &e)
              {
                this->GetExceptionHandler()->HandleCriticalException(e);
              }
          }


        //Prepare the next Iteration
        try
          {
            this->GetReducedProblem()->ComputeReducedGradient(q,gradient,gradient_transposed);
          }
        catch (DOpEException &e)
          {
            this->GetExceptionHandler()->HandleCriticalException(e,"ReducedNewtonAlgorithm::Solve");
          }

        this->GetOutputHandler()->Write(q,"Control"+postindex_,"control");
        this->GetOutputHandler()->Write(gradient,"NewtonResidual"+postindex_,"control");

        res = Residual(gradient,gradient_transposed);//gradient*gradient_transposed;

        out<<"\t Newton step: " <<iter<<"\t Residual (rel.): "<<this->GetOutputHandler()->ZeroTolerance(sqrt(res)/sqrt(firstres),1.0)<< "\t LinearIters ["<<liniter<<"]\t LineSearch {"<<lineiter<<"} ";
        this->GetOutputHandler()->Write(out,3+this->GetBasePriority());
      }

    //We are done write total evaluation
    out<< "CostFunctional: " << cost;
    this->GetOutputHandler()->Write(out,2+this->GetBasePriority());
    try
      {
        this->GetReducedProblem()->ComputeReducedFunctionals(q);
      }
    catch (DOpEException &e)
      {
        this->GetExceptionHandler()->HandleCriticalException(e,"ReducedNewtonAlgorithm::Solve");
      }

    out << "**************************************************\n";
    out << "*        Stopping Reduced Newton Algorithm       *\n";
    out << "*             after "<<std::setw(6)<<iter<<"  Iterations           *\n";
    out.precision(4);
    out << "*             with rel. Residual "<<std::scientific << std::setw(11) << this->GetOutputHandler()->ZeroTolerance(sqrt(res)/sqrt(firstres),1.0)<<"          *\n";
    out.precision(10);
    out << "**************************************************";
    this->GetOutputHandler()->Write(out,1+this->GetBasePriority(),1,1);
    return iter;
  }

  /******************************************************/

  template <typename PROBLEM, typename VECTOR>
  int ReducedNewtonAlgorithm<PROBLEM, VECTOR>::SolveReducedLinearSystem(const ControlVector<VECTOR> &q,
      const ControlVector<VECTOR> &gradient,
      const ControlVector<VECTOR> &gradient_transposed,
      ControlVector<VECTOR> &dq)
  {
    std::stringstream out;
    dq = 0.;
    ControlVector<VECTOR> r(q), r_transposed(q),  d(q), Hd(q), Hd_transposed(q);

    r            = gradient;
    r_transposed = gradient_transposed;
    d.equ(-1,gradient_transposed);

    double res = Residual(r,r_transposed);//r*r_transposed;
    double firstres = res;

    assert(res >= 0.);

    out << "Starting Reduced Linear Solver with Residual: "<<sqrt(res);
    this->GetOutputHandler()->Write(out,4+this->GetBasePriority());

    unsigned int iter = 0;
    double cgalpha, cgbeta, oldres;

    this->GetOutputHandler()->SetIterationNumber(iter,"OptNewtonCg"+postindex_);

    //while(res>=linear_tol_*linear_tol_*firstres && res>=linear_global_tol_*linear_global_tol_)
    //using Algorithm 6.1 from Nocedal Wright
    while (res>= std::min(0.25,sqrt(firstres))*firstres && res>=linear_global_tol_*linear_global_tol_)
      {
        iter++;
        this->GetOutputHandler()->SetIterationNumber(iter,"OptNewtonCg"+postindex_);
        if (iter > linear_maxiter_)
          {
            throw DOpEIterationException("Iteration count exceeded bounds!","ReducedNewtonAlgorithm::SolveReducedLinearSystem");
          }

        try
          {
            this->GetReducedProblem()->ComputeReducedHessianVector(q,d,Hd,Hd_transposed);
          }
        catch (DOpEException &e)
          {
            this->GetExceptionHandler()->HandleCriticalException(e,"ReducedNewtonAlgorithm::SolveReducedLinearSystem");
          }

        cgalpha = res / (Hd*d);

        if (cgalpha < 0)
          {
            if (iter==1)
              {
                dq.add(cgalpha,d);
              }
            throw DOpENegativeCurvatureException("Negative curvature detected!","ReducedNewtonAlgorithm::SolveReducedLinearSystem");
          }

        dq.add(cgalpha,d);
        r.add(cgalpha,Hd);
        r_transposed.add(cgalpha,Hd_transposed);

        oldres = res;
        res = Residual(r,r_transposed);//r*r_transposed;
        if (res < 0.)
          {
            //something is broken, maybe don't use update formula and
            //calculate res from scratch.
            try
              {
                this->GetReducedProblem()->ComputeReducedHessianVector(q,dq,Hd,Hd_transposed);
              }
            catch (DOpEException &e)
              {
                this->GetExceptionHandler()->HandleCriticalException(e);
              }
            r = gradient;
            r_transposed = gradient_transposed;
            r.add(1.,Hd);
            r_transposed.add(1.,Hd_transposed);
            res = Residual(r,r_transposed);
          }
        if (res < 0. && ( res > -1. *linear_global_tol_*linear_global_tol_ || res > -1.*std::min(0.25,sqrt(firstres))*firstres) )
          {
            //Ignore the wrong sign, it may be due to cancellations, and we would stop now.
            //This is precisely what will happen if we just `correct' the sign of res
            res = fabs(res);
            out<<"\t There seem to be cancellation errors accumulating in the Residual,"<<std::endl;
            out<<"\t and its norm gets negative. Since it is below the tolerance, we stop the iteration.";
            this->GetOutputHandler()->Write(out,4+this->GetBasePriority());
          }
        assert(res >= 0.);
        out<<"\t Cg step: " <<iter<<"\t Residual: "<<sqrt(res);
        this->GetOutputHandler()->Write(out,4+this->GetBasePriority());

        cgbeta = res / oldres; //Fletcher-Reeves
        d*= cgbeta;
        d.add(-1,r_transposed);
      }
    return iter;
  }


  /******************************************************/

  template <typename PROBLEM, typename VECTOR>
  int ReducedNewtonAlgorithm<PROBLEM, VECTOR>::ReducedNewtonLineSearch(const ControlVector<VECTOR> &dq,
      const ControlVector<VECTOR>  &gradient,
      double &cost,
      ControlVector<VECTOR> &q)
  {
    double rho = lineasearch_rho_;
    double c   = linesearch_c_;

    double costnew = 0.;
    bool force_linesearch = false;

    q+=dq;
    try
      {
        costnew = this->GetReducedProblem()->ComputeReducedCostFunctional(q);
      }
    catch (DOpEException &e)
      {
//    this->GetExceptionHandler()->HandleException(e);
        force_linesearch = true;
        this->GetOutputHandler()->Write("Computing Cost Failed",4+this->GetBasePriority());
      }

    double alpha=1;
    unsigned int iter =0;

    double reduction = gradient*dq;
    if (reduction > 0)
      {
        this->GetOutputHandler()->WriteError("Waring: computed direction doesn't seem to be a descend direction!");
        reduction = 0;
      }

    if (line_maxiter_ > 0)
      {
        if (fabs(reduction) < 1.e-10*cost)
          reduction = 0.;
        if (std::isinf(costnew) || std::isnan(costnew) || (costnew >= cost + c*alpha*reduction) || force_linesearch)
          {
            this->GetOutputHandler()->Write("\t linesearch ",4+this->GetBasePriority());
            while (std::isinf(costnew) || std::isnan(costnew) || (costnew >= cost + c*alpha*reduction) || force_linesearch)
              {
                iter++;
                if (iter > line_maxiter_)
                  {
                    if (force_linesearch)
                      {
                        throw DOpEException("Iteration count exceeded bounds while unable to compute the CostFunctional!","ReducedNewtonAlgorithm::ReducedNewtonLineSearch");
                      }
                    else
                      {
                        cost = costnew;
                        throw DOpEIterationException("Iteration count exceeded bounds!","ReducedNewtonAlgorithm::ReducedNewtonLineSearch");
                      }
                  }
                force_linesearch = false;
                q.add(alpha*(rho-1.),dq);
                alpha *= rho;

                try
                  {
                    costnew = this->GetReducedProblem()->ComputeReducedCostFunctional(q);
                  }
                catch (DOpEException &e)
                  {
                    //this->GetExceptionHandler()->HandleException(e);
                    force_linesearch = true;
                    this->GetOutputHandler()->Write("Computing Cost Failed",4+this->GetBasePriority());
                  }
              }
          }
        cost = costnew;
      }

    return iter;

  }


}
#endif
