/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-30 15:00:44
 * @ Modified by: Jun Tao
 * @ Modified time: 2025-01-06 10:49:22
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include "include/Frame/Frame.h"
#include "include/Applications/Applications.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace iono;
struct SiteInfo
{
    std::string name; // 测站名
    double dist;      // 与目标测站的距离（单位：米）
    double dx[3];     // 可选：三维坐标差 (dx, dy, dz)

    SiteInfo(const std::string &n, double d, const double diff[3])
        : name(n), dist(d)
    {
        dx[0] = diff[0];
        dx[1] = diff[1];
        dx[2] = diff[2];
    }

    SiteInfo(const std::string &n, double d)
        : name(n), dist(d)
    {
        dx[0] = dx[1] = dx[2] = 0.0;
    }

    SiteInfo() : name(""), dist(0.0)
    {
        dx[0] = dx[1] = dx[2] = 0.0;
    }
};
static map<string, map<int, StecC>> s_getCommSite(map<string, vector<double>> &s2coor, map<string, map<int, StecC>> &data, const Station &req)
{
    std::vector<SiteInfo> sites;
    sites.reserve(s2coor.size());

    // 计算距离
    for (auto &[name, x] : s2coor)
    {
        double diff[3] = {x[0] - req.x[0], x[1] - req.x[1], x[2] - req.x[2]};
        double dist = std::sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);

        if (dist / 1000.0 < 200.0 && data.count(name))
            sites.emplace_back(name, dist, diff);
    }

    // 排序
    std::sort(sites.begin(), sites.end(),
              [](const SiteInfo &a, const SiteInfo &b)
              {
                  return a.dist < b.dist;
              });

    // 限制数量
    const size_t MAX_NEARBY = 30;
    if (sites.size() > MAX_NEARBY)
        sites.resize(MAX_NEARBY);

    // 生成结果
    map<string, map<int, StecC>> comm;
    for (auto &s : sites)
    {
        // LOGPRINT("%s : %s %9.3lf", req.name.c_str(), s.name.c_str(), s.dist / 1000.0);
        comm[s.name] = data.at(s.name);
    }

    return comm;
}

Interp_manager::Interp_manager() : Callable(this)
{
    m_moduleid = m_url_send = "";
    unsigned int nthreads = std::thread::hardware_concurrency();
    if (nthreads == 0)
        nthreads = 4; // fallback
    _pool = new ThreadPool(4);
}
Interp_manager::~Interp_manager()
{
}
void Interp_manager::_set_requests(vector<Station> &in)
{
    function chk_diff = [](list<string> s1, list<string> s2) -> list<string> /* items in s1 that not in s2 */
    {
        list<string> s_dif;
        for (auto &il : s1)
        {
            bool b_exist = false;
            for (auto &jl : s2)
            {
                if ((b_exist = (il == jl)))
                    break;
            }
            if (!b_exist)
                s_dif.push_back(il);
        }
        return s_dif;
    };
    list<string> keys_n, keys_o;
    for (auto &kv : mReqs)
        keys_o.push_back(kv.name);
    for (auto &kv : in)
        keys_n.push_back(kv.name);
    list<string> rx_adds_ = chk_diff(keys_n, keys_o);
    list<string> rx_dels_ = chk_diff(keys_o, keys_n);

    for (auto &v : rx_adds_)
    {
        rq2krig[v] = new Interp_kriging_smp();
        rq2krig[v]->m_set_pool(_pool);
    }
    for (auto &v : rx_dels_)
    {
        if (rq2krig.find(v) != rq2krig.end())
        {
            delete rq2krig[v];
            rq2krig.erase(v);
        }
    }
    mReqs = in;
}
static map<string, vector<double>> s_getStationC(map<string, map<int, StecC>> &data)
{
    map<string, vector<double>> s2coor;
    for (auto &[_, v] : data)
    {
        for (auto &[__, v_sat] : v)
        {
            if (s2coor.count(_) == 0)
                s2coor[_] = vector<double>(3, 0);
            memcpy(s2coor[_].data(), v_sat.x, sizeof(double) * 3);
        }
    }
    return s2coor;
}

