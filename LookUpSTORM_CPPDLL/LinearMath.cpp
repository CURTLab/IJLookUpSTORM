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

#ifdef USE_MKL
#include <mkl_cblas.h>
#include <mkl_lapacke.h>
#define INT MKL_INT
#else
// atlas implementation
#define INT int
extern "C" {
#include "atlas/atlas_reflevel2.h"
#include "atlas/atlas_reflevel3.h"
}


inline
void cblas_dsyrk(const enum CBLAS_ORDER Order, const enum CBLAS_UPLO Uplo,
    const enum CBLAS_TRANSPOSE Trans, const int N, const int K,
    const double alpha, const double* A, const int lda,
    const double beta, double* C, const int ldc)
{
    if (Order == CblasColMajor)
        ATL_drefsyrk(Uplo, Trans, N, K, alpha, A, lda, beta, C, ldc);
    else
    {
        CBLAS_UPLO uplo;
        CBLAS_TRANSPOSE trans;
        if (Uplo == CblasUpper) uplo = CblasLower;
        else uplo = CblasUpper;
        if (Trans == CblasNoTrans) trans = CblasTrans;
        else trans = CblasNoTrans;
        ATL_drefsyrk(uplo, trans, N, K, alpha, A, lda, beta, C, ldc);
    }
}

inline
void cblas_dtrsv(const enum CBLAS_ORDER Order, const enum CBLAS_UPLO Uplo,
    const enum CBLAS_TRANSPOSE TA, const enum CBLAS_DIAG Diag,
    const int N, const double* A, const int lda, double* x,
    const int incX)
{
    if (incX < 0) x += ((1 - N) * incX);
    if (Order == CblasColMajor)
        ATL_dreftrsv(Uplo, TA, Diag, N, A, lda, x, incX);
    else
    {
        enum CBLAS_UPLO uplo;
        enum CBLAS_TRANSPOSE ta;
        uplo = ((Uplo == CblasUpper) ? CblasLower : CblasUpper);
        if (TA == CblasNoTrans) ta = CblasTrans;
        else ta = CblasNoTrans;
        ATL_dreftrsv(uplo, ta, Diag, N, A, lda, x, incX);
    }
}
#endif // USE_MKL

#include <stdexcept>
#include <iostream>

int BLAS::dsyrk(UPLO_t Uplo, TRANSPOSE_t Trans, double alpha, const Matrix& A, double beta, Matrix& C)
{
    if (A.isNull() || C.isNull())
        return LIN_ERR_NULL_ARRAY;

    const size_t M = C.size1(), N = C.size2();
    const size_t J = (Trans == CblasNoTrans) ? A.size1() : A.size2();
    const size_t K = (Trans == CblasNoTrans) ? A.size2() : A.size1();
    if (M != N)
    {
        std::cerr << "BLAS: matrix C must be square" << std::endl;
        return LIN_ERR_NOT_SQUARE;
    }
    else if (N != J)
    {
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

    if (M != N)
    {
        std::cerr << "BLAS: matrix C must be square" << std::endl;
        return LIN_ERR_NOT_SQUARE;
    }
    else if (N != X.size())
    {
        std::cerr << "BLAS: invalid length" << std::endl;
        return LIN_ERR_SIZE;
    }

    cblas_dtrsv(CblasRowMajor, (CBLAS_UPLO)Uplo, (CBLAS_TRANSPOSE)Trans, (CBLAS_DIAG)Diag, INT(N), A.constData(),
        INT(A.tda()), X.data(), INT(X.stride()));
    return LIN_SUCCESS;
}

#ifndef NO_LAPACKE
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
