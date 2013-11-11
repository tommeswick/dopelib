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

#ifndef _PDE_INTERFACE_H_
#define _PDE_INTERFACE_H_

#include <map>
#include <string>

#include <fe/fe_system.h>
#include <fe/fe_values.h>
#include <fe/mapping.h>
#include <lac/full_matrix.h>
#include <base/function.h>

#include "fevalues_wrapper.h"
#include "celldatacontainer.h"
#include "facedatacontainer.h"
#include "multimesh_celldatacontainer.h"
#include "multimesh_facedatacontainer.h"

namespace DOpE
{

  /**
   * A template providing all evaluations of a PDE that may be used
   * during the solution of a PDE or an optimization with a PDE constraint.
   *
   */
  template<
      template<template<int, int> class DH, typename VECTOR, int dealdim> class CDC,
      template<template<int, int> class DH, typename VECTOR, int dealdim> class FDC,
      template<int, int> class DH, typename VECTOR,  int dealdim>
    class PDEInterface
    {
      public:
        PDEInterface();
        virtual
        ~PDEInterface();

        /******************************************************/

        /**
         * Assuming that the PDE is given in the form a(u;\phi) = f(\phi),
	 * this function implements all terms in a(u;\phi) that are
	 * represented by intergrals over elements T.
	 * Hence, if a(u;\phi) = \sum_T \int_T a_T(u;\phi) + ...
	 * then this function needs to implement \int_T a_T(u;\phi)
	 * a_T may depend upon any spatial derivatives, but not on temporal 
	 * derivatives.
	 * 
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 * @param scale_ico          A special scaling parameter to be used 
	 *                           in certain parts of the equation
	 *                           if they need to be treated differently in 
	 *                           time-stepping schemes, see the PDF-documentation
	 *                           for more details.
	 *                           
         */
        virtual void
	  CellEquation(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
		       dealii::Vector<double> &/*local_cell_vector*/, 
		       double /*scale*/,
		       double /*scale_ico*/);

        /******************************************************/

        /**
         * This function is used for error estimation and should implement
	 * the strong form of the residual on an element T.
	 * 
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param cdc_wight          The CellDataContainer for the weight-function,
	 *                           e.g., the testfunction by which the
	 *                           residual needs to be multiplied
	 * @param ret                The value of the integral on the element 
	 *                           of residual times testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 *                           
         */
        virtual void
        StrongCellResidual(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
			   const CDC<DH, VECTOR, dealdim>& /*cdc_weight*/, 
			   double& /*ret*/, 
			   double /*scale*/);

        /******************************************************/

        /**
         * Assuming that the discretization of temporal derivatives by a backward 
	 * difference, i.e., \partial_t u(t_i) \approx 1/\delta t ( u(t_i) - u(t_{i-1})
	 * in several cases the the temporal derivatives of the
	 * equation give rise to a spacial integral of the form 
	 *
	 * \int_Omega T(u(t_i); \phi(t_i)) - \int_Omega T(u(t_{i-1}); \phi(t_{i-1}))
	 *
	 * This equation is used to implement the element contribution
	 * \int_T T(u,\phi)
	 * 
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 */
        //Note that the _UU term is not needed, since we assume that CellTimeEquation is linear!
        virtual void
	  CellTimeEquation(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
			   dealii::Vector<double> &/*local_cell_vector*/, 
			   double /*scale*/);

        /******************************************************/
        /**
         * Same as CellTimeEquation, but here the derivative of T with
	 * respect to u is considered.
	 * Here, the derivative of T in u in a direction du 
	 * for a fixed test function z
	 * is denoted as T'(u;du,z)
	 *
	 * This equation is used to implement the element contribution
	 * \int_T T'(u;\phi,z)
	 * 
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 */
	virtual void
	  CellTimeEquation_U(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
			     dealii::Vector<double> &/*local_cell_vector*/, 
			     double /*scale*/);

        /******************************************************/
        /**
         * Same as CellTimeEquation_U, but we exchange the argument for
	 * the test function.
	 *
	 * This equation is used to implement the element contribution
	 * \int_T T'(u;du,\phi)
	 * 
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 */  
	virtual void
	  CellTimeEquation_UT(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
			      dealii::Vector<double> &/*local_cell_vector*/,
			      double /*scale*/);

