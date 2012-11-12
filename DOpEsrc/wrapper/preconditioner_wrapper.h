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

#ifndef _DOPE_PRECONDITIONER_H_
#define _DOPE_PRECONDITIONER_H_

#include <lac/precondition.h>
#include <lac/sparse_ilu.h>

namespace DOpEWrapper
{
  template <typename MATRIX>
    class PreconditionSSOR_Wrapper : public dealii::PreconditionSSOR<MATRIX>
  {
  public:
    void initialize(const MATRIX& A)
    {
      dealii::PreconditionSSOR<MATRIX>::initialize(A,1);
    }
  };

  template <typename MATRIX>
    class PreconditionIdentity_Wrapper : public dealii::PreconditionIdentity
  {
  public:
    void initialize(const MATRIX& /*A*/)
    {
    }
  };

  template <typename number>
    class PreconditionSparseILU_Wrapper : public dealii::SparseILU<number>
  {
  public:
    void initialize(const SparseMatrix<number>& A)
    {
      dealii::SparseILU<number>::initialize(A);
    }
  };
}

#endif