map<string, map<int, StecC>> Interp_manager::_get_residual(map<std::string, polyCoefficient> &coef, map<string, map<int, StecC>> &data)
{
    int i, j;
    map<string, map<int, StecC>> res;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    for (auto &sit2v : data) // 测站
    {
        const string &sname = sit2v.first;
        double geod[3] = {0};
        for (auto &sat2v : sit2v.second)
        {
            int isat = sat2v.first;
            if (coef.count(dly->prn_alias[isat]) == 0)
                continue;
            polyCoefficient &c = coef.at(dly->prn_alias[isat]);
            if (!sit2v.second.count(c.irf))
                continue;

            if (geod[0] * geod[1] == 0.0)
                xyzblh(sat2v.second.x, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, geod);

            double v = sat2v.second.ionva - sit2v.second[c.irf].ionva;
            double v_model = c.m_getModelV(sat2v.second.t_r.time, geod[0] * RAD2DEG, geod[1] * RAD2DEG);
            v = v - v_model;

            if (res.count(sname) == 0)
                res[sname] = map<int, StecC>();
            res[sname][isat] = sat2v.second;
            res[sname][isat].ionva = v;
            res[sname][isat].ionvm = v_model;
            res[sname][c.irf] = sit2v.second[c.irf];
            res[sname][c.irf].ionva = 0.001;
            res[sname][c.irf].ionvm = 0.001;

            memcpy(res[sname][isat].x, sat2v.second.x, sizeof(double) * 3);
            memcpy(res[sname][c.irf].x, sat2v.second.x, sizeof(double) * 3);
        }
    }
    return res;
}
static vector<int> s_get_refbycoef(map<string, polyCoefficient> &coeff)
{
    vector<int> ref(MAXSYS, -1);
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    for (auto &kv : coeff)
    {
        int isys = index_string(SYS, dly->prn_alias[kv.second.isat][0]);
        if (ref[isys] == -1)
            ref[isys] = kv.second.irf;
    }
    return ref;
}
/// @brief 统一坐标到平面坐标系
/// @param lon0 中央经度，单位为度
/// @param data 非差电离层
void Interp_manager::_kriging_update_xy(map<string, vector<double>> &s2coor, double lon0, map<string, map<int, StecC>> &data, Station &sta)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    for (auto &s2v : data)
    {
        double geod[3] = {0}, gausx[3] = {0};
        xyzblh(s2coor[s2v.first].data(), 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, geod);
        bl2Gaussxy(geod[0], geod[1], gausx, gausx + 1, lon0 * D2R, ESQUARE, EARTH_A, 0.0);
        gausx[2] = geod[2];
        for (auto &sat2v : s2v.second)
            memcpy(sat2v.second.gausx, gausx, sizeof(double) * 3);
    }
    bl2Gaussxy(sta.geod[0], sta.geod[1], sta.gausx, sta.gausx + 1, lon0 * D2R, ESQUARE, EARTH_A, 0.0);
}
void Interp_manager::_interp(int mjd, double sod, bool buse_residual, map<string, map<int, StecC>> &data, map<string, map<int, StecC>> &realv)
{
    map<string, t_gridV> s2grid;
    map<string, vector<double>> s2cor = s_getStationC(data);
    vector<int> ref;
    map<string, map<int, StecC>> input;
    if (buse_residual)
    {
        /* using the coefficient to compute residual */
        input = _get_residual(m_coef, data);
        /* update ref */
        ref = s_get_refbycoef(m_coef);
    }
    else
        input = data;
    for (auto &req : mReqs)
    {
        map<string, map<int, StecC>> comm = s_getCommSite(s2cor, input, req);
        double lon0 = NINT(req.geod[1] * R2D / 3) * 3;
        _kriging_update_xy(s2cor, lon0, comm, req);
        LOGPRINT_EX("%s : %d (nsit)", req.name.c_str(), comm.size());
        if (rq2krig.find(req.name) != rq2krig.end())
        {
            rq2krig[req.name]->_interp_kriging(mjd, sod, ref.empty() ? NULL : ref.data(), {req}, comm);
            s2grid[req.name] = rq2krig[req.name]->mGridv[req.name];
        }
    }
    /* merge all results */
    _output(mjd, sod, s2grid, mReqs, realv);
}