        /******************************************************/
        /**
         * Same as CellTimeEquation_UT, but we exchange the argument for
	 * the test function.
	 *
	 * This equation is used to implement the element contribution
	 * \int_T T'(u;\phi,dz)
	 * 
	 * Note that this is the same function as in CellTimeEquation_U,
	 * but it is used with an other argument dz instead of z.
	 * 
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 */
        virtual void
	  CellTimeEquation_UTT(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
			       dealii::Vector<double> &/*local_cell_vector*/, 
			       double /*scale*/);

        /******************************************************/

	/**
	 * In certain cases, the assumption of CellTimeEquation 
	 * are not meet, i.e., we can not use the 
	 * same operator T at t_i and t_{i-1}. 
	 * In these cases instead of CellTimeEquation this 
	 * funtion is used, where the user can implement the
	 * complete approximation of the temporal derivative.
	 *
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 */
        virtual void
	  CellTimeEquationExplicit(const CDC<DH, VECTOR, dealdim>& /*cdc**/,
				   dealii::Vector<double> &/*local_cell_vector*/, 
				   double /*scale*/);
        /******************************************************/
	/**
	 * Analog to CellTimeEquationExplicit, this function is used 
	 * to replace CellTimeEquation_U if needed.
	 *
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 */
	virtual void
	  CellTimeEquationExplicit_U(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
				     dealii::Vector<double> &/*local_cell_vector*/, 
				     double /*scale*/);

        /******************************************************/
	/**
	 * Analog to CellTimeEquationExplicit, this function is used 
	 * to replace CellTimeEquation_UT if needed.
	 *
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 */

        virtual void
	  CellTimeEquationExplicit_UT(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
				      dealii::Vector<double> &/*local_cell_vector*/, 
				      double /*scale*/);

        /******************************************************/
	/**
	 * Analog to CellTimeEquationExplicit, this function is used 
	 * to replace CellTimeEquation_UTT if needed.
	 *
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 */
        virtual void
	  CellTimeEquationExplicit_UTT(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
				       dealii::Vector<double> &/*local_cell_vector*/, 
				       double /*scale*/);

        /******************************************************/
	/**
	 * For nonlinear terms involved in the temporal derivative 
	 * the second derivatives with respect to the state of the 
	 * time derivative are implemented here.
	 *
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 */

        virtual void
	  CellTimeEquationExplicit_UU(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
				      dealii::Vector<double> &/*local_cell_vector*/, 
				      double /*scale*/);

        /******************************************************/
	/**
	 * This term implements the derivative of CellEquation 
	 * with respect to the u argument. I.e., if 
	 * CellEquation implements the term
	 * int_T a_T(u;\phi) then this method
	 * implements \int_T a_T'(u;\phi,z) 
	 * where \phi denotes the direction to which the derivative 
	 * is applied
	 * 
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 * @param scale_ico          A special scaling parameter to be used 
	 *                           in certain parts of the equation
	 *                           if they need to be treated differently in 
	 *                           time-stepping schemes, see the PDF-documentation
	 *                           for more details.
	 */
        virtual void
	  CellEquation_U(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
			 dealii::Vector<double> &/*local_cell_vector*/, 
			 double /*scale*/,
			 double /*scale_ico*/);

        /******************************************************/
	/**
	 * Similar to the StongCellResidual, this function implements the
	 * strong element residual for the adjoint equation.
	 * 
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param cdc_wight          The CellDataContainer for the weight-function,
	 *                           e.g., the testfunction by which the
	 *                           residual needs to be multiplied
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 */

        virtual void
	  StrongCellResidual_U(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
			       const CDC<DH, VECTOR, dealdim>& /*cdc_weight*/, 
			       double& /*ret*/, double /*scale*/);

