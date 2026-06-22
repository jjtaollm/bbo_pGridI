//
// Created by juntao, at wuhan university   on 2020/11/2.
//

#ifndef BAMBOO_BRDORBITCLKADAPTER_H
#define BAMBOO_BRDORBITCLKADAPTER_H
#include "BroadcastEphUtils.h"
#include "include/Frame/Gnss/GnssInput/Orbclk/OrbitClkAdapter.h"
#include <mutex>
namespace iono
{
    class BrdOrbitClkAdapter : public OrbitClkAdapter, public BroadcastEphUtils
    {
    public:
        BrdOrbitClkAdapter();
        virtual int v_readOrbit(const char *cprn, int mjd, double sod, double *xsat);
        virtual int v_readClk(const char *cprn, int mjd, double sod, double *sclk);
        // for real-time oc
        virtual int v_readOrbit(const char *cprn, int mjd, double sod, double *xsat, int *iode);
        virtual int v_readClk(const char *cprn, int mjd, double sod, double *sclk, int *iode);

        // for clock drift
        virtual int v_readClkDrift(const char *cprn, int mjd, double sod, double *vclk);
        virtual map<string, GPSEPH> v_getCurrentGpsEph(double ptime);
        virtual map<string, GLSEPH> v_getCurrentGlsEph(double ptime);

        /* 这里可以设置，只返回最近的一个星历 */
        map<string, list<GPSEPH>> m_getGnssEphemeris()
        {
            function compare = [](GPSEPH &a, GPSEPH &b) -> bool
            {
                return (a.mjd - b.mjd) * 86400.0 + a.sod - b.sod < 0;
            };
            std::unique_lock<std::mutex> lk(q_mutex);
            /* get the newest ephemeris */
            map<string, list<GPSEPH>> m_r;
            for (auto &kv : gnssEph)
            {
                m_r[kv.first] = list<GPSEPH>();
                if (!kv.second.empty())
                {
                    auto itr = std::max_element(kv.second.begin(), kv.second.end(), compare);
                    if (itr != kv.second.end())
                    {
                        m_r[kv.first].emplace_back(*itr);
                    }
                }
            }
            return m_r;
        }
        map<string, list<GLSEPH>> m_getGlsEphemeris()
        {
            function compare = [](GLSEPH &a, GLSEPH &b) -> bool
            {
                return (a.mjd - b.mjd) * 86400.0 + a.sod - b.sod < 0;
            };
            std::unique_lock<std::mutex> lk(q_mutex);
            /* get the newest ephemeris */
            map<string, list<GLSEPH>> m_r;
            for (auto &kv : glsEph)
            {
                m_r[kv.first] = list<GLSEPH>();
                if (!kv.second.empty())
                {
                    auto itr = std::max_element(kv.second.begin(), kv.second.end(), compare);
                    if (itr != kv.second.end())
                    {
                        m_r[kv.first].emplace_back(*itr);
                    }
                }
            }
            return m_r;
        }
        // for member functions
        virtual void v_inputEph(GPSEPH *, GLSEPH *);

    protected:
        void m_inputEphPost(GPSEPH *, GLSEPH *);
        std::mutex q_mutex;
    };
}
#endif // BAMBOO_BRDORBITCLKADAPTER_H
