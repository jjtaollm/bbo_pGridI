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
#include <algorithm>
#include "include/Applications/Applications.h"

namespace iono
{
#define DEFAULT_RANGE 150 // 150 km
    struct t_vbin
    {
        double q = 0.0;
        double g = 0.0;
        double w = 0.0;
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

    static double s_spherical_q(double h)
    {
        if (h >= DEFAULT_RANGE)
            return 1.0;
        double u = h / DEFAULT_RANGE;
        return 1.5 * u - 0.5 * u * u * u;
    }

    static vector<t_vbin> s_form_bins(size_t n, double *h, double *g)
    {
        const int nbin = 6;
        vector<t_vbin> tmp(nbin + 1), bins;
        vector<int> num(nbin + 1, 0);
        for (size_t i = 0; i < n; ++i)
        {
            if (!std::isfinite(h[i]) || !std::isfinite(g[i]) || h[i] < 0.0 || g[i] < 0.0)
                continue;
            int ibin = h[i] >= DEFAULT_RANGE ? nbin : std::min(nbin - 1, int(h[i] / DEFAULT_RANGE * nbin));
            tmp[ibin].q += s_spherical_q(h[i]);
            tmp[ibin].g += g[i];
            num[ibin]++;
        }
        for (int i = 0; i <= nbin; ++i)
        {
            if (num[i] == 0)
                continue;
            tmp[i].q /= num[i];
            tmp[i].g /= num[i];
            tmp[i].w = sqrt(double(num[i]));
            bins.push_back(tmp[i]);
        }
        return bins;
    }

    static double s_median(vector<double> v)
    {
        if (v.empty())
            return 0.0;
        size_t n = v.size() / 2;
        nth_element(v.begin(), v.begin() + n, v.end());
        double m = v[n];
        if (v.size() % 2 == 0)
        {
            nth_element(v.begin(), v.begin() + n - 1, v.end());
            m = (m + v[n - 1]) / 2.0;
        }
        return m;
    }

    static double s_fit_err(const vector<t_vbin> &d, const vector<double> &rw, double nugget, double sill)
    {
        double err = 0.0;
        for (size_t i = 0; i < d.size(); ++i)
        {
            double e = nugget + sill * d[i].q - d[i].g;
            err += d[i].w * rw[i] * e * e;
        }
        return err;
    }

    static bool s_fit_once(const vector<t_vbin> &d, const vector<double> &rw, double &nugget, double &sill)
    {
        double s0 = 0.0, s1 = 0.0, s2 = 0.0, t0 = 0.0, t1 = 0.0;
        for (size_t i = 0; i < d.size(); ++i)
        {
            double w = d[i].w * rw[i];
            double q = d[i].q;
            s0 += w;
            s1 += w * q;
            s2 += w * q * q;
            t0 += w * d[i].g;
            t1 += w * q * d[i].g;
        }
        if (s0 <= 0.0)
            return false;

        double det = s0 * s2 - s1 * s1;
        if (det > 1e-12 * std::max(1.0, s0 * s2))
        {
            double n0 = (t0 * s2 - t1 * s1) / det;
            double s0_ = (s0 * t1 - s1 * t0) / det;
            if (n0 >= 0.0 && s0_ >= 0.0)
            {
                nugget = n0;
                sill = s0_;
                return true;
            }
        }

        double n1 = std::max(0.0, t0 / s0), s1_ = 0.0;
        double n2 = 0.0, s2_ = s2 > 0.0 ? std::max(0.0, t1 / s2) : 0.0;
        if (s_fit_err(d, rw, n1, s1_) < s_fit_err(d, rw, n2, s2_))
            nugget = n1, sill = s1_;
        else
            nugget = n2, sill = s2_;
        return true;
    }

    t_VariogramM spherical_fit(const size_t n, double *h, double *gamma)
    {
        t_VariogramM rvl;
        rvl.range = DEFAULT_RANGE;
        rvl.TYPE = 'S';
        if (!h || !gamma || n < 2)
            return rvl;

        vector<t_vbin> d = s_form_bins(n, h, gamma);
        if (d.empty())
            return rvl;
        if (d.size() == 1)
        {
            rvl.nugget = 0.0;
            rvl.sill = std::max(1e-10, d[0].g);
            rvl.b_isok = std::isfinite(rvl.sill);
            return rvl;
        }

        double nugget = 0.0, sill = 0.0;
        vector<double> rw(d.size(), 1.0);
        for (int ite = 0; ite < 6; ++ite)
        {
            if (!s_fit_once(d, rw, nugget, sill))
                return rvl;

            vector<double> e(d.size()), ae(d.size());
            for (size_t i = 0; i < d.size(); ++i)
            {
                e[i] = nugget + sill * d[i].q - d[i].g;
                ae[i] = fabs(e[i]);
            }
            double sig = 1.4826 * s_median(ae);
            if (sig <= 1e-12)
                break;

            double dif = 0.0;
            for (size_t i = 0; i < d.size(); ++i)
            {
                double u = fabs(e[i]) / sig;
                double w = u <= 1.5 ? 1.0 : 1.5 / u;
                dif = std::max(dif, fabs(w - rw[i]));
                rw[i] = w;
            }
            if (dif < 1e-3)
                break;
        }

        rvl.nugget = std::max(0.0, nugget);
        rvl.sill = std::max(1e-10, sill);
        rvl.b_isok = std::isfinite(rvl.nugget) && std::isfinite(rvl.sill);
        return rvl;
    }
}
