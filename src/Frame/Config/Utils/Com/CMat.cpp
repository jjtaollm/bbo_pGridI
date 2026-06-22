/*
 * CMat.cpp
 *
 *  Created on: 2018年3月12日
 *      Author: juntao, at wuhan university
 */
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include "include/Frame/Frame.h"
#define MKL
#ifdef MKL
#include "include/Frame/Config/Utils/Com/mkl.h"
#include "include/Frame/Config/Utils/Com/mkl_cblas.h"
#endif
using namespace std;
using namespace iono;
double *CMat::mat(int n, int m)
{
    double *p = NULL;
    if (m < 0 || n < 0)
    {
        cout << "***ERROR(mat):cant be zero!" << endl;
        return NULL;
    }
    p = (double *)malloc(m * n * sizeof(double));
    if (!p)
    {
        cout << "***ERROR(mat):allocate error!" << endl;
        return NULL;
    }
    memset(p, 0, sizeof(double) * m * n);
    return p;
}
int *CMat::imat(int n, int m)
{
    int *p;
    if (n <= 0 || m <= 0)
        return NULL;
    if (!(p = (int *)malloc(sizeof(int) * n * m)))
    {
        cout << "***ERROR(mat):allocate error!" << endl;
        return NULL;
    }
    return p;
}

/* multiply matrix, final shape is n*k in column order -----------------------------------------------------------*/
void CMat::matmul(const char *tr, int n, int k, int m, double alpha, const double *A, const double *B, double beta, double *C)
{
    double d;
    int i, j, x, f = tr[0] == 'N' ? (tr[1] == 'N' ? 1 : 2) : (tr[1] == 'N' ? 3 : 4);

    for (i = 0; i < n; i++)
        for (j = 0; j < k; j++)
        {
            d = 0.0;
            switch (f)
            {
            case 1:
                for (x = 0; x < m; x++)
                    d += A[i + x * n] * B[x + j * m];
                break;
            case 2:
                for (x = 0; x < m; x++)
                    d += A[i + x * n] * B[j + x * k];
                break;
            case 3:
                for (x = 0; x < m; x++)
                    d += A[x + i * m] * B[x + j * m];
                break;
            case 4:
                for (x = 0; x < m; x++)
                    d += A[x + i * m] * B[j + x * k];
                break;
            }
            if (beta == 0.0)
                C[i + j * n] = alpha * d;
            else
                C[i + j * n] = alpha * d + beta * C[i + j * n];
        }
}
int CMat::ludcmp(double *A, int n, int *indx, double *d)
{
    double big, s, tmp, *vv = mat(n, 1);
    int i, imax = 0, j, k;
    *d = 1.0;
    for (i = 0; i < n; i++)
    {
        big = 0.0;
        for (j = 0; j < n; j++)
            if ((tmp = fabs(A[i + j * n])) > big)
                big = tmp;
        if (big > 0.0)
            vv[i] = 1.0 / big;
        else
        {
            free(vv);
            return -1;
        }
    }
    for (j = 0; j < n; j++)
    {
        for (i = 0; i < j; i++)
        {
            s = A[i + j * n];
            for (k = 0; k < i; k++)
                s -= A[i + k * n] * A[k + j * n];
            A[i + j * n] = s;
        }
        big = 0.0;
        for (i = j; i < n; i++)
        {
            s = A[i + j * n];
            for (k = 0; k < j; k++)
                s -= A[i + k * n] * A[k + j * n];
            A[i + j * n] = s;
            if ((tmp = vv[i] * fabs(s)) >= big)
            {
                big = tmp;
                imax = i;
            }
        }
        if (j != imax)
        {
            for (k = 0; k < n; k++)
            {
                tmp = A[imax + k * n];
                A[imax + k * n] = A[j + k * n];
                A[j + k * n] = tmp;
            }
            *d = -(*d);
            vv[imax] = vv[j];
        }
        indx[j] = imax;
        if (A[j + j * n] == 0.0)
        {
            free(vv);
            return -1;
        }
        if (j != n - 1)
        {
            tmp = 1.0 / A[j + j * n];
            for (i = j + 1; i < n; i++)
                A[i + j * n] *= tmp;
        }
    }
    free(vv);
    return 0;
}
void CMat::lubksb(const double *A, int n, const int *indx, double *b)
{
    double s;
    int i, ii = -1, ip, j;
    for (i = 0; i < n; i++)
    {
        ip = indx[i];
        s = b[ip];
        b[ip] = b[i];
        if (ii >= 0)
            for (j = ii; j < i; j++)
                s -= A[i + j * n] * b[j];
        else if (s)
            ii = i;
        b[i] = s;
    }
    for (i = n - 1; i >= 0; i--)
    {
        s = b[i];
        for (j = i + 1; j < n; j++)
            s -= A[i + j * n] * b[j];
        b[i] = s / A[i + i * n];
    }
}

