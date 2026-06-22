/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-28 23:54:33
 * @ Modified by: Jun Tao
 * @ Modified time: 2025-01-06 10:42:35
 * @ Description: simplified version with sigma computation
 */

#include <set>
#include <cmath>
#include "include/Applications/Applications.h"
#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace iono;
using namespace std;
Interp_kriging_smp::Interp_kriging_smp()
{
    _pool = nullptr;
}
Interp_kriging_smp::~Interp_kriging_smp()
{
    if (_pool)
        delete _pool;
}

void Interp_kriging_smp::_kriging_single_p(time_t t, int isat,     /* processed satellites */
                                           vector<string> &snames, /* names for the data */
                                           vector<int> &sindex,    /* base station index, that contains */
                                           vector<Station> &reqs,  /* processed requests */
                                           map<string, map<int, StecC>> &data_sd)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    map<string, t_Sunit> grid_all;
    {
        vector<t_SampPt> pts = _prepare_samples(snames, sindex, data_sd, isat);
        if (pts.size() < 4)
            return;
        t_VariogramM m = _kriging_est_hyperpam(pts, 'S');
        if (!m.b_isok)
            return;
        /* kriging interpolate */
        map<string, t_Sunit> grid_v = _kriging_interp_onesat(t, isat, reqs, pts, m); /* iq --> satellites */
        for (auto &kv : grid_v)
            grid_all[kv.first] = kv.second;
    }
    {
        /* add nodes to the gridV */
        std::lock_guard<std::mutex> lock(_mtx);
        for (auto &kv : grid_all)
        {
            if (mGridv.count(kv.first) == 0)
                mGridv[kv.first] = t_gridV();
            mGridv[kv.first].gridv[isat] = kv.second;
        }
    }
}

void Interp_kriging_smp::_interp_r_one(int isat, int *refsat, vector<string> &snames, map<string, map<int, StecC>> &data_sd)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    vector<int> sindex(snames.size());
    for (int i = 0; i < snames.size(); ++i)
        sindex[i] = i;
    vector<t_SampPt> pts = _prepare_samples(snames, sindex, data_sd, isat);
    if (pts.size() < 4)
        return;
    t_VariogramM m = _kriging_est_hyperpam(pts, 'S');
    if (!m.b_isok)
        return;
    map<int, double> pt2v = _kriging_get_gridresidual(pts, m); /* sta -> v */
    {
        std::lock_guard<std::mutex> lock(_mtx);
        int isys = index_string(SYS, dly->prn_alias[isat][0]), _r = refsat[isys];
        if (_r != -1 && m_residual.count(_r) == 0)
            m_residual[_r] = map<int, double>();
        for (auto a : pt2v)
        {
            if (m_residual.count(isat) == 0)
                m_residual[isat] = map<int, double>();
            m_residual[isat][pts[a.first].sta_index] = a.second; /* isat -> sit -> v */

            if (m_residual[_r].count(pts[a.first].sta_index) == 0)
                m_residual[_r][pts[a.first].sta_index] = 0.0; /* isat-> sit -> _r 0.0 here */
        }
    }
}
void Interp_kriging_smp::_interp_r_all(time_t t, int *refsat, vector<string> &snames, std::set<int> &sates, map<string, map<int, StecC>> &data_sd)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    m_residual.clear();
    Log::s_tmeTag("residual");
    for (auto &isat : sates)
    {
        if (!_pool)
        {
            _interp_r_one(isat, refsat, snames, data_sd);
        }
        else
        {
            _pool->append(std::bind(&Interp_kriging_smp::_interp_r_one, this, isat, refsat, snames, data_sd));
        }
    }
    if (_pool)
        _pool->sync();
    /* smooth residual through multi-epochs windows */
    _update_window(t, refsat, m_residual, m_r_window);
    Log::s_tmeConsume("residual");
}

