/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-28 21:14:52
 * @ Modified by: Jun Tao
 * @ Modified time: 2024-10-10 22:50:58
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */

#include "include/Applications/Applications.h"
#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
#include <string>
#include <list>
#include <vector>
#include <cmath>
#include <iostream>
#include <iomanip>
using namespace std;
using namespace iono;

polyModel::polyModel(int n, int m, double lon0, double lat0, char csys)
{
    this->mM = m;
    this->mN = n;
    this->mlat0 = lat0;
    this->mlon0 = lon0;
    this->m_csys = csys;
    m_tag_coef_fil = "POLY_COEF", m_tag_res_fil = "POLY_RES";
}
polypm::polypm(int isat, char csys, string s, int tp, int i, int j)
{
    this->isat = isat;
    this->csys = csys;
    this->sname = s;
    this->tp = tp;
    this->i = i;
    this->j = j;
    xini = xcor = xest = 0.0;
    iobs = 0;
}
polypm::~polypm()
{
}
polyModel::polyModel(char csys)
{
    // 构造函数实现
    this->m_csys = csys;
    m_tag_coef_fil = "POLY_COEF", m_tag_res_fil = "POLY_RES";
}
polyModel::~polyModel()
{
}
void polyModel::_read_DCB(int mjd, double sod, bool breset)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    /* correct the DCB values here */
    if (breset)
    {
        reader.m_setAdapter(&dcbAdapter);
        reader.m_read_DCB(dly->dcbfile);
    }
}
void polyModel::_union_data(time_t time, map<string, map<int, StecC>> &data)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    map<time_t, map<string, map<int, StecC>>>::iterator itr;
    for (itr = m_data.begin(); itr != m_data.end();)
    {
        if (time - itr->first > span)
        {
            itr = m_data.erase(itr);
            continue;
        }
        ++itr;
    }

    map<string, map<int, StecC>> payload;
    for (auto &[_, v1] : data)
    {
        for (auto &[isat, v2] : v1)
        {
            if (dly->prn_alias[isat][0] == m_csys)
                payload[_][isat] = v2;
        }
    }
    m_data[time] = payload;
}
void polyModel::v_run(int mjd, double sod, map<string, map<int, StecC>> &data)
{
    int sat2o[MAXSAT] = {0};
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();

    /* step 1,union data with current data */
    _union_data(mjd2time(mjd, sod).time, data);

    /* step 2, corrected the DCB values */
    _read_DCB(mjd, sod, mjd != mjdsync);
    /* step 3, get the current estimated list */
    int nobs_all = _get_estimated_list(m_data, sat2o);

    /* step 4, update current extimated parameters and lat0/lon0 */
    _update_estimated_conf(mjd2time(mjd, sod).time, m_data);

    /* step 5, estimated values */
    _estimated(m_data, nobs_all, sat2o);

    /* step 6, get coefficientents */
    m_coef = _get_estimated_coeff();

    /* step 7, output residuals */

    _output(mjd, sod, m_data[mjd2time(mjd, sod).time]);

    mjdsync = mjd;
}