        /******************************************************/
	/**
	 * This term implements the derivative of CellEquation 
	 * with respect to the u argument. I.e., if 
	 * CellEquation implements the term
	 * int_T a_T(u;\phi) then this method
	 * implements \int_T a_T'(u;du,\phi) .
	 * In contrast to CellEquation_U the arguments are exchanged.
	 * 
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 * @param scale_ico          A special scaling parameter to be used 
	 *                           in certain parts of the equation
	 *                           if they need to be treated differently in 
	 *                           time-stepping schemes, see the PDF-documentation
	 *                           for more details.
	 */

        virtual void
	  CellEquation_UT(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
			  dealii::Vector<double> &/*local_cell_vector*/, 
			  double /*scale*/,
			  double /*scale_ico*/);

        /******************************************************/
	/**
	 * This term implements the derivative of CellEquation 
	 * with respect to the u argument. I.e., if 
	 * CellEquation implements the term
	 * int_T a_T(u;\phi) then this method
	 * implements \int_T a_T'(u;phi,dz) .
	 * 
	 * This implements the same form as CellEquation_U, but
	 * with exchanged functions, i.e., dz instead of z.
	 * 
	 * @param cdc                The CellDataContainer object which provides 
	 *                           access to all information on the element, 
	 *                           e.g., test-functions, mesh size,...
	 * @param local_cell_vector  The vector containing the integrals
	 *                           ordered according to the local number 
	 *                           of the testfunction.
	 * @param scale              A scaling parameter to be used in all
	 *                           equations.
	 * @param scale_ico          A special scaling parameter to be used 
	 *                           in certain parts of the equation
	 *                           if they need to be treated differently in 
	 *                           time-stepping schemes, see the PDF-documentation
	 *                           for more details.
	 */