void Interp_kriging_smp::_process(time_t t, int *refsat, vector<string> &snames, std::set<int> &sates, vector<Station> &grids, map<string, map<int, StecC>> &data_sd)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    vector<int> sindex(snames.size());
    for (int i = 0; i < snames.size(); ++i)
        sindex[i] = i;
    /* interpolate for CV */
    _interp_r_all(t, refsat, snames, sates, data_sd);
    /* interpolate for each grid */
    {
        /* using thread to interpolate for each grid */
        for (auto &isat : sates)
        {
            if (!_pool)
                _kriging_single_p(t, isat, snames, sindex, grids, data_sd);
            else
            {
                _pool->append(std::bind(&Interp_kriging_smp::_kriging_single_p, this, t, isat, snames, sindex, grids, data_sd));
            }
        }
        if (_pool)
            _pool->sync();
    }
}

/// @brief
/// @param lon0 mid longitude in degrees
/// @param grids request list
/// @param data samples
map<string, t_gridV> Interp_kriging_smp::_interp_kriging(int mjd, double sod, int *refsat_in, vector<Station> mreqs, map<string, map<int, StecC>> &data)
{
    function _get_satu = [this](map<string, map<int, StecC>> &data, int *refsat) -> set<int>
    {
        set<int> s_set;
        Deploy *dly = Controller::s_getInstance()->m_getConfigure();
        for (auto &s2v : data)
        {
            for (auto &sat2v : s2v.second)
            {
                int isys = index_string(SYS, dly->prn_alias[sat2v.first][0]);
                if (sat2v.first == refsat[isys])
                    continue;
                s_set.insert(sat2v.first);
            }
        }
        return s_set;
    };
    /* 单个请求进行处理 */
    mGridv.clear();
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    vector<string> snames;
    int refsat[MAXSYS] = {0};
    for (auto &[s, _] : data)
        snames.push_back(s);
    for (int &ref : refsat)
        ref = -1;
    /* step 1, get reference satellite */
    if (refsat_in != NULL)
        memcpy(refsat, refsat_in, sizeof(int) * MAXSYS);
    else if (!_kriging_set_ref(data, refsat))
        return mGridv;
    {
        /* step 3, form single-difference ionosphere */
        map<std::string, map<int, StecC>> data_sd = _kriging_form_sd(data, refsat);
        std::set<int> sates = _get_satu(data, refsat);
        LOGPRINT_EX("[%d %9.1lf] Processing for %s %02d...", mjd, sod, mreqs.front().name.c_str(), data_sd.size());
        _process(mjd2time(mjd, sod).time, refsat, snames, sates, mreqs, data_sd);
        LOGPRINT_EX("[%d %9.1lf] Processing Done %s", mjd, sod, mreqs.front().name.c_str());
        for (auto &kv : mGridv)
            memcpy(kv.second.refsat, refsat, sizeof(int) * MAXSYS);
    }
    return mGridv;
}
vector<t_SampPt> Interp_kriging_smp::_prepare_samples(vector<string> &snames, vector<int> index, map<std::string, map<int, StecC>> &data, int isat)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    vector<t_SampPt> pts;

    for (auto &id : index)
    {
        string &s = snames[id];
        if (data.count(s) == 0)
            continue;
        for (auto &sat2v : data[s])
        {
            if (sat2v.first == isat)
            {
                t_SampPt pt;
                pt.sta_index = id; /* index of snames */
                pt.x = sat2v.second.gausx[0] / 1000.0;
                pt.y = sat2v.second.gausx[1] / 1000.0;
                pt.z = sat2v.second.ionva;
                pts.push_back(pt);
            }
        }
    }

    return pts;
}