int polyModel::m_form_norm_and_solve(int nb, double *A, int lda, double *L, int ldaL, double *AtA, int ldAtA)
{
    /* form the norm matrix */
    int npms = this->m_Pm.size(), bok = 1;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    double *AtL = (double *)calloc(npms, sizeof(double));
    CMat::CMat_Matmul("TN", npms, npms, nb, 1.0, A, lda, A, lda, 0.0, AtA, ldAtA); /* form normal matrix */
    CMat::CMat_Matmul("TN", npms, 1, nb, 1.0, A, lda, L, lda, 0.0, AtL, npms);     /* update AtL */
    for (size_t ip = 0; ip < m_Pm.size(); ip++)
        if (m_Pm[ip].iobs == 0)
            AtA[ip * npms + ip] = AtA[ip * npms + ip] + 1e5; /* add zero contraint */
    if (true)
    {
        /* add constraint for this system  */
        if (!_add_center_constraint(AtA, npms, m_csys, true)) /* 适用于参数采用测站的经纬度 */
            _add_center_constraint(AtA, npms, m_csys, false);
    }
    else
    {
        _add_single_constraint(AtA, npms, m_csys);
    }
    double *x = (double *)calloc(npms, sizeof(double));
    /* 求解多项式系数 */
    if (!CMat::CMat_Solve(AtA, ldAtA, npms, AtL, npms, 1, x, npms))
    {
        for (int i = 0; i < npms; ++i)
        {
            m_Pm[i].xcor = x[i];
            m_Pm[i].xest = m_Pm[i].xini + m_Pm[i].xcor;
        }
        bok = 1;
    }
    else
        bok = 0;

    free(AtL);
    free(x);
    return bok;
}
bool polyModel::_b_gotDCBCorrection(string sname, char csys)
{
    map<string, map<char, double>> &dcbv = dcbAdapter.s2dcb;
    if (dcbv.find(sname) != dcbv.end() && dcbv[sname].find(csys) != dcbv[sname].end())
        return true;
    else
        return false;
}
void polyModel::_init_DCB_pam(double *AtA, int lda)
{
    double stec_sig = 10.0;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    int npms = this->m_Pm.size();
    map<string, map<char, double>> &dcbv = dcbAdapter.s2dcb;
    map<string, map<char, double>> &dcbv_sig = dcbAdapter.s2dcb_sig;
    for (int i = 0; i < npms; ++i)
    {
        if (m_Pm[i].tp == 2)
        {
            polypm &pm = m_Pm[i];
            if (_b_gotDCBCorrection(pm.sname, pm.csys))
            {
                int isys = index_string(SYS, pm.csys);
                double fq = freqbysys(isys, -1, dly->freq[isys][0]);
                double stec = dcbv[pm.sname][SYS[isys]] / 1e9 * VEL_LIGHT * fq * fq / 40.28 / 1e16;
                pm.xini = stec;                               /* update initial value */
                AtA[i * lda + i] = 1.0 / stec_sig / stec_sig; /* update the priori P */
            }
        }
    }
}
void polyModel::_estimated(map<time_t, map<string, map<int, StecC>>> &data, int nobs, int *sat2no)
{
    bool bcont = true;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    int npms = this->m_Pm.size(), ite = 0, maxobs = nobs + m_Pm.size() + dly->nsys, nb_epoch;
    int (*nb2idx)[4] = (int (*)[4])calloc(maxobs * 4, sizeof(int));
    vector<double> A(npms * maxobs, 0), L(maxobs, 0), AtA(npms * npms, 0);
    while (bcont)
    {
        bcont = false;
        A.assign(A.size(), 0.0);
        L.assign(L.size(), 0.0);
        AtA.assign(AtA.size(), 0);
        memset(nb2idx, 0, sizeof(int) * maxobs * 4);
        /* step 1， ADD DCB initial values */
        _init_DCB_pam(AtA.data(), npms);
        /* step 1, form the observation matrix */
        nb_epoch = _form_A_matrix(data, A.data(), maxobs, L.data(), sat2no, nb2idx, true);
        /* step 2, solve the equations */
        if (m_form_norm_and_solve(nb_epoch, A.data(), maxobs, L.data(), maxobs, AtA.data(), npms))
        {
            /* step 3, iterate to check  */
            vector<double> x(npms, 0);
            int i = 0;
            for (auto &pm : m_Pm)
                x[i++] = pm.xcor;
            vector<double> rs = L;
            CMat::CMat_Matmul("NN", nb_epoch, 1, npms, 1.0, A.data(), maxobs, x.data(), npms, -1, rs.data(), maxobs); /* AX - L */
            /* check the residuals */
            bcont = _residual_check(data, rs.data(), nb2idx, nb_epoch) && ++ite < 3;
            /* update sat2o, since isel is updated */
            _update_s2o(data, sat2no);
        }
    }
    free(nb2idx);
}
int polyModel::_residual_check(map<time_t, map<string, map<int, StecC>>> &data, double *r, int (*nb2idx)[4], int nb)
{
    int n_w_down, n_w_del;
    double k0 = 1.5, k1 = 3.0; /* igg limit */
    double esig_sum;
    CMat::CMat_Matmul("TN", 1, 1, nb, 1.0, r, nb, r, nb, 0.0, &esig_sum, 1);
    mesig = sqrt(esig_sum / nb);
    n_w_down = n_w_del = 0;
    char buff[1024] = {0};
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    for (int i = 0; i < nb; ++i)
    {
        double v = fabs(r[i]) / mesig;
        if (v < k0)
            continue;
        if (v < k1)
        {
            double scal = k0 / v * pow((k1 - v) / (k1 - k0), 2);
            time_t t = nb2idx[i][0] + mt0.time;
            string &s = sta_list[nb2idx[i][1]];
            int isat = nb2idx[i][2];
            data[t][s][isat].R = data[t][s][isat].R * sqrt(scal);
            n_w_down = n_w_down + 1;
        }
        else
        {
            double scal = 1e-4;
            time_t t = nb2idx[i][0] + mt0.time;
            string &s = sta_list[nb2idx[i][1]];
            int isat = nb2idx[i][2];
            data[t][s][isat].R = data[t][s][isat].R * sqrt(scal);

            data[t][s][isat].isel = 0;
            n_w_del = n_w_del + 1;
        }
    }
    time2str(mjd2time(dly->mjd, dly->sod), buff, 2);
    LOGPRINT("[%s ] %c %9.3lf (sig) %04d (down weight) %04d (del obs) %04d(nobs_all)", buff, m_csys, mesig, n_w_down, n_w_del, nb);
    return n_w_down + n_w_del;
}

