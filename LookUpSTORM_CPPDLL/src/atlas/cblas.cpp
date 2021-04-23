
extern "C" {

#include "cblas.h"
#include "atlas_reflevel1.h"
#include "atlas_reflevel2.h"
#include "atlas_reflevel3.h"

}

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

int cblas_idamax(const int N, const double* X, const int incX)
{
    if (N > 0 && incX > 0)
        return(ATL_idrefamax(N, X, incX));
    return(0);
}

void ALT_dcopy(const int N, const double* X, const int incX, double* Y, const int incY)
{
    int i;
    for (i = N; i; i--, X += incX, Y += incY) *Y = *X;
}

void cblas_dscal(const int N, const double alpha, double* X, const int incX)
{
    if (N > 0 && incX > 0)
        ATL_drefscal(N, alpha, X, incX);
}

void cblas_dswap(const int N, double* X, const int incX,
    double* Y, const int incY)
{
    if (N > 0)
    {
        if (incX < 0)
        {
            if (incY < 0) ATL_drefswap(N, X, -incX, Y, -incY);
            else ATL_drefswap(N, X + (1 - N) * incX, incX, Y, incY);
        }
        else if (incY < 0) ATL_drefswap(N, X + (N - 1) * incX, -incX, Y, -incY);
        else ATL_drefswap(N, X, incX, Y, incY);
    }
}

void ATL_dsyreflect(const enum CBLAS_UPLO Uplo, const int N, double* C, const int ldc)
{
    int j;
    const size_t incC = ldc + 1, ldc2 = ldc;
    double* pC;
    if (Uplo == CblasLower) {
        for (j = 0; j < N - 1; j++, C += incC)
            ALT_dcopy(N - j - 1, C + 1, 1, C + ldc2, ldc);
    }
    else {
        pC = C + ((size_t)(N - 1));
        C += ldc2 * (N - 1);
        for (j = 0; j < N - 1; j++, C -= ldc2, pC -= 1)
            ALT_dcopy(N - j - 1, C, 1, pC, ldc);
    }
}

void cblas_dgemm(const enum CBLAS_ORDER Order,
    const enum CBLAS_TRANSPOSE TA, const enum CBLAS_TRANSPOSE TB,
    const int M, const int N, const int K,
    const double  alpha, const double* A, const int lda,
    const double* B, const int ldb, const double  beta,
    double* C, const int ldc)
{
    if (A == B && M == N && TA != TB && lda == ldb && beta == 0.0)
    {
        ATL_drefsyrk(CblasUpper, (Order == CblasColMajor) ? TA : TB, N, K,
            alpha, A, lda, beta, C, ldc);
        ATL_dsyreflect(CblasUpper, N, C, ldc);
        return;
    }
    if (Order == CblasColMajor)
        ATL_drefgemm(TA, TB, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
    else
        ATL_drefgemm(TB, TA, N, M, K, alpha, B, ldb, A, lda, beta, C, ldc);
}

void cblas_dgemv(const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE TA,
    const int M, const int N, const double alpha, const double* A,
    const int lda, const double* X, const int incX,
    const double beta, double* Y, const int incY)
{
    if (TA == CblasNoTrans)
    {
        if (incX < 0) X += (1 - N) * incX;
        if (incY < 0) Y += (1 - M) * incY;
    }
    else
    {
        if (incX < 0) X += (1 - M) * incX;
        if (incY < 0) Y += (1 - N) * incY;
    }
    if (Order == CblasColMajor)
        ATL_drefgemv(TA, M, N, alpha, A, lda, X, incX, beta, Y, incY);
    else
    {
        if (TA == CblasNoTrans)
            ATL_drefgemv(CblasTrans, N, M, alpha, A, lda, X, incX, beta, Y, incY);
        else
            ATL_drefgemv(CblasNoTrans, N, M, alpha, A, lda, X, incX, beta, Y, incY);
    }
}

void cblas_dtrsm(const enum CBLAS_ORDER Order, const enum CBLAS_SIDE Side,
    const enum CBLAS_UPLO Uplo, const enum CBLAS_TRANSPOSE TA,
    const enum CBLAS_DIAG Diag, const int M, const int N,
    const double  alpha, const double* A, const int lda,
    double* B, const int ldb)
{
    enum CBLAS_SIDE side;
    enum CBLAS_UPLO uplo;

    if (Order == CblasColMajor)
        ATL_dreftrsm(Side, Uplo, TA, Diag, M, N, alpha, A, lda, B, ldb);
    else
    {
        if (Side == CblasLeft) side = CblasRight;
        else side = CblasLeft;
        if (Uplo == CblasUpper) uplo = CblasLower;
        else uplo = CblasUpper;
        ATL_dreftrsm(side, uplo, TA, Diag, N, M, alpha, A, lda, B, ldb);
    }
}