bool Interp_kriging_smp::_kriging_set_ref(map<std::string, map<int, StecC>> &data, int *ref)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    map<char, int> rvl, rvl_e;
    int prn_count[MAXSAT][2] = {0};
    for (const auto &sit2v : data)
    {
        for (const auto &sat2v : sit2v.second)
        {
            int isat = sat2v.first;
            // if (dly->cprn[isat] == "G31")
            //     continue;
            prn_count[isat][0]++;
            prn_count[isat][1] = int(sat2v.second.elev);
        }
    }

    // 每个系统得到最大公共站最多且高度较最高的卫星号
    for (int i = 0; i < dly->nprn; ++i)
    {
        if (prn_count[i][0] == 0)
            continue;
        char sys = dly->prn_alias[i][0];
        if (rvl.count(sys) == 0 || prn_count[rvl[sys]][0] < prn_count[i][0] ||
            ((prn_count[rvl[sys]][0] == prn_count[i][0] && rvl_e[sys] < prn_count[i][1])))
        {
            rvl[sys] = i;
            rvl_e[sys] = prn_count[i][1];
        }
    }
    /* update results */
    for (int isys = 0; isys < MAXSYS; ++isys)
        ref[isys] = -1;
    for (auto &kv : rvl)
        ref[index_string(SYS, kv.first)] = kv.second;

    return rvl.size() > 0;
}
map<std::string, map<int, StecC>> Interp_kriging_smp::_kriging_form_sd(map<std::string, map<int, StecC>> &data, int *refsat)
{
    int isys, jsys;
    map<std::string, map<int, StecC>> rvl;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();

    for (auto &sit2v : data)
    {
        for (jsys = 0; jsys < dly->nsys; ++jsys)
        {
            isys = index_string(SYS, dly->system[jsys]);
            if (refsat[isys] == -1 || sit2v.second.count(refsat[isys]) == 0) /* no reference or no this reference */
                continue;
            for (auto &sat2v : sit2v.second)
            {
                int isat = sat2v.first;
                if (isat == refsat[isys] || dly->prn_alias[isat][0] != SYS[isys])
                    continue;
                StecC c = sat2v.second;
                c.ionva = c.ionva - sit2v.second[refsat[isys]].ionva;
                rvl[sit2v.first][isat] = c;
            }
        }
    }
    return rvl;
}
t_VariogramM Interp_kriging_smp::_kriging_est_hyperpam(vector<t_SampPt> &pts, const char type)
{
    t_VariogramM m;
    if (type == 'S')
        m = _kriging_est_hyperpam_spherical(pts);
    return m;
}

t_VariogramM Interp_kriging_smp::_kriging_est_hyperpam_spherical(vector<t_SampPt> &pts)
{
    function _cal_hyper_samples = [](vector<t_SampPt> &pts, double **h, double **gama, int *nb) -> void
    {
        vector<double> h_, gama_;
        for (size_t i = 0; i < pts.size(); ++i)
        {
            for (size_t j = i + 1; j < pts.size(); ++j)
            {
                double dx = pts[i].x - pts[j].x;
                double dy = pts[i].y - pts[j].y;
                double dist = sqrt(dx * dx + dy * dy);
                double semivar = 0.5 * (pts[i].z - pts[j].z) * (pts[i].z - pts[j].z);
                h_.push_back(dist);
                gama_.push_back(semivar);
            }
        }
        int sz = h_.size();
        *h = new double[sz];
        *gama = new double[sz];

        std::copy_n(h_.begin(), sz, *h);
        std::copy_n(gama_.begin(), sz, *gama);

        *nb = sz;
        return;
    };
    int nb = 0;
    double *h = NULL, *gama = NULL;
    /* step 2, compute the distance and the sig */
    _cal_hyper_samples(pts, &h, &gama, &nb);

    t_VariogramM m;
    /* step 3, fit the spherical functions */
    if (m.TYPE == 'S')
        m = spherical_fit(nb, h, gama);
    if (h)
        delete[] h;
    if (gama)
        delete[] gama;
    return m;
}

