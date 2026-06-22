/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-28 20:50:53
 * @ Modified by: Jun Tao
 * @ Modified time: 2024-08-24 17:12:49
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */

#include <cmath>
#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace std;
using namespace iono;
/* input functions */

void UDAtomAdapter::m_inputIono(StecC &stec)
{
    /* step 1, update time */
    using t_keysit = map<int, StecC>;
    time_t t_now = stec.t_r.time;
    bool b_update = 0;
    map<time_t, map<string, t_keysit>>::iterator itr;
    {
        std::lock_guard<std::mutex> lk(mltx);
        /* step 1, erase out of date record */
        for (itr = m_iondata.begin(); itr != m_iondata.end();)
        {
            if (t_now - itr->first > 150)
            {
                itr = m_iondata.erase(itr);
                continue;
            }
            ++itr;
        }
        /* step 2, add the corresponding data */
        if (m_iondata.find(t_now) == m_iondata.end())
            m_iondata[t_now] = map<string, t_keysit>();
        map<string, t_keysit> &sit2v = m_iondata[t_now];
        if (sit2v.find(stec.name) == sit2v.end())
            sit2v[stec.name] = t_keysit();
        t_keysit &sat2v = sit2v[stec.name];
        /* ****** mount the data */
        sat2v[stec.isat] = stec;
        /******** update time tag */
        if (t_lastrcv != t_now)
        {
            t_lastrcv = t_now;
            b_update = 1;
            // LOGPRINT("receving ionosphere time: %d %d(delay)", t_now, t_compuater.time - t_now);
        }
    }
    if (b_update)
        mcv.notify_all();
}

void UDAtomAdapter::m_inputIono(map<int, StecC> &sat2s)
{
    /* step 1, update time */
    using t_keysit = map<int, StecC>;
    time_t t_now = sat2s.begin()->second.t_r.time;
    string name = sat2s.begin()->second.name;
    bool b_update = 0;
    map<time_t, map<string, t_keysit>>::iterator itr;
    {
        std::lock_guard<std::mutex> lk(mltx);
        /* step 1, erase out of date record */
        for (itr = m_iondata.begin(); itr != m_iondata.end();)
        {
            if (t_now - itr->first > 150)
            {
                itr = m_iondata.erase(itr);
                continue;
            }
            ++itr;
        }
        /* step 2, add the corresponding data */
        if (m_iondata.find(t_now) == m_iondata.end())
            m_iondata[t_now] = map<string, t_keysit>();
        map<string, t_keysit> &sit2v = m_iondata[t_now];
        sit2v[name] = sat2s;
        /******** update time tag */
        if (t_lastrcv != t_now)
        {
            t_lastrcv = t_now;
            b_update = 1;
            // gtime_t t_compuater = utc2gpst(timeget());
            // LOGPRINT("receving ionosphere time: %d %d(delay)", t_now, t_compuater.time - t_now);
        }
    }
    if (b_update)
        mcv.notify_all();
}

/* 实时条件下，最新的time tag可能没有收全 */
map<string, map<int, StecC>> UDAtomAdapter::m_inquireIono(int &mjd, double &sod, double span)
{
    time_t t_sel = -1;
    int dif_t = 1e6, i;
    map<string, map<int, StecC>> v_r;
    time_t t_req = mjd2time(mjd, sod).time;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    if (dly->lpost)
    {
        std::lock_guard<std::mutex> lk(mltx);
        for (auto &kv : m_iondata)
        {
            if (fabs(t_req - kv.first) <= dif_t)
            {
                dif_t = fabs(t_req - kv.first);
                t_sel = kv.first;
            }
        }
        if (t_sel != -1 && dif_t < span)
            return m_iondata[t_sel];
    }
    else
    {
        int delay = 2;
        while (true)
        {
            std::unique_lock<std::mutex> lk(mltx);
            time_t t_reach = round(t_lastsel / dly->dintv) * dly->dintv + dly->dintv + delay;
            if (t_lastrcv < t_reach || m_iondata.size() < 2)
            {
                auto end_time = std::chrono::system_clock::now() + std::chrono::seconds(NINT(dly->dintv + 1));
                this->mcv.wait_until(lk, end_time, [&]()
                                     { return t_lastrcv >= t_reach && m_iondata.size() >= 2; });
                if (t_lastrcv < t_reach || m_iondata.size() < 2)
                    break;
            }
            vector<time_t> t_tag;
            for (auto &kv : m_iondata)
                t_tag.push_back(kv.first);
            std::sort(t_tag.begin(), t_tag.end());
            t_sel = -1;
            for (i = 0; i < t_tag.size(); ++i)
            {
                double s = time2mjd(t_tag[i], NULL);
                if (fmod(s, dly->dintv) == 0.0 && t_tag[i] > t_req && i != 0)
                {
                    t_sel = t_tag[i];
                    break;
                }
            }
            LOGPRINT("%d(t_lastrcv) %d(t_reach) %d(t_req) %d(t_sel)", t_lastrcv, t_reach, t_req, t_sel);
            if (t_sel == -1)
                continue;
            t_lastsel = t_sel;
            {
                /* update t_selected */
                gtime_t gt_sel = {0};
                gt_sel.time = t_lastsel;
                sod = time2mjd(gt_sel, &mjd);
            }
            return m_iondata[t_lastsel];
        }
    }
    return v_r;
}