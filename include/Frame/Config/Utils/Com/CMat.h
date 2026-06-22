/*
 * CMat.h
 *
 *  Created on: 2018/3/12
 *      Author: juntao, at wuhan university
 */

#ifndef INCLUDE_COM_CMAT_H_
#define INCLUDE_COM_CMAT_H_

#include <iostream>
#include <string>
using namespace std;
namespace iono
{
	class CMat
	{
	public:
		//// double precision part
		static void CMat_Matmul(const char *tr, int m, int n, int k, double alpha, double *a, int lda, double *b, int ldb, double beta, double *c, int ldc);
		static int CMat_Inverse(double *A, int lda, int n);
		static void CMat_PrintMatrix(double *mat, int lda, int row, int col, const char *premsg);
		static void CMat_PrintMatrix_file(double *mat, int lda, int row, int col, const char *premsg, const char *pfile);
		static void matmul(const char *tr, int n, int k, int m, double alpha, const double *A, const double *B, double beta, double *C);
		static int CMat_Solve(double *A, int lda, int row,
							  double *L, int ldaL, int nrhs,
							  double *X, int ldx);

	protected:
		static double *mat(int n, int m);
		static int *imat(int n, int m);
		static void lubksb(const double *A, int n, const int *indx, double *b);
		static int ludcmp(double *A, int n, int *indx, double *d);
	};
}
#endif /* INCLUDE_COM_CMAT_H_ */
