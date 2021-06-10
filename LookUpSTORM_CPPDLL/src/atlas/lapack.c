#include <float.h>
#include <stddef.h>
#include <stdlib.h>

#include "cblas.h"
#include "atlas_reflevel2.h"
#include "atlas_lapack.h"
#include "atlas_enum.h"

#define TREAL
#define SHIFT
#define TYPE double
#define ATL_INT int
#define ATL_CINT int
#define ATL_rone 1.0
#define ATL_rnone -1.0
#define ATL_rzero 0.0
#define PATL ALT_d

#include "atlas_lapack.h"

#define Mabs(x) ( (x) >= 0 ? (x) : -(x) )
#define Mmax(x, y) ( (x) > (y) ? (x) : (y) )
#define Mmin(x, y) ( (x) > (y) ? (y) : (x) )

#define ATL_L1elts 256 * 1024ul // 256 k L1 cache

#define ATL_laSAFMIN DBL_MIN

#define MB 56
#define NB 56
#define KB 56

#define ATL_mmMU  8
#define ATL_mmNU  1
#define ATL_mmKU  56

#define cblas_iamax cblas_idamax
#define cblas_gemv cblas_dgemv
#define cblas_gemm cblas_dgemm
#define cblas_trsv cblas_dtrsv
#define cblas_trsm cblas_dtrsm
#define cblas_scal cblas_dscal
#define cblas_swap cblas_dswap
#define my_ger ATL_drefger

#define ATL_MulByNB(n_) ((n_)<<5)
#define ATL_DivByNB(n_) ((n_)>>5)

#include "ATL_laswp.c"
#undef lda

#include "ATL_trtriCL.c"
#include "ATL_trtriCU.c"
#include "ATL_trtriRL.c"
#include "ATL_trtriRU.c"
#include "ATL_trtri.c"

#undef one
#undef none
#undef mone

#include "ATL_getriC.c"
#include "ATL_getriR.c"
#include "ATL_getri.c"

#include "ATL_getf2.c"

#include "ATL_getrfC.c"
#include "ATL_getrfR.c"

#include "ATL_getrs.c"

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
    const enum CBLAS_TRANSPOSE Trans = TransA == 'T' ? CblasTrans : TransA == 'C' ? CblasConjTrans : CblasNoTrans;
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

#define ATL_dMulBySize(N_) ((((N_)) << 3))
#define ATL_Cachelen 32

#define ATL_MulByCachelen(N_) ( (N_) << 5 )
#define ATL_DivByCachelen(N_) ( (N_) >> 5 )

#define ATL_AlignPtr(vp) \
   (void*) (ATL_Cachelen + ATL_MulByCachelen(ATL_DivByCachelen((size_t) (vp))))

inline
int LAPACKE_dgetri(const enum CBLAS_ORDER Order, const int N, double* A,
    const int lda, const int* ipiv)
{
    int ierr = 0, lwrk;
    void* vp;

    lwrk = NB;
    if (lwrk <= N) lwrk *= N;
    else lwrk = N * N;
    vp = malloc(ATL_Cachelen + ATL_dMulBySize(lwrk));
    if (vp) {
        ierr = ATL_getri(Order, N, A, lda, ipiv, (double*)ATL_AlignPtr(vp), &lwrk);
        free(vp);
    }
    else {
        return(-7);
    }
    return(ierr);
}

inline
int LAPACKE_dgesv(const enum CBLAS_ORDER Order, const int n, const int nrhs,
    double* a, const int lda, int* ipiv, double* b, const int ldb)
{
    int info = LAPACKE_dgetrf(Order, n, n, a, lda, ipiv);
    if (info == 0)
        ATL_getrs(AtlasColMajor, AtlasNoTrans, n, nrhs, a, lda, ipiv, b, ldb);
    for (int i = 0; i != n; i++) 
        ipiv[i]++;
    return info;
}

#undef MB
#undef NB
#undef KB
