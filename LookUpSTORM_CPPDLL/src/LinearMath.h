/****************************************************************************
 *
 * Copyright (C) 2020-2021 Fabian Hauser
 *
 * Author: Fabian Hauser <fabian.hauser@fh-linz.at>
 * University of Applied Sciences Upper Austria - Linz - Austra
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#ifndef LINEAR_MATH_H
#define LINEAR_MATH_H

#include "Matrix.h"
#include "Vector.h"
#include "Common.h"

namespace LookUpSTORM
{

static constexpr int LIN_SUCCESS = 0;
static constexpr int LIN_ERR = -2;
static constexpr int LIN_ERR_SIZE = -2;
static constexpr int LIN_ERR_SHAPE = -3;
static constexpr int LIN_ERR_NOT_SQUARE = -4;
static constexpr int LIN_ERR_NULL_ARRAY = -5;

namespace BLAS {
    enum TRANSPOSE_t {
        CblasNoTrans = 111,
        CblasTrans = 112,
        CblasConjTrans = 113
    };
    enum UPLO_t {
        CblasUpper = 121,
        CblasLower = 122
    };
    enum DIAG_t { 
        CblasNonUnit = 131, 
        CblasUnit = 132 
    };
    
    // matrix, matrix multiplication of the same matrix that results in
    // a symmetrical triangular matrix
    int dsyrk(UPLO_t Uplo, TRANSPOSE_t Trans, double alpha,
              const Matrix& A, double beta, Matrix& C);

    // solves the triangular system of linear equations
    int dtrsv(UPLO_t Uplo, TRANSPOSE_t Trans,
              DIAG_t Diag, const Matrix& A, Vector& X);

    int dgemv(TRANSPOSE_t TransA, double alpha,
              const Matrix& A, const Vector& X,
              double beta, Vector& Y);

    int dgemm(TRANSPOSE_t TransA, TRANSPOSE_t TransB,
              double alpha, const Matrix& A, const Matrix& B, 
              double beta, Matrix& C);
};

namespace LAPACKE {
    enum UPLO_t : char {
        U = 'U',
        L = 'L'
    };
    enum TRANSPOSE_t : char {
        NoTrans = 'N',
        Trans = 'T',
        ConjTrans = 'C'
    };

    int dgetrf(Matrix& A, int* ipiv);
    int dgetrs(TRANSPOSE_t Trans, const Matrix& A, int* ipiv, Vector& b);

#ifndef NO_LAPACKE_LUT
    int dsysv(UPLO_t uplo, Matrix& A, int* ipiv, Vector& b);
#endif // NO_LAPACKE

};

} // namespace LookUpSTORM

#endif // !LINEAR_MATH_H