vector<double> polyModel::_update_coeff(double dif_lat, double dif_lon, StecC &s, bool bunify)
{
    int i, j;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    char csys = dly->prn_alias[s.isat][0];
    vector<double> A(m_Pm.size(), 0);
    for (i = 0; i < (mN + 1); ++i)
    {
        for (j = 0; j < (mM + 1); ++j)
        {
            int imp = _idx_pm(s.isat, csys, " ", 1, i, j);
            A[imp] = std::pow(dif_lat, i) * std::pow(dif_lon, j) * (bunify ? s.R : 1.0); // 按照列构造
        }
    }
    int imp = _idx_pm(-1, csys, s.name, 2, -1, -1);
    A[imp] = 1 * (bunify ? s.R : 1.0);
    return A;
}
/* 形成观测方程 */
int polyModel::_form_A_matrix(map<time_t, map<string, t_key2v>> &data, double *A, int ldaA, double *L, int *sat2o, int (*nb2idx)[4], bool bunify)
{
    double posp[3] = {0};
    int i = 0, j = 0, iobs = 0, isat = 0, isit = 0, isys = -1;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    for (int ip = 0; ip < m_Pm.size(); ++ip)
        m_Pm[ip].iobs = 0;
    vector<string> site_exclude = {};
    for (auto &kv : data) // 时间
    {
        for (auto &sit2v : kv.second) // 测站
        {
            const string &sname = sit2v.first;
            if (bbo_contains_vector(site_exclude, sname))
            {
                for (auto &ss : sit2v.second)
                    ss.second.isel = 0;
                continue;
            }
            if (-1 == (isit = bbo_index_vector(sta_list, sit2v.first)))
            {
                for (auto &ss : sit2v.second)
                    ss.second.isel = 0;
                continue;
            }
            for (auto &sat2v : sit2v.second) // 卫星
            {
                isat = sat2v.first, isys = index_string(SYS, dly->prn_alias[sat2v.first][0]);
                // coefficientes
                if (sat2o[isat] < ((mN + 1) * (mM + 1) + redundancySite))
                {
                    sat2v.second.isel = 0; /* direct remove the observations */
                    continue;
                }
                if (sat2v.second.isel == 0) /* not selected */
                    continue;

                s2ipp(dly->mSta[sname].geod, sat2v.second.azim * DEG2RAD, sat2v.second.elev * DEG2RAD, EARTH_RADIUS, HION, posp); /* 生成穿刺点 */
                sat2v.second.ipp_lat = posp[0] * RAD2DEG;
                sat2v.second.ipp_lon = posp[1] * RAD2DEG;

                double dif_lat = (dly->mSta[sname].geod[0] * RAD2DEG - mlat0);
                double dif_lon = ((dly->mSta[sname].geod[1] * RAD2DEG - mlon0) + ((sat2v.second.t_r.time - mt0.time) / 3600.0 * 15));

                vector<double> ccf = _update_coeff(dif_lat, dif_lon, sat2v.second, bunify);
                for (int i = 0; i < ccf.size(); ++i)
                {
                    if (ccf[i] != 0.0)
                        m_Pm[i].iobs = m_Pm[i].iobs + 1;
                    A[ldaA * i + iobs] = ccf[i];
                }
                // update omc

                int imp = _idx_pm(-1, SYS[isys], sit2v.first, 2, -1, -1);
                L[iobs] = (sat2v.second.ionva - m_Pm[imp].xini) * (bunify ? sat2v.second.R : 1.0);
                nb2idx[iobs][0] = kv.first - mt0.time; /* time */
                nb2idx[iobs][1] = isit;                /* isit */
                nb2idx[iobs][2] = isat;                /* satellite */
                ++iobs;
            }
        }
    }
    // CMat::CMat_PrintMatrix_file(A, ldaA, iobs, m_Pm.size(), "A", "A_tempf");
    return iobs;
}