static void _form_obs(vector<t_SampPt> &pts, t_VariogramM &m, int idel, double *A, int lda)
{
    vector<t_SampPt *> p_ptr;
    memset(A, 0, sizeof(double) * lda * lda);
    for (int i = 0; i < pts.size(); ++i)
    {
        if (i == idel)
            continue;
        p_ptr.push_back(&pts[i]);
    }
    int nd = p_ptr.size();
    for (int i = 0; i < nd; ++i)
    {
        for (int j = i; j < nd; ++j)
        {
            double dist = sqrt(pow((p_ptr[i]->x - p_ptr[j]->x), 2) + pow((p_ptr[i]->y - p_ptr[j]->y), 2));
            double v = spherical_variogram(m.nugget, m.sill, m.range, dist);
            A[i * lda + j] = A[j * lda + i] = v;
        }
    }
    for (int i = 0; i < nd; ++i)
        A[i * lda + nd] = A[nd * lda + i] = 1;
    A[nd * lda + nd] = 0;
}
static double nearest_neighbor_avg(const std::vector<t_SampPt> &pts)
{
    int N = pts.size();
    if (N < 2)
        return 999;

    double sum = 0.0;
    for (int i = 0; i < N; i++)
    {
        double minDist = std::numeric_limits<double>::max();
        for (int j = 0; j < N; j++)
        {
            if (i == j)
                continue;
            double dx = pts[i].x - pts[j].x;
            double dy = pts[i].y - pts[j].y;
            double dist = std::sqrt(dx * dx + dy * dy);
            if (dist < minDist)
                minDist = dist;
        }
        sum += minDist;
    }
    return sum / N;
}
map<string, t_Sunit> Interp_kriging_smp::_kriging_interp_onesat(time_t t, int isat, vector<Station> &grids, vector<t_SampPt> &pts, t_VariogramM &m)
{
    int MAX_DIST = 450; // 130 kilometers
    map<string, t_Sunit> v_r;
    map<int, double> pt2v; /* pt 2 residual */
    /* step 1, form the observation matrix, iterate to compute */
    int nd = pts.size(), nq = grids.size();
    double *A = (double *)calloc((nd + 1) * (nd + 1), sizeof(double));
    double *L = (double *)calloc((nd + 1) * nq, sizeof(double));
    double *C = (double *)calloc((nd + 1) * nq, sizeof(double));
    double *Z = (double *)calloc(nd, sizeof(double));
    double *Iv = (double *)calloc(nq, sizeof(double));
    double *nsig = (double *)calloc(nq, sizeof(double));
    for (int i = 0; i < nd; ++i)
        Z[i] = pts[i].z;
    /* form the observations observations */
    _form_obs(pts, m, -1, A, nd + 1);
    /* step 2, iterate to compute residual of each samples */
    for (size_t iq = 0; iq < grids.size(); ++iq)
    {
        for (int i = 0; i < nd; ++i)
        {
            double dist = sqrt(pow((pts[i].x - grids[iq].gausx[0] / 1000.0), 2) + pow((pts[i].y - grids[iq].gausx[1] / 1000.0), 2));
            double v = spherical_variogram(m.nugget, m.sill, m.range, dist);
            L[iq * (nd + 1) + i] = v;
        }
        L[iq * (nd + 1) + nd] = 1.0;
    }
    /* step 3, output grids and its corresponding sigma */
    // CMat::CMat_Inverse(A, nd + 1, nd + 1);
    // CMat::CMat_Matmul("NN", nd + 1, nq, nd + 1, 1.0, A, nd + 1, L, nd + 1, 0.0, C, nd + 1);
    double aveDist = nearest_neighbor_avg(pts);
    if (!CMat::CMat_Solve(A, nd + 1, nd + 1, L, nd + 1, nq, C, nd + 1))
    {
        /* step 4, using coefficients to compute Z */
        CMat::CMat_Matmul("NN", 1, nq, nd, 1.0, Z, 1, C, nd + 1, .0, Iv, 1);
        /* compute the formal error */
        for (size_t iq = 0; iq < grids.size(); ++iq)
        {
            t_Sunit pv;
            pv.isat = isat;
            pv.v = Iv[iq];
            pv.nobs = nd;
            pv.aveDist = aveDist;

            {
                double sum = 0.0;
                for (int i = 0; i < nd; ++i)
                {
                    sum = sum + L[iq * (nd + 1) + i] * C[iq * (nd + 1) + i];
                }
                // LOGPRINT("sum is %9.3lf %9.3lf", sum, C[iq * (nd + 1) + nd]);
                // sum = sum - C[iq * (nd + 1) + nd];
                sum = sum + C[iq * (nd + 1) + nd];
                pv.vsign = sqrt(sum);
            }
            v_r[grids[iq].name] = pv;
        }
        /* update residuals */
        for (int i = 0; i < pts.size(); ++i)
            if (m_residual.count(isat) && m_residual[isat].count(pts[i].sta_index))
                pt2v[i] = m_residual[isat][pts[i].sta_index];
        /* using the residual and change it into grids */
        for (size_t iq = 0; iq < grids.size(); ++iq)
        {
            double wgt_sum = 0.0, v = 0.0;
            for (auto &samp : pt2v)
            {
                double dist = sqrt(pow((grids[iq].gausx[0] / 1000.0 - pts[samp.first].x), 2) + pow((grids[iq].gausx[1] / 1000.0 - pts[samp.first].y), 2));
                // if (dist < MAX_DIST)
                {
                    v = v + samp.second / dist;
                    wgt_sum = wgt_sum + 1.0 / dist;
                }
            }
            if (wgt_sum != 0.0)
            {
                v = v / wgt_sum;                    /* the difference */
                v_r[grids[iq].name].vsig = fabs(v); /* assume the value is around 68%*/
            }
            else
            {
                v_r[grids[iq].name].vsig = INVALID_VAL;
            }
        }
    }
    free(A);
    free(L);
    free(C);
    free(Z);
    free(Iv);
    return v_r;
}
/// @brief cross validation to compute sigma
/// @param pts
/// @param m
/// @return
map<int, double> Interp_kriging_smp::_kriging_get_gridresidual(vector<t_SampPt> &pts, t_VariogramM &m)
{
    map<int, double> pt2v, pt2t;
    int nd = pts.size(), i, j, n;
    double *A = (double *)calloc(nd * nd, sizeof(double));
    double *L = (double *)calloc(nd, sizeof(double));
    double *C = (double *)calloc(nd, sizeof(double));

    for (i = 0; i < nd; ++i) /* iterate to exclude the selected sample */
    {
        pt2t[i] = pts[i].z;
        _form_obs(pts, m, i, A, nd); /* form observations */
        for (n = j = 0; j < nd; ++j)
        {
            if (i == j)
                continue;
            double dist = sqrt(pow((pts[i].x - pts[j].x), 2) + pow((pts[i].y - pts[j].y), 2));
            double v = spherical_variogram(m.nugget, m.sill, m.range, dist);
            L[n++] = v;
        }
        L[nd - 1] = 1.0;
        /* step 3, output grids and its corresponding sigma */
        // CMat::CMat_Inverse(A, nd, nd);
        // CMat::CMat_Matmul("NN", nd, 1, nd, 1.0, A, nd, L, nd, 0.0, C, nd);

        if (!CMat::CMat_Solve(A, nd, nd, L, nd, 1, C, nd))
        {
            /* step 4, using coefficients to compute Z */
            double zi = 0.0;
            for (j = 0, n = 0; j < nd; ++j)
            {
                if (j == i)
                    continue;
                zi = zi + C[n++] * pts[j].z;
            }
            pt2v[i] = fabs(zi - pts[i].z);
        }
    }
    free(A);
    free(L);
    free(C);
    return pt2v;
}
void Interp_kriging_smp::_update_window(time_t t, int *refsat, map<int, map<int, double>> &s2idxv, map<time_t, map<int, map<int, double>>> &s2idxv_list)
{
    function rms = [](list<double> &v) -> double
    {
        double sum = 0.0;
        for (auto &x : v)
        {
            sum = sum + x * x;
        }
        return sqrt(sum / v.size());
    };
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    for (int jsys = 0; jsys < dly->nsys; ++jsys)
    {
        int isys = index_string(SYS, dly->system[jsys]);
        if (refsat[isys] != -1)
        {
            for (auto &kv : s2idxv) /* iterate every satellite */
            {
                const int &isat = kv.first;
                if (dly->prn_alias[isat][0] != SYS[isys])
                    continue;
                for (auto &kv_sit : kv.second)
                {
                    const int &isit = kv_sit.first;
                    list<double> _ser(1, kv_sit.second);
                    for (auto &t2v : s2idxv_list)
                    {
                        /* no reference satellite and no this station */
                        if (t2v.second.count(refsat[isys]) == 0 || t2v.second[refsat[isys]].count(isit) == 0)
                            continue;
                        /* no this satellite and no this station */
                        if (t2v.second.count(isat) == 0 || t2v.second[isat].count(isit) == 0)
                            continue;
                        /* form sd */
                        double d_r = t2v.second[isat][isit] - t2v.second[refsat[isys]][isit];
                        _ser.push_back(d_r);
                    }
                    kv_sit.second = rms(_ser);
                }
            }
            /* update current to list */
            s2idxv_list[t] = s2idxv;
            for (auto itr = s2idxv_list.begin(); itr != s2idxv_list.end();)
            {
                if (t - itr->first > 150)
                {
                    itr = s2idxv_list.erase(itr);
                    continue;
                }
                ++itr;
            }
        }
    }
}