map<string, pair<double, double>> Interp_manager::_get_gridv(int mjd, double sod, int *refsat, map<int, t_Sunit> &sat2v, Station &grid)
{
    int idx = -1, ref_o[MAXSYS] = {0};
    time_t t = mjd2time(mjd, sod).time;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    memset(ref_o, 0, sizeof(ref_o));
    map<string, pair<double, double>> m_r;
    for (auto &kv : sat2v)
    {
        int isat = kv.first, isys = index_string(SYS, dly->prn_alias[isat][0]);
        /* compute the surface value */
        double surf = 0.0, surf_r = 0.0;
        string &cprn = dly->prn_alias[isat], &cprn_r = dly->prn_alias[refsat[isys]];
        if (m_coef.count(cprn) && m_coef.count(cprn_r))
        {
            surf = m_coef[cprn].m_getModelV(t, grid.geod[0] * RAD2DEG, grid.geod[1] * RAD2DEG);
            surf_r = m_coef[cprn_r].m_getModelV(t, grid.geod[0] * RAD2DEG, grid.geod[1] * RAD2DEG);
        }
        m_r[dly->prn_alias[isat]] = make_pair(kv.second.v + surf - surf_r, kv.second.vsig);
        if (!ref_o[isys] && refsat[isys] != -1)
        {
            ref_o[isys] = true;
            m_r[dly->prn_alias[refsat[isys]]] = make_pair(0.0, 0.001);
        }
    }

    return m_r;
}
void Interp_manager::_output(int mjd, double sod, map<string, t_gridV> &grid_v, vector<Station> &grids, map<string, map<int, StecC>> &realv)
{
    int y, d;
    char pname[1024] = {0};
    d = mjd2doy(mjd, &y);
    string f;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    string ttag = "INTERP", tag_grid_fil = "GRIDFILE", tag_precision_fil = "PRECISION_FILE";
    string tag_grid_withrsig = "GRIDFILE_RSIG", tag_coef_withrsig = "COEFFILE_RSIG";
    if (KvContainer::getv(string(ttag)) != toString(d))
    {
        if (strstr(dly->m_addargs, "-outgrid"))
        {
            f = KvContainer::getv(tag_grid_fil);
            Logtrace::s_defaultlogger.m_closeLog(f);
            sprintf(pname, "%s/grid_%04d%03d", dly->outdir, y, d);
            Logtrace::s_defaultlogger.m_openLog(pname);
            KvContainer::setv(string(tag_grid_fil), pname);
        }

        KvContainer::setv(ttag, toString(d));
    }
    /* output residuals */
    f = KvContainer::getv(tag_grid_fil);
    if (f != "" && Logtrace::s_defaultlogger.m_lexist(f))
    {
        _output_grids(mjd, sod, f, grid_v, grids, realv);
    }

    {
        _output_redis(mjd, sod, grid_v, grids);
    }
}
void Interp_manager::_output_grids(int mjd, double sod, string f, map<string, t_gridV> &grid_v, vector<Station> &grids, map<string, map<int, StecC>> &reald)
{
    function _get_sta_idx = [](vector<Station> &grids, const string &sta) -> int
    {
        for (size_t i = 0; i < grids.size(); ++i)
            if (sta == grids[i].name)
                return i;
        return -1;
    };

    int i, ref_o[MAXSYS] = {0}, idx;
    time_t t = mjd2time(mjd, sod).time;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();

    if (t % dly->t_ogrid_intv != 0)
        return;
    for (auto &[s, sit2v] : grid_v)
    {
        const string &sname = s;
        memset(ref_o, 0, sizeof(ref_o));
        if (-1 == (idx = _get_sta_idx(grids, sname)))
            continue;
        for (auto &sat2v : sit2v.gridv)
        {
            int isat = sat2v.first, isys = index_string(SYS, dly->prn_alias[isat][0]), irf = sit2v.refsat[isys];
            /* compute the surface value */
            double surf = 0.0, surf_r = 0.0;
            string &cprn = dly->prn_alias[isat], &cprn_r = dly->prn_alias[irf];
            if (m_coef.count(cprn))
                surf = m_coef[cprn].m_getModelV(t, grids[idx].geod[0] * RAD2DEG, grids[idx].geod[1] * RAD2DEG);

            double real_dif = 999;
            double elev = 0.0, azim = 0.0, relev = 0.0, razim = 0.0;
            if (reald.count(sname) && reald[sname].count(isat) && reald[sname].count(irf))
            {
                double rdata = reald[sname][isat].ionva, rdata_ref = reald[sname][irf].ionva;
                real_dif = rdata - rdata_ref - sat2v.second.v - surf;

                elev = reald[sname].at(isat).elev;
                azim = reald[sname].at(isat).azim;

                relev = reald[sname].at(irf).elev;
                razim = reald[sname].at(irf).azim;
            }

            Logtrace::s_defaultlogger.m_wtMsg("@%s %5s %4s %3s  %5d %9.1lf %9.4lf %9.4lf %12.3lf %12.3lf %10d %12.3lf %12.3lf %03d %9.3lf\n", f.c_str(), "ION", sname.c_str(),
                                              dly->cprn[isat].c_str(), mjd, sod, elev, azim,
                                              sat2v.second.v + surf, sat2v.second.vsign, AMB_FIX, sat2v.second.vsig, real_dif, sat2v.second.nobs, sat2v.second.aveDist);
            if (!ref_o[isys] && irf != -1)
            {
                ref_o[isys] = true;
                Logtrace::s_defaultlogger.m_wtMsg("@%s %5s %4s %3s  %5d %9.1lf %9.4lf %9.4lf %12.3lf %12.3lf %10d %12.3lf %12.3lf %03d %9.3lf\n", f.c_str(), "ION", sname.c_str(),
                                                  dly->cprn[irf].c_str(), mjd, sod, relev, razim,
                                                  0.0, 0.001, AMB_FIX, 0.001, 0.001, sat2v.second.nobs, sat2v.second.aveDist);
            }
        }
    }
}