/* 获取基础配置 */
int polyModel::_get_estimated_list(map<time_t, map<string, t_key2v>> &data, int *sat2o)
{
    int nobs = 0, isat;
    if (sat2o)
        memset(sat2o, 0, sizeof(int) * MAXSAT);
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    map<int, list<string>> v2lsta;
    sta_list.clear(), sat_list.clear();
    for (auto &kv : data) // 时间
    {
        for (auto &sit2v : kv.second) // 测站
        {
            for (auto &sat2v : sit2v.second) // 卫星
            {
                isat = sat2v.first;
                string &cprn = dly->prn_alias[isat];
                if (sat2v.second.isel == 0) /* if it is deleted */
                    continue;
                if (!bbo_contains_vector(sta_list, sit2v.first))
                    sta_list.push_back(sit2v.first); // 添加测站//这里应该只有有数据的测站
                if (!bbo_contains_vector(sat_list, cprn))
                    sat_list.push_back(cprn);
                /* update the number of observed stations */
                if (v2lsta.find(isat) == v2lsta.end()) // 初始化该卫星的map类型数据
                    v2lsta[isat] = list<string>();
                if (!bbo_contains_list(v2lsta[isat], sit2v.first)) // 查询该测站观测到此颗卫星的情况是否被记录
                    v2lsta[isat].push_back(sit2v.first);           // 保存观测到此颗卫星不重复的测站
                ++nobs;
            }
        }
    }
    if (sat2o)
    {
        for (auto &kv : v2lsta)
            sat2o[kv.first] = kv.second.size(); // 存储对应卫星的观测值个数
    }

    return nobs;
}
void polyModel::_update_s2o(map<time_t, map<string, t_key2v>> &data, int *sat2o)
{
    map<int, list<string>> v2lsta;
    memset(sat2o, 0, sizeof(int) * MAXSAT);
    for (auto &kv : data) // 时间
    {
        for (auto &sit2v : kv.second) // 测站
        {
            for (auto &sat2v : sit2v.second) // 卫星
            {
                int isat = sat2v.first;
                if (sat2v.second.isel == 0)
                    continue;
                if (v2lsta.find(isat) == v2lsta.end()) // 初始化该卫星的map类型数据
                    v2lsta[isat] = list<string>();
                if (!bbo_contains_list(v2lsta[isat], sit2v.first)) // 查询该测站观测到此颗卫星的情况是否被记录
                    v2lsta[isat].push_back(sit2v.first);           // 保存观测到此颗卫星不重复的测站
            }
        }
    }
    for (auto &kv : v2lsta)
        sat2o[kv.first] = kv.second.size(); // 存储对应卫星的观测值个数
}

