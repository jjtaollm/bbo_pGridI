/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-29 11:53:05
 * @ Modified by: Jun Tao
 * @ Modified time: 2025-01-06 10:50:59
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */

#include <iostream>
#include <cmath>
#include <vector>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_blas.h>
#include "include/Applications/Applications.h"

namespace iono
{
#define DEFAULT_RANGE 150 // 150 km
    class t_data
    {
    public:
        t_data()
        {
            h = weights = gamma = NULL;
        }
        double *h;
        double *weights;
        double *gamma;
        size_t n;
    };

    /* 球形半方差函数 */
    double spherical_variogram(double nugget, double sill, double range, double h)
    {
        if (h <= range)
            return nugget + sill * (1.5 * (h / range) - 0.5 * pow(h / range, 3));
        else
            return nugget + sill;

        // return nugget + sill * (1.5 * (h / range) - 0.5 * pow(h / range, 3));
    }

    static int spherical_f(const gsl_vector *params, void *data, gsl_vector *f)
    {
        double sill = std::exp(gsl_vector_get(params, 0));   // 确保 sill 为非负
        double nugget = std::exp(gsl_vector_get(params, 1)); // 确保 nugget 为非负
        double range = DEFAULT_RANGE;

        t_data *d = static_cast<t_data *>(data);
        size_t n = d->n;

        for (size_t i = 0; i < n; ++i)
        {
            double dist = d->h[i];
            double model;
            if (dist <= range)
            {
                model = nugget + sill * (1.5 * (dist / range) - 0.5 * std::pow(dist / range, 3));
            }
            else
            {
                model = nugget + sill;
            }
            gsl_vector_set(f, i, d->weights[i] * (model - d->gamma[i]));
        }

        return GSL_SUCCESS;
    }

    // 球状模型雅可比矩阵
    static int spherical_df(const gsl_vector *params, void *data, gsl_matrix *J)
    {
        double sill = std::exp(gsl_vector_get(params, 0));   // 确保 sill 为非负
        double nugget = std::exp(gsl_vector_get(params, 1)); // 确保 nugget 为非负
        double range = DEFAULT_RANGE;

        t_data *d = static_cast<t_data *>(data);
        size_t n = d->n;

        for (size_t i = 0; i < n; ++i)
        {
            double dist = d->h[i];
            double drange = dist / range;


            if (dist <= range) {
                double df_dsill = (1.5 * drange - 0.5 * pow(drange, 3));
                gsl_matrix_set(J, i, 0, d->weights[i] * sill * df_dsill);  // 对 log(sill)
            } else {
                gsl_matrix_set(J, i, 0, d->weights[i] * sill);
            }
            // 对 log(nugget)，偏导恒为 nugget
            gsl_matrix_set(J, i, 1, d->weights[i] * nugget);

            // if (dist <= range)
            // {
            //     gsl_matrix_set(J, i, 0, d->weights[i] * sill * (1.5 * drange - 0.5 * std::pow(drange, 3)));
            // }
            // else
            // {
            //     gsl_matrix_set(J, i, 0, d->weights[i] * sill);
            // }
            // gsl_matrix_set(J, i, 1, d->weights[i] * nugget);
        }

        return GSL_SUCCESS;
    }

    // 更新权重函数
    static void update_weights(gsl_multifit_fdfsolver *solver, t_data &d, const gsl_vector *residuals)
    {
        double k0 = 1.5, k1 = 3.5;
        size_t n = d.n, n_del = 0, n_down = 0;
        double err_sum = 0.0;
        for (size_t i = 0; i < n; ++i)
        {
            err_sum = err_sum + gsl_vector_get(residuals, i) * gsl_vector_get(residuals, i);
        }
        err_sum = sqrt(err_sum / n);

        for (size_t i = 0; i < n; ++i)
        {
            double v = fabs(gsl_vector_get(residuals, i) / err_sum);
            if (v < k0)
                continue;
            if (v < k1)
            {
                double scal = k0 / v * pow((k1 - v) / (k1 - k0), 2);
                d.weights[i] = d.weights[i] * sqrt(scal);
                ++n_del;
            }
            else
            {
                double scal = 1e-4;
                d.weights[i] = d.weights[i] * sqrt(scal);
                ++n_down;
            }
        }
        // printf("%d %d\n", n_down, n_del);
    }

    t_VariogramM spherical_fit(const size_t n, double *h, double *gamma)
    {
        t_data d;
        d.h = h;
        d.gamma = gamma;
        d.n = n;
        d.weights = (double *)calloc(n, sizeof(double));
        for (int i = 0; i < n; ++i)
            d.weights[i] = 1.0;

        const size_t p = 2; // 参数数量: range, sill, nugget

        gsl_vector *params = gsl_vector_alloc(p);
        gsl_vector_set(params, 0, std::log(1.0));  // 初始 sill 的对数值
        gsl_vector_set(params, 1, std::log(1e-6)); // 初始 nugget 的对数值，避免 log(0)

        gsl_multifit_function_fdf f;
        f.f = spherical_f;
        f.df = spherical_df;
        f.n = n;
        f.p = p;
        f.params = &d;

        const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;
        gsl_multifit_fdfsolver *solver = gsl_multifit_fdfsolver_alloc(T, n, p);
        gsl_multifit_fdfsolver_set(solver, &f, params);

        // 迭代拟合
        int status;
        size_t iter = 0;
        const size_t max_iter = 500;
        do
        {
            iter++;
            status = gsl_multifit_fdfsolver_iterate(solver);
            if (status)
                break;
            gsl_vector *residuals = gsl_multifit_fdfsolver_residual(solver);
            update_weights(solver, d, residuals);
            status = gsl_multifit_test_delta(solver->dx, solver->x, 1e-4, 1e-4);
        } while (status == GSL_CONTINUE && iter < max_iter);

        // 获取拟合结果
        double sill = std::exp(gsl_vector_get(solver->x, 0));   // 将对数值转换回实际值
        double nugget = std::exp(gsl_vector_get(solver->x, 1)); // 同上

        t_VariogramM rvl;
        rvl.nugget = nugget;
        rvl.sill = sill;
        rvl.range = DEFAULT_RANGE;
        rvl.TYPE = 'S';
        rvl.b_isok = true;
        // 释放内存
        gsl_multifit_fdfsolver_free(solver);
        gsl_vector_free(params);
        if (d.weights)
            free(d.weights);
        return rvl;
    }
}