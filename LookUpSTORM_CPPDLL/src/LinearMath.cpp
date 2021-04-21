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
#include <float.h>
#include "atlas/cblas.h"
#include "atlas/atlas_reflevel2.h"

#define TREAL
#define SHIFT
#define TYPE double
#define ATL_INT int
#define ATL_CINT int
#define ATL_rone 1.0
#define ATL_rnone -1.0
#define ATL_rzero 0.0

#define Mabs(x) ( (x) >= 0 ? (x) : -(x) )
#define Mmax(x, y) ( (x) > (y) ? (x) : (y) )
#define Mmin(x, y) ( (x) > (y) ? (y) : (x) )

#define ATL_L1elts 256 * 1024ul // 256 k L1 cache

#define ATL_laSAFMIN DBL_MIN

#define cblas_iamax cblas_idamax
#define cblas_gemv cblas_dgemv
#define cblas_gemm cblas_dgemm
#define cblas_trsv cblas_dtrsv
#define cblas_trsm cblas_dtrsm
#define cblas_scal cblas_dscal
#define cblas_swap cblas_dswap
#define my_ger ATL_drefger

#include "atlas/ATL_laswp.c"
#include "atlas/ATL_getf2.c"
#include "atlas/ATL_getrfC.c"
#include "atlas/ATL_getrfR.c"
#include "atlas/ATL_getrs.c"

inline
int LAPACKE_dgetrf(const enum CBLAS_ORDER Order, const int M, const int N,
    double* A, const int lda, int* ipiv)
{
    if (Order == CblasColMajor) return(ATL_getrfC(M, N, A, lda, ipiv));
    else return(ATL_getrfR(M, N, A, lda, ipiv));
}

inline
int LAPACKE_dgetrs(const enum CBLAS_ORDER Order, char TransA,
    const int N, const int NRHS, const double* A, const int lda,
    const int* ipiv, double* B, const int ldb)
{
    CBLAS_TRANSPOSE Trans = TransA == 'T' ? CblasTrans : TransA == 'C' ? CblasConjTrans : CblasNoTrans;
    if (Order != CblasRowMajor && Order != CblasColMajor)
        return -1;
    if (Trans != CblasNoTrans && Trans != CblasTrans && Trans != CblasConjTrans)
        return -2;
    if (lda < N || lda < 1)
        return -3;
    if (ldb < N || ldb < 1)
        return -4;
    ATL_getrs(Order, Trans, N, NRHS, A, lda, ipiv, B, ldb);
    return 0;
}

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