/* 更新参数 */
void polyModel::_update_estimated_conf(time_t tnow, map<time_t, map<string, t_key2v>> &data)
{
    int i = 0, j = 0;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    m_Pm.clear();
    // step 1, coefficients
    for (const auto &sat : sat_list) // 循环prn
    {
        for (i = 0; i < mN + 1; i++)
        {
            for (j = 0; j < mM + 1; j++)
            {
                int isat = pointer_string(dly->nprn, dly->prn_alias, sat);
                char csys = dly->prn_alias[isat][0];
                polypm pm = polypm(isat, csys, " ", 1, i, j);
                m_Pm.push_back(pm);
            }
        }
    }
    // step 2, DCB parameters
    for (auto &sta : sta_list)
    {
        for (i = 0; i < dly->nsys; i++)
        {
            polypm pm = polypm(-1, dly->system[i], sta, 2, -1, -1);
            m_Pm.push_back(pm);
        }
    }
    // 更新中央经纬度
    double all_lat = 0.0, all_lon = 0.0;
    for (const auto &kv : dly->mSta)
    {
        all_lat += kv.second.geod[0];
        all_lon += kv.second.geod[1];
    }
    mlat0 = (all_lat / (dly->mSta.size())) * RAD2DEG;
    mlon0 = (all_lon / (dly->mSta.size())) * RAD2DEG;

    // 更新参考时间
    gtime_t t0 = {0};
    // 更新参考时间,为当前时间
    t0.time = tnow;
    mt0 = t0;
}
/* 寻找参数位置 */
int polyModel::_idx_pm(int isat, char csys, string ssta, int tp, int i, int j)
{
    int nPs = (mN + 1) * (mM + 1);
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    if (tp == 1)
    { // 表示系数
        int isat_idx = -1;
        for (size_t idx = 0; idx < sat_list.size(); idx++)
        {
            int isat_ = pointer_string(dly->nprn, dly->prn_alias, sat_list[idx]); //
            if (isat_ == isat)
            {
                isat_idx = idx;
                break;
            }
        }
        if (isat_idx == -1)
            return -1;
        int idx_start = nPs * isat_idx;
        for (int s = 0; s < nPs; ++s)
        {
            if (m_Pm[idx_start + s].i == i && m_Pm[idx_start + s].j == j)
            {
                return idx_start + s;
            }
        }
        return -1;
    }
    else if (tp == 2)
    { // 表示 DCB 参数
        int nsat = sat_list.size(), ista = -1;
        if (-1 == (ista = bbo_index_vector(sta_list, ssta)))
            return -1;

        int idx_start = nsat * nPs + ista * dly->nsys;
        for (int s = 0; s < dly->nsys; ++s)
        {
            if (m_Pm[idx_start + s].sname == sta_list[ista] && m_Pm[idx_start + s].csys == csys)
            {
                return idx_start + s;
            }
        }
        return -1;
    }
    else
    {
        return -1;
    }
}

bool polyModel::_add_center_constraint(double *AtA, int lda, char csys, bool b_check_xini)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    // 遍历 PMS
    int pms = m_Pm.size(), bcon = false;
    int isys = index_string(SYS, csys), ncf = 0;
    double *ccf = (double *)calloc(pms, sizeof(double));
    for (size_t ip = 0; ip < m_Pm.size(); ++ip)
    {
        if (m_Pm[ip].tp != 2 || m_Pm[ip].csys != csys || m_Pm[ip].iobs == 0)
            continue;
        if (b_check_xini && m_Pm[ip].xini == 0.0) /* no DCB init values */
            continue;
        ccf[ip] = 1.0 * 1e3;
        ++ncf;
    }
    if (ncf > 0)
    {
        bcon = true;
        CMat::CMat_Matmul("TN", pms, pms, 1, 1.0, ccf, 1, ccf, 1, 1.0, AtA, lda);
    }
    free(ccf);
    return bcon;
}
/* 加入DCB基准 */
void polyModel::_add_single_constraint(double *AtA, int lda, const char csys)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    int *max_sta_sat = new int[dly->nsys];
    int *max_sta_num = new int[dly->nsys];
    int npms = m_Pm.size();
    for (int i = 0; i < dly->nsys; i++)
    {
        max_sta_sat[i] = -1;
        max_sta_num[i] = -1;
    }
    // 遍历 PMS
    for (size_t ip = 0; ip < m_Pm.size(); ip++)
    {
        if (m_Pm[ip].tp == 2)
        {
            int sys_index = index_string(SYS, m_Pm[ip].csys);
            int iobs = m_Pm[ip].iobs;
            if (iobs > max_sta_num[sys_index])
            {
                max_sta_num[sys_index] = iobs;
                max_sta_sat[sys_index] = static_cast<int>(ip);
            }
        }
    }
    // 对每个系统加入基准偏差约束
    for (int i = 0; i < dly->nsys; i++)
    {
        if (max_sta_num[i] > 0 && dly->system[i] == csys)
        {
            AtA[max_sta_sat[i] * lda + max_sta_sat[i]] += 1e5;
        }
    }
    delete[] max_sta_sat;
    delete[] max_sta_num;
}

void polyModel::_assign_esigma(map<string, map<int, StecC>> &res, map<string, polyCoefficient> &coef)
{
    map<string, double> esig;
    map<string, int> n_esig;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    for (auto &sit2v : res) // 测站
    {
        // const string &sname = sit2v.first;
        for (auto &sat2v : sit2v.second)
        {
            int isat = sat2v.first;

            double v = sat2v.second.ionva;

            if (esig.find(dly->prn_alias[isat]) == esig.end())
                esig[dly->prn_alias[isat]] = 0.0;
            esig[dly->prn_alias[isat]] += v * v; /* sat2v.second.R has already be multiply into A, L and Residual */

            if (n_esig.find(dly->prn_alias[isat]) == n_esig.end())
                n_esig[dly->prn_alias[isat]] = 0;
            n_esig[dly->prn_alias[isat]] += 1;
        }
    }

    for (auto &kv : esig)
    {
        if (coef.count(kv.first) != 0)
        {
            coef[kv.first].esig = sqrt(kv.second / n_esig[kv.first]);
        }
    }
    return;
}

map<string, polyCoefficient> polyModel::_get_estimated_coeff()
{
    int para_num = (mN + 1) * (mM + 1);
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    std::map<std::string, polyCoefficient> coef; // 存储每个卫星的系数
    for (int isat = 0; isat < dly->nprn; ++isat)
    {
        int idx = _idx_pm(isat, dly->prn_alias[isat][0], "", 1, 0, 0);
        if (idx == -1 || m_Pm[idx].iobs == 0)
            continue;

        polyCoefficient coefi;
        coefi.m = mM;
        coefi.n = mN;
        coefi.lon0 = mlon0;
        coefi.lat0 = mlat0;
        coefi.t0 = mt0.time;
        for (int i = 0; i < para_num; ++i)
            coefi.coef.push_back(m_Pm[idx + i].xcor);

        coef[dly->prn_alias[isat]] = coefi;
    }
    return coef;
}

map<string, map<int, StecC>> polyModel::_get_residual(time_t time, map<std::string, polyCoefficient> &coef, map<string, map<int, StecC>> &data)
{
    int i, j;
    map<string, map<int, StecC>> res;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    for (auto &sit2v : data) // 测站
    {
        const string &sname = sit2v.first;
        for (auto &sat2v : sit2v.second)
        {
            int isat = sat2v.first;
            if (coef.count(dly->prn_alias[isat]) == 0)
                continue;

            if (time != 0 && m_data.count(time) != 0 && m_data[time][sname][isat].isel == 0) /* deleted during model */
                continue;

            double v = sat2v.second.ionva;
            polyCoefficient &c = coef[dly->prn_alias[isat]];
            double v_model = c.m_getModelV(sat2v.second.t_r.time, dly->mSta[sname].geod[0] * RAD2DEG, dly->mSta[sname].geod[1] * RAD2DEG);
            v = v - v_model;

            int imp = _idx_pm(-1, dly->prn_alias[isat][0], sit2v.first, 2, -1, -1);
            v = v - m_Pm[imp].xest;
            if (res.count(sname) == 0)
                res[sname] = map<int, StecC>();
            res[sname][isat] = sat2v.second;
            res[sname][isat].ionva = v;
        }
    }
    return res;
}

void polyModel::_output(int mjd, double sod, map<string, map<int, StecC>> &data)
{
    int y, d;
    char pname[1024] = {0};
    d = mjd2doy(mjd, &y);
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    string ttag = "POLY";
    if (KvContainer::getv(string(ttag)) != toString(d))
    {
        string f = KvContainer::getv(m_tag_coef_fil);
        Logtrace::s_defaultlogger.m_closeLog(f);

        sprintf(pname, "%s/coef_%04d%03d", dly->outdir, y, d);
        Logtrace::s_defaultlogger.m_openLog(pname);
        KvContainer::setv(string(m_tag_coef_fil), pname);

        if (strstr(dly->m_addargs, "-outres"))
        {
            f = KvContainer::getv(m_tag_res_fil);
            Logtrace::s_defaultlogger.m_closeLog(f);
            sprintf(pname, "%s/resi_%04d%03d", dly->outdir, y, d);
            Logtrace::s_defaultlogger.m_openLog(pname);
            KvContainer::setv(string(m_tag_res_fil), pname);
        }

        KvContainer::setv(ttag, toString(d));
    }

    map<string, map<int, StecC>> res_v = _get_residual(mjd2time(mjd, sod).time, m_coef, data);
    _assign_esigma(res_v, m_coef);

    /* output residuals */
    string f = KvContainer::getv(m_tag_res_fil);
    if (f != "" && Logtrace::s_defaultlogger.m_lexist(f))
    {
        map<string, map<int, StecC>> res = _get_residual(0, m_coef, data);
        _output_residual(mjd, sod, f, res);
    }
}
void polyModel::_output_coeff(int mjd, double sod, string f, std::map<std::string, polyCoefficient> &coef)
{

    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    if (f != "" && Logtrace::s_defaultlogger.m_lexist(f))
    {
        time_t t0;
        double lat0;
        double lon0;
        int N = 0, M = 0;
        for (auto &kv : coef)
        {
            N = kv.second.n;
            M = kv.second.m;
            lat0 = kv.second.lat0;
            lon0 = kv.second.lon0;
            t0 = kv.second.t0;
        }
        char tbuf[1024] = {0};
        int nums = coef.size();
        int para_num = (N + 1) * (M + 1);
        time2str(mjd2time(mjd, sod), tbuf, 2);

        Logtrace::s_defaultlogger.m_wtMsg("@%s > %s %13.9lf %13.9lf %ld %d %d %d # (lat0,lon0,t0,N,M,NSAT (TEC))\n", f.c_str(), tbuf, lat0, lon0, t0,
                                          N, M, coef.size());
        for (auto &kv : coef)
        {
            Logtrace::s_defaultlogger.m_wtMsg("@%s %s", f.c_str(), kv.first.c_str());
            for (int i = 0; i < para_num; ++i)
                Logtrace::s_defaultlogger.m_wtMsg("@%s %20.9lf ", f.c_str(), kv.second.coef[i]);
            Logtrace::s_defaultlogger.m_wtMsg("@%s %4.2lf ", f.c_str(), kv.second.esig);
            Logtrace::s_defaultlogger.m_wtMsg("@%s \n", f.c_str());
        }
    }
}
void polyModel::_output_residual(int mjd, double sod, string f, map<string, map<int, StecC>> &res)
{
    int i, j;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    for (auto &sit2v : res) // 测站
    {
        const string &sname = sit2v.first;
        for (auto &sat2v : sit2v.second)
        {
            int isat = sat2v.first;
            Logtrace::s_defaultlogger.m_wtMsg("@%s %5s %4s %3s  %5d %9.1lf %9.4lf %9.4lf %13.9lf %13.9lf %12.3lf %12.3lf %10d\n", f.c_str(), "ION", sname.c_str(),
                                              dly->cprn[isat].c_str(), mjd, sod, sat2v.second.elev, sat2v.second.azim, sat2v.second.ipp_lat, sat2v.second.ipp_lon,
                                              sat2v.second.ionva, 0.3, AMB_FIX);
        }
    }
}
