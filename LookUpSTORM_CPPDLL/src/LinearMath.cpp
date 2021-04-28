/****************************************************************************
 *
 * MIT License
 *
 * Copyright (C) 2021 Fabian Hauser
 *
 * Author: Fabian Hauser <fabian.hauser@fh-linz.at>
 * University of Applied Sciences Upper Austria - Linz - Austria
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ****************************************************************************/

#include "LinearMath.h"

#include <cmath>

#ifdef USE_MKL_LUT
#include <mkl_cblas.h>
#include <mkl_lapacke.h>
#define INT MKL_INT
#else
// atlas implementation
#define INT int
extern "C" {
#include "atlas/cblas.h"
#include "atlas/lapack.c"
#define LAPACK_ROW_MAJOR CblasRowMajor
#define LAPACK_COL_MAJOR CblasColMajor
} // extern "C"
#endif // USE_MKL_LUT

#include <stdexcept>
#include <iostream>

using namespace LookUpSTORM;

int BLAS::dsyrk(UPLO_t Uplo, TRANSPOSE_t Trans, double alpha, const Matrix& A, double beta, Matrix& C)
{
    if (A.isNull() || C.isNull())
        return LIN_ERR_NULL_ARRAY;

    const size_t M = C.size1(), N = C.size2();
    const size_t J = (Trans == CblasNoTrans) ? A.size1() : A.size2();
    const size_t K = (Trans == CblasNoTrans) ? A.size2() : A.size1();
    if (M != N) {
        std::cerr << "BLAS: matrix C must be square" << std::endl;
        return LIN_ERR_NOT_SQUARE;
    } else if (N != J) {
        std::cerr << "BLAS: invalid length" << std::endl;
        return LIN_ERR_SIZE;
    }

    cblas_dsyrk(CblasRowMajor, (CBLAS_UPLO)Uplo, (CBLAS_TRANSPOSE)Trans, INT(N), INT(K), alpha, A.constData(),
        INT(A.tda()), beta, C.data(), INT(C.tda()));
    return LIN_SUCCESS;
}

int BLAS::dtrsv(UPLO_t Uplo, TRANSPOSE_t Trans, DIAG_t Diag, const Matrix& A, Vector& X)
{
    const size_t M = A.size1(), N = A.size2();

    if (M != N) {
        std::cerr << "BLAS: matrix C must be square" << std::endl;
        return LIN_ERR_NOT_SQUARE;
    } else if (N != X.size()) {
        std::cerr << "BLAS: invalid length" << std::endl;
        return LIN_ERR_SIZE;
    }

    cblas_dtrsv(CblasRowMajor, (CBLAS_UPLO)Uplo, (CBLAS_TRANSPOSE)Trans, (CBLAS_DIAG)Diag, INT(N), A.constData(),
        INT(A.tda()), X.data(), INT(X.stride()));
    return LIN_SUCCESS;
}

int LookUpSTORM::BLAS::dgemv(TRANSPOSE_t TransA, double alpha, const Matrix& A, const Vector& X, double beta, Vector& Y)
{
    const size_t M = A.size1(), N = A.size2();

    if (((TransA == CblasNoTrans) && (N == X.size()) && (M == Y.size()))
        || ((TransA == CblasTrans) && (M == X.size()) && (N == Y.size()))) 
    {
        cblas_dgemv(CblasRowMajor, (CBLAS_TRANSPOSE)TransA, INT(M), INT(N), 
            alpha, A.constData(), INT(A.tda()), X.constData(), INT(X.stride()), beta, 
            Y.data(), INT(Y.stride()));
        return LIN_SUCCESS;
    }
    std::cerr << "BLAS: invalid size" << std::endl;
    return LIN_ERR_SIZE;
}

int LookUpSTORM::BLAS::dgemm(TRANSPOSE_t TransA, TRANSPOSE_t TransB, double alpha, const Matrix& A, const Matrix& B, double beta, Matrix& C)
{
    const size_t M = C.size1(), N = C.size2();
    const size_t MA = (TransA == CblasNoTrans) ? A.size1() : A.size2();
    const size_t NA = (TransA == CblasNoTrans) ? A.size2() : A.size1();
    const size_t MB = (TransB == CblasNoTrans) ? B.size1() : B.size2();
    const size_t NB = (TransB == CblasNoTrans) ? B.size2() : B.size1();

    if ((M == MA) && (N == NB) && (NA == MB)) {
        cblas_dgemm(CblasRowMajor, (CBLAS_TRANSPOSE)TransA, 
            (CBLAS_TRANSPOSE)TransB, INT(M), INT(N), INT(NA),
            alpha, A.constData(), INT(A.tda()), B.constData(), 
            INT(B.tda()), beta, C.data(), INT(C.tda()));
        return LIN_SUCCESS;
    }
    std::cerr << "BLAS: invalid size" << std::endl;
    return LIN_ERR_SIZE;
}

int LAPACKE::dgetrf(Matrix& A, int* ipiv)
{
    if (A.isNull())
        return LIN_ERR_NULL_ARRAY;
    return LAPACKE_dgetrf(LAPACK_ROW_MAJOR, INT(A.size1()), INT(A.size2()), A.data(), INT(A.tda()), ipiv);
}

int LAPACKE::dgetrs(TRANSPOSE_t Trans, const Matrix& A, int* ipiv, Vector& b)
{
    if (A.isNull() || b.isNull())
        return LIN_ERR_NULL_ARRAY;
    return LAPACKE_dgetrs(LAPACK_ROW_MAJOR, Trans, INT(A.size1()), INT(1), 
                          A.constData(), INT(A.size2()), 
                          ipiv, b.data(), INT(b.size()));
}

int LAPACKE::dgetri(Matrix& A, const int* ipiv)
{
    if (A.isNull())
        return LIN_ERR_NULL_ARRAY;
    return LAPACKE_dgetri(LAPACK_ROW_MAJOR, INT(A.size1()), A.data(), INT(A.size2()), ipiv);
}

#ifndef NO_LAPACKE_LUT
int LAPACKE::dsysv(UPLO_t uplo, Matrix& A, int* ipiv, Vector& b)
{
    if (A.isNull() || b.isNull())
        return LIN_ERR_NULL_ARRAY;

    const size_t N = b.size();
    if (N != A.size1())
        return LIN_ERR_SIZE;
    return LAPACKE_dsysv(LAPACK_ROW_MAJOR, uplo, INT(N), 1, A.data(), INT(A.tda()), ipiv, b.data(), b.stride());
}
#endif