/// @brief output coeff/grid information into redis
/// @param mjd
/// @param sod
/// @param grid_v
/// @param grids
void Interp_manager::_output_redis(int mjd, double sod, map<string, t_gridV> &grid_v, vector<Station> &grids)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    function _get_sta_idx = [](vector<Station> &grids, const string &sta) -> int
    {
        for (size_t i = 0; i < grids.size(); ++i)
            if (sta == grids[i].name)
                return i;
        return -1;
    };
    if (m_url_send != "" && !m_sender.b_ison())
    {
        char buff[1024];
        char command[2048];
        char addr[256] = {0}, port_str[256] = {0};
        decodetcppath(m_url_send.c_str(), addr, port_str, NULL, NULL, NULL, NULL);
        // 找到最后一个 '/'
        size_t pos = m_url_send.find_last_of('/');
        std::string mnt = (pos == std::string::npos) ? m_url_send : m_url_send.substr(pos + 1);

        m_sender.m_setRedisConfigure(addr, atoi(port_str), "");
        m_sender.m_StartService();
    }
    size_t pos = m_url_send.find_last_of('/');
    std::string mnt = (pos == std::string::npos) ? m_url_send : m_url_send.substr(pos + 1);
    for (auto &kv : grid_v)
    {
        int idx = -1;
        Json::Value root;
        char buff[1024] = {0};
        vector<int> irf(MAXSYS, -1), maxobs(MAXSYS, 0);
        if (-1 == (idx = _get_sta_idx(grids, kv.first)))
            continue;
        for (auto &[isat, data] : kv.second.gridv)
        {
            Json::Value sate;
            double surf = 0.0;
            string &cprn = dly->prn_alias[isat];
            if (m_coef.count(cprn))
                surf = m_coef[cprn].m_getModelV(mjd2time(mjd, sod).time, grids[idx].geod[0] * RAD2DEG, grids[idx].geod[1] * RAD2DEG);

            int isys = index_string(SYS, dly->prn_alias[isat][0]);
            irf[isys] = kv.second.refsat[isys];
            sate["cprn"] = dly->cprn[isat];
            sate["ion"] = toString(data.v + surf);
            sate["sig"] = toString(data.vsign);
            sate["nobs"] = toString(data.nobs);

            if (data.nobs > maxobs[isys])
                maxobs[isys] = data.nobs;
            root["satellite"].append(sate);
        }

        for (int isys = 0; isys < MAXSYS; ++isys)
        {
            if (irf[isys] == -1)
                continue;
            Json::Value sate;
            sate["cprn"] = dly->cprn[irf[isys]];
            sate["ion"] = toString(0.0);
            sate["sig"] = toString(0.001);
            sate["nobs"] = toString(maxobs[isys]);
            root["satellite"].append(sate);
        }
        if (root["satellite"].size() > 0)
        {
            root["time"] = run_timefmt(mjd, sod, buff);
            Package pack_am;
            pack_am.expire = -1;
            pack_am.type = "SET";
            pack_am.channel = mnt + ":ion:" + kv.first;
            pack_am.ptime = mjd2time(mjd, sod).time;
            string str = zipJson(root.toStyledString());
            pack_am.buff = (char *)calloc(sizeof(char), str.size());
            memcpy(pack_am.buff, str.c_str(), sizeof(char) * str.size());
            pack_am.nbyte = str.size();
            m_sender.m_addPackage(pack_am, "grid_ion");
        }
    }
    /* output coefficient */
    time_t t0;
    double lat0;
    double lon0;
    int N = 0, M = 0;
    for (auto &kv : m_coef)
    {
        N = kv.second.n;
        M = kv.second.m;
        lat0 = kv.second.lat0;
        lon0 = kv.second.lon0;
        t0 = kv.second.t0;
    }
    int nums = m_coef.size();
    int para_num = (N + 1) * (M + 1);

    if (nums > 0 && m_moduleid != "")
    {
        Json::Value root;
        vector<int> irf(MAXSYS, -1);
        char strv[1024] = {0};
        for (auto &kv : m_coef)
        {
            Json::Value sate;
            char buff[4096] = {0};
            int isys = index_string(SYS, dly->prn_alias[kv.second.isat][0]);
            irf[isys] = kv.second.irf;

            for (int i = 0; i < para_num; ++i)
                sprintf(buff + strlen(buff), " %15.10lf ", kv.second.coef[i]);
            sate["coef"] = toString(buff);
            sate["cprn"] = dly->cprn[kv.second.isat];
            sate["sig-f"] = toString(kv.second.esig);
            sate["sig-c"] = toString(kv.second.esig_cv);
            sate["nobs"] = toString(kv.second.nobs);

            root["satellite"].append(sate);
        }
        for (int isys = 0; isys < MAXSYS; ++isys)
        {
            if (irf[isys] == -1)
                continue;
            Json::Value sate;
            char buff[4096] = {0};
            for (int i = 0; i < para_num; ++i)
                sprintf(buff + strlen(buff), " %20.9lf ", 0.0);
            sate["coef"] = toString(buff);
            sate["cprn"] = dly->cprn[irf[isys]];

            sate["sig-f"] = toString(0.001);
            sate["sig-c"] = toString(0.001);
            root["satellite"].append(sate);
        }
        root["N"] = toString(N);
        root["M"] = toString(M);
        sprintf(strv, "%20.9lf", lat0);
        root["lat0"] = toString(strv);
        sprintf(strv, "%20.9lf", lon0);
        root["lon0"] = toString(strv);
        sprintf(strv, "%ld", t0);
        root["t0"] = toString(strv);

        root["time"] = run_timefmt(mjd, sod, strv);
        Package pack_am;
        pack_am.expire = -1;
        pack_am.type = "SET";
        pack_am.channel = mnt + ":" + m_moduleid;
        pack_am.ptime = mjd2time(mjd, sod).time;
        string str = root.toStyledString();
        pack_am.buff = (char *)calloc(sizeof(char), str.size());
        memcpy(pack_am.buff, str.c_str(), sizeof(char) * str.size());
        pack_am.nbyte = str.size();
        m_sender.m_addPackage(pack_am, "POLY_ion");
    }
}