        virtual void
        CellEquation_UTT(const CDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        CellEquation_Q(const CDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        CellEquation_QT(const CDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        CellEquation_QTT(const CDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        CellEquation_UU(const CDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        CellEquation_QU(const CDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        CellEquation_UQ(const CDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        CellEquation_QQ(const CDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        /**
         * Documentation in optproblemcontainer.h.
         */
        virtual void
        CellRightHandSide(const CDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale);

        /******************************************************/

        /**
         * Documentation in optproblemcontainer.h.
         */
        virtual void
        CellMatrix(const CDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/,
            double /*scale*/, double /*scale_ico*/);

        /******************************************************/

        /**
         * Documentation in optproblemcontainer.h.
         */
        virtual void
        CellTimeMatrix(const CDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/);

        /******************************************************/
        virtual void
        CellTimeMatrix_T(const CDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/);

        /******************************************************/

        /**
         * Documentation in optproblemcontainer.h.
         */
        virtual void
        CellTimeMatrixExplicit(const CDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/);

        /******************************************************/
        virtual void
        CellTimeMatrixExplicit_T(const CDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/);

        /******************************************************/

        virtual void
        CellMatrix_T(const CDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/, double, double);

        /******************************************************/

        virtual void
        ControlCellEquation(const CDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale);

        /******************************************************/

        virtual void
        ControlCellMatrix(const CDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/);
        /******************************************************/

        virtual void
        StrongCellResidual_Control(const CDC<DH, VECTOR, dealdim>&,
            const CDC<DH, VECTOR, dealdim>& cdc_weight, double&, double scale);
        /******************************************************/

        virtual void
        StrongFaceResidual_Control(const FDC<DH, VECTOR, dealdim>&,
            const FDC<DH, VECTOR, dealdim>& fdc_weight, double&, double scale);
        /******************************************************/

        virtual void
        StrongBoundaryResidual_Control(const FDC<DH, VECTOR, dealdim>&,
            const FDC<DH, VECTOR, dealdim>& fdc_weight, double&, double scale);

        /******************************************************/
        // Functions for Face Integrals
        /**
         * Documentation in optproblemcontainer.h.
         */
        virtual void
        FaceEquation(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);
        /******************************************************/

        /**
         * Documentation in optproblemcontainer.h.
         */

        virtual void
        StrongFaceResidual(const FDC<DH, VECTOR, dealdim>&,
            const FDC<DH, VECTOR, dealdim>& fdc_weight, double&, double scale);

        /******************************************************/

        virtual void
        FaceEquation_U(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        StrongFaceResidual_U(const FDC<DH, VECTOR, dealdim>&,
            const FDC<DH, VECTOR, dealdim>& cdc_weight, double&, double scale);

        /******************************************************/

        virtual void
        FaceEquation_UT(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        FaceEquation_UTT(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        FaceEquation_Q(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        FaceEquation_QT(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        FaceEquation_QTT(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        FaceEquation_UU(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        FaceEquation_QU(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        FaceEquation_UQ(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        FaceEquation_QQ(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        FaceRightHandSide(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale);

        /******************************************************/

        /**
         * Documentation in optproblemcontainer.h.
         */
        virtual void
        FaceMatrix(const FDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        FaceMatrix_T(const FDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/, double scale,
            double scale_ico);

        /******************************************************/
        //Functions for Interface Integrals
        virtual void
        InterfaceMatrix(const FDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/, double scale,
            double scale_ico);

        /******************************************************/
        //Functions for Interface Integrals
        virtual void
        InterfaceMatrix_T(const FDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/, double scale,
            double scale_ico);

        /******************************************************/
        /**
         * Documentation in optproblemcontainer.h
         */

        virtual void
        InterfaceEquation(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/
        /**
         * Documentation in optproblemcontainer.h
         */

        virtual void
        InterfaceEquation_U(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/
        // Functions for Boundary Integrals
        /**
         * Documentation in optproblemcontainer.h.
         */
        virtual void
        BoundaryEquation(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        /**
         * Documentation in optproblemcontainer.h.
         */

        virtual void
        StrongBoundaryResidual(const FDC<DH, VECTOR, dealdim>&,
            const FDC<DH, VECTOR, dealdim>& fdc_weight, double&, double scale);

        /******************************************************/

        virtual void
        BoundaryEquation_U(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        StrongBoundaryResidual_U(const FDC<DH, VECTOR, dealdim>&,
            const FDC<DH, VECTOR, dealdim>& fdc_weight, double&, double scale);

        /******************************************************/

        virtual void
        BoundaryEquation_UT(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        BoundaryEquation_UTT(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        BoundaryEquation_Q(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        BoundaryEquation_QT(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        BoundaryEquation_QTT(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        BoundaryEquation_UU(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        BoundaryEquation_QU(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        BoundaryEquation_UQ(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        BoundaryEquation_QQ(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        BoundaryRightHandSide(const FDC<DH, VECTOR, dealdim>&,
            dealii::Vector<double> &/*local_cell_vector*/, double scale);

        /******************************************************/

        /**
         * Documentation in optproblemcontainer.h.
         */
        virtual void
        BoundaryMatrix(const FDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/, double scale,
            double scale_ico);

        /******************************************************/

        virtual void
        BoundaryMatrix_T(const FDC<DH, VECTOR, dealdim>&,
            dealii::FullMatrix<double> &/*local_entry_matrix*/, double scale,
            double scale_ico);

        /******************************************************/
        /******************************************************/
        /****For the initial values ***************/
        /* Default is componentwise L2 projection */
        virtual void
        Init_CellEquation(const CDC<DH, VECTOR, dealdim>& cdc,
            dealii::Vector<double> &local_cell_vector, double scale,
            double /*scale_ico*/)
        {
          const DOpEWrapper::FEValues<dealdim> & state_fe_values =
              cdc.GetFEValuesState();
          unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
          unsigned int n_q_points = cdc.GetNQPoints();
          std::vector<dealii::Vector<double> > uvalues;
          uvalues.resize(n_q_points,
              dealii::Vector<double>(this->GetStateNComponents()));
          cdc.GetValuesState("last_newton_solution", uvalues);

          dealii::Vector<double> f_values(
              dealii::Vector<double>(this->GetStateNComponents()));

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            for (unsigned int i = 0; i < n_dofs_per_cell; i++)
            {
              for (unsigned int comp = 0; comp < this->GetStateNComponents();
                  comp++)
              {
                local_cell_vector(i) += scale
                    * (state_fe_values.shape_value_component(i, q_point, comp)
                        * uvalues[q_point](comp))
                    * state_fe_values.JxW(q_point);
              }
            }
          } //endfor q_point
        }

        virtual void
        Init_CellRhs_Q(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
            dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
        {

        }
        virtual void
        Init_CellRhs_QT(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
            dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
        {

        }
        virtual void
        Init_CellRhs_QTT(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
            dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
        {

        }
        virtual void
        Init_CellRhs_QQ(const CDC<DH, VECTOR, dealdim>& /*cdc*/,
            dealii::Vector<double> &/*local_cell_vector*/, double /*scale*/)
        {

        }

        virtual void
        Init_CellRhs(const dealii::Function<dealdim>* init_values,
            const CDC<DH, VECTOR, dealdim>& cdc,
            dealii::Vector<double> &local_cell_vector, double scale)
        {
          const DOpEWrapper::FEValues<dealdim> & state_fe_values =
              cdc.GetFEValuesState();
          unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
          unsigned int n_q_points = cdc.GetNQPoints();

          dealii::Vector<double> f_values(
              dealii::Vector<double>(this->GetStateNComponents()));

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            init_values->vector_value(state_fe_values.quadrature_point(q_point),
                f_values);

            for (unsigned int i = 0; i < n_dofs_per_cell; i++)
            {
              for (unsigned int comp = 0; comp < this->GetStateNComponents();
                  comp++)
              {
                local_cell_vector(i) += scale
                    * (f_values(comp)
                        * state_fe_values.shape_value_component(i, q_point,
                            comp)) * state_fe_values.JxW(q_point);
              }
            }
          }
        }

        virtual void
        Init_CellMatrix(const CDC<DH, VECTOR, dealdim>& cdc,
            dealii::FullMatrix<double> &local_entry_matrix, double scale,
            double /*scale_ico*/)
        {
          const DOpEWrapper::FEValues<dealdim> & state_fe_values =
              cdc.GetFEValuesState();
          unsigned int n_dofs_per_cell = cdc.GetNDoFsPerCell();
          unsigned int n_q_points = cdc.GetNQPoints();

          for (unsigned int q_point = 0; q_point < n_q_points; q_point++)
          {
            for (unsigned int i = 0; i < n_dofs_per_cell; i++)
            {
              for (unsigned int j = 0; j < n_dofs_per_cell; j++)
              {
                for (unsigned int comp = 0; comp < this->GetStateNComponents();
                    comp++)
                {
                  local_entry_matrix(i, j) += scale
                      * state_fe_values.shape_value_component(i, q_point, comp)
                      * state_fe_values.shape_value_component(j, q_point, comp)
                      * state_fe_values.JxW(q_point);
                }
              }
            }
          }
        }
        /*************************************************************/
        virtual dealii::UpdateFlags
        GetUpdateFlags() const;
        virtual dealii::UpdateFlags
        GetFaceUpdateFlags() const;
        virtual bool
        HasFaces() const;
        virtual bool
        HasInterfaces() const;

        /******************************************************/

        void
        SetProblemType(std::string type);

        /******************************************************/

        virtual unsigned int
        GetControlNBlocks() const;
        virtual unsigned int
        GetStateNBlocks() const;
        virtual std::vector<unsigned int>&
        GetControlBlockComponent();
        virtual const std::vector<unsigned int>&
        GetControlBlockComponent() const;
        virtual std::vector<unsigned int>&
        GetStateBlockComponent();
        virtual const std::vector<unsigned int>&
        GetStateBlockComponent() const;

        /******************************************************/

        virtual void
        SetTime(double t __attribute__((unused))) const
        {
        }

        /******************************************************/

        unsigned int
        GetStateNComponents() const;

        /******************************************************/
        /**
         * This function is set by the error estimators in order
         * to allow the calculation of squared norms of the residual
         * as needed for Residual Error estimators as well as
         * the residual itself as needed by the DWR estimators.
         */
        boost::function1<void, double&> ResidualModifier;
        boost::function1<void, dealii::Vector<double>&> VectorResidualModifier;

      protected:
        std::string _problem_type;

      private:
    };
}

#endif
