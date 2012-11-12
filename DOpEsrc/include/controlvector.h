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

#ifndef _CONTROL_VECTOR_H_
#define _CONTROL_VECTOR_H_

#include "spacetimehandler_base.h"

#include <lac/vector.h>
#include <lac/block_vector_base.h>
#include <lac/block_vector.h>

#include <vector>
#include <iostream>
#include <sstream>


namespace DOpE
{

template<typename VECTOR>
  class ControlVector
  {
  public:
    //TODO: Currently we only consider one fixed control 
    //      for all timesteps, if more is desired one needs to augment the 
    //      Spacetimehandler to have a time discretization for the control,
    //      Then one can update this vector similar to the statevector 
    //      with different meshes for Vectors.
    //      Note that this requires to keep track of the interpolation
    //      between state and control time points...
    ControlVector(const ControlVector& ref);
    ControlVector(const SpaceTimeHandlerBase<VECTOR>* STH, std::string behavior);
    ~ControlVector();

    /**
     * Sets the time in the vector. This Function or SetTimeDoFNumber
     * must be called before calling GetSpacialVector
     * in order to load the required data.
     *
     * @param t            A double containing the time we are interested in. If t doesn't match the time given by
     *                     time_point, then an interpolation between the corresponding time_points is
     *			               computed.
     * @param interval      An TimeIterator. The interval containing t.
     *
     */
    void SetTime(double t, const TimeIterator& interval) const;
    /**
     * Sets the time in the vector. This Function or SetTime
     * must be called before calling GetSpacialVector
     * in order to load the required data.
     *
     * @param time_point   An unsigned integer. This gives the number of the point in the  time mesh.
     *
     */
    void SetTimeDoFNumber(unsigned int time_point) const;
    /**
     * Returns a reference to the spacial vector associated to the last time given by SetTime*
     */
    VECTOR& GetSpacialVector();
    /**
     * Returns a const reference to the spacial vector associated to the last time given by SetTime*
     */
    const VECTOR& GetSpacialVector() const;
    /**
     * Returns a const reference to the spacial vector associated to the last time given by SetTime*
     * This makes a copy of the real vector  in order to change the vector type.
     * To assert data integrity this Only one Copy may  be obtained at any time.
     * Hence prior to calling this Function again UnLockCopy must be called.
     */
    const dealii::Vector<double>& GetSpacialVectorCopy() const;

    /**
     * Sets all the vector to a constant value.
     *
     * @param value    The constant value to be assigned to the vector.
     */
    void operator=(double value);
    /**
     * Sets this vector to the values of an other given vector.
     * If required this vector is resized. This invalidates all prior SetTime* calls.
     *
     * @param dq    The other vector.
     */
    void operator=(const ControlVector& dq);
    /**
     * Upon completion each entry of this Vector contains the following
     * Result this = this + dq;
     * It is required that both this and dq have the same structure!
     *
     * @param dq    The increment.
     */
    void operator+=(const ControlVector& dq);
    /**
     * Multiplies the Vector with a constant.
     *
     * @param a    A double to be multiplied with the vector.
     */
    void operator*=(double a);
    /**
     * Computes the Euclidean scalar product of this vector with the argument.
     * Both Vectors must have the same struckture.
     *
     * @param dq    The argument for the computation of the scalarproduct.
     * @return      A double containing the scalar product.
     */
    double operator*(const ControlVector& dq) const;
    /**
     * Sets this vector adds a multiple of an other vector to this vector.
     * this = this + s * dq
     * It expects both vectors  to be of the same structure.
     *
     * @param s    A double, by which the other vector is scaled.
     * @param dq   The other vector.
     */
    void add(double s, const ControlVector& dq);
    /**
     * Sets this vector to the values of an other given vector.
     * The vector is not resized!
     *
     * @param dq    The other vector.
     */
    void equ(double s, const ControlVector& dq);
    /**
     * Sets this vector to the componentwise maximum of its own 
     * entries and that of the other vector
     * The vector is not resized!
     *
     * @param dq    The other vector.
     */
    void max(const ControlVector& dq);
    /**
     * Sets this vector to the componentwise minimum of its own 
     * entries and that of the other vector
     * The vector is not resized!
     *
     * @param dq    The other vector.
     */
    void min(const ControlVector& dq);

    /**
     * Computes the component wise product of this vector with the argument.
     */
    void comp_mult(const ControlVector& dq);
    
    /**
     * Inverts the elements of the vetor component wise 
     */
    void comp_invert();
    
    /**
     * Initializes this vector according to the signs in it.
     *
     * @param smaller   value to be taken if sign is negative
     * @param larger    value to be taken if sign is positive
     * @param unclear   value to be taken if sign is unclear
     * @param TOL       if abs(value) < TOL we consider the sign to be unclear
     */
    void init_by_sign(double smaller, double larger, double unclear, double TOL = 1.e-10);
    
    /**
     * Prints Information on this vector into the given stream.
     *
     * @param out    The output stream.
     */
    void PrintInfos(std::stringstream& out);
    /**
     * This unlocks the function GetSpacialVectorCopy
     */
    void UnLockCopy() const { _lock = false; }

    /**
     * This returns the behavior of the ControlVector
     * Currently implemented are the following posibilities
     * @par  fullmem         Means there is a spacial vector for each time point. The whole vector
     *                       is stored in main memory.
     *
     * @return               A string indicating the behavior.
     */
    std::string GetBehavior() const { return _behavior; }

    /**
     * @return               A const pointer to the SpaceTimeHandler associated with this vector.
     */
    const SpaceTimeHandlerBase<VECTOR>* GetSpaceTimeHandler() const { return _STH; }

    /**
     * Call if the SpaceTimeHandler has changed to reinitialize vector sizes.
     *
     */
    void ReInit();
 
   /**
     * Computes the norm given by name of the vector.
     * Feasible values are "infty", and "l1"
     * The string restriction defines if only certain values are
     * to be considered. Currently "all" and "positive" are feasible
     * Meaning that either all or only the positive entries are 
     * considered.
     */
    double Norm(std::string name,std::string restriction = "all") const;

  private:
    /**
     * This function resizes the spacial vector at a prior given time point.
     * Hence SetTimeDoFNumber should be called before this function.
     */
    void ReSizeSpace(unsigned int ndofs, const std::vector<unsigned int>& dofs_per_block);

    std::vector<VECTOR* > _control;
    mutable VECTOR _local_control;
    mutable dealii::Vector<double> _copy_control;

    mutable int _accessor;
    mutable bool _lock;

    const SpaceTimeHandlerBase<VECTOR>* _STH;
    std::string _behavior;
    unsigned int _sfh_ticket;
  };


}
#endif