void CMat::CMat_Matmul(const char *tr, int m, int n, int k, double alpha, double *a, int lda, double *b, int ldb, double beta, double *c, int ldc)
{
#ifdef MKL
    CBLAS_TRANSPOSE trsa, trsb;
    trsa = tr[0] == 'N' ? CblasNoTrans : CblasTrans;
    trsb = tr[1] == 'N' ? CblasNoTrans : CblasTrans;
    cblas_dgemm(CblasColMajor, trsa, trsb, m, n, k, alpha, a, lda, b, ldb, beta, c, ldc);
#else
    double d;
    int i, j, x;
    int opr = tr[0] == 'N' ? (tr[1] == 'N' ? 1 : 2) : (tr[1] == 'N' ? 3 : 4);
    for (i = 0; i < m; i++)
    {
        for (j = 0; j < n; j++)
        {
            d = 0;
            switch (opr)
            {
            case 1:
                for (x = 0; x < k; x++)
                    d += a[lda * x + i] * b[ldb * j + x];
                break;
            case 2:
                for (x = 0; x < k; x++)
                    d += a[lda * x + i] * b[ldb * x + j];
                break;
            case 3:
                for (x = 0; x < k; x++)
                    d += a[lda * i + x] * b[ldb * j + x];
                break;
            case 4:
                for (x = 0; x < k; x++)
                    d += a[lda * i + x] * b[ldb * x + j];
                break;
            }
            c[ldc * j + i] = alpha * d + beta * c[ldc * j + i];
        }
    }
#endif
}

int CMat::CMat_Inverse(double *A, int lda, int n)
{
#ifdef MKL
    int ret;
    // mkl_set_num_threads(14);
    lapack_int *ipiv = (int *)calloc(MAX(1, n), sizeof(lapack_int));
    ret = LAPACKE_dgetrf(LAPACK_COL_MAJOR, n, n, A, lda, ipiv);
    if (ret != 0)
    {
        free(ipiv);
        return 0;
    }
    ret = LAPACKE_dgetri(LAPACK_COL_MAJOR, n, A, lda, ipiv);
    free(ipiv);
    return !ret;
#else
    double d, *B;
    int i, j, *indx;
    indx = imat(n, 1);
    B = mat(n, n);
    for (i = 0; i < n; i++)
        memcpy(B + n * i, A + lda * i, sizeof(double) * n);
    if (ludcmp(B, n, indx, &d))
    {
        free(indx);
        free(B);
        return -1;
    }
    for (j = 0; j < n; j++)
    {
        for (i = 0; i < n; i++)
            A[i + j * lda] = 0.0;
        A[j + j * lda] = 1.0;
        lubksb(B, n, indx, A + j * lda);
    }
    free(indx);
    free(B);
    return 0;
#endif
}
int CMat::CMat_Solve(double *A, int lda, int row,
                     double *L, int ldaL, int nrhs,
                     double *X, int ldx)
{
    lapack_int n = row;
    lapack_int info;
    lapack_int *ipiv = new lapack_int[n]; // 主元数组

    // 先复制 L 到 X，因为 LAPACKE_dgesv 会覆盖输入的右端项
    for (int j = 0; j < nrhs; ++j)
    {
        for (int i = 0; i < row; ++i)
        {
            X[i + j * ldx] = L[i + j * ldaL];
        }
    }

    // 解 AX = X
    info = LAPACKE_dgesv(LAPACK_COL_MAJOR, n, nrhs, A, lda, ipiv, X, ldx);

    delete[] ipiv;

    if (info < 0)
    {
        // std::cerr << "CMat_Solve: 第 " << -info << " 个参数非法" << std::endl;
        ;
    }
    else if (info > 0)
    {
        // std::cerr << "CMat_Solve: A(" << info << "," << info << ") 为零，矩阵不可解" << std::endl;
        ;
    }

    return info; // 0 表示成功
}

void CMat::CMat_PrintMatrix(double *mat, int lda, int row, int col, const char *premsg)
{
    if (row < 0 || col < 0)
    {
        printf("***print_matrix wrong input arguments for the row and col ***");
        return;
    }
    int i, j;
    if (strlen(premsg) != 0)
    {
        printf("%s\n", premsg);
    }
    for (i = 0; i < row; i++)
    {
        for (j = 0; j < col; j++)
        {
            printf("%8.4lf ", mat[i + j * lda]);
        }
        printf("\n");
    }
}
void CMat::CMat_PrintMatrix_file(double *mat, int lda, int row, int col, const char *premsg, const char *pfile_name)
{
    static FILE *fp = NULL;
    if (fp == NULL)
    {
        fp = fopen(pfile_name, "w");
    }
    if (row < 0 || col < 0)
    {
        printf("***print_matrix wrong input arguments for the row and col ***");
        return;
    }
    int i, j;
    if (strlen(premsg) != 0)
    {
        printf("%s\n", premsg);
    }
    for (i = 0; i < row; i++)
    {
        for (j = 0; j < col; j++)
        {
            fprintf(fp, "%9.5lf ", mat[i + j * lda]);
        }
        fprintf(fp, "\n");
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
}
