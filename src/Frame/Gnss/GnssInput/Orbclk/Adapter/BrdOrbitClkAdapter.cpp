//
// Created by juntao, at wuhan university   on 2020/11/2.
//
#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace iono;
BrdOrbitClkAdapter::BrdOrbitClkAdapter()
{
    this->orbType = ORB_BRD;
    this->clkType = CLK_BRD;
}
int BrdOrbitClkAdapter::v_readClk(const char *cprn, int mjd, double sod, double *sclk)
{
    // using rinex navigation file to acquire satellite clock
    int wk, iode = -1, ret;
    double sow, tgd, dtmin;
    *sclk = 0.0;
    mjd2wksow(mjd, sod, &wk, &sow);
    std::unique_lock<std::mutex> lock(q_mutex);
    if (cprn[0] != 'R')
        if (cprn[0] == 'C' || cprn[0] == 'B')
            ret = this->m_brd2xyz("ynn", cprn, wk, sow - 14.0, NULL, sclk, &dtmin, &tgd, &iode);
        else
            ret = this->m_brd2xyz("ynn", cprn, wk, sow, NULL, sclk, &dtmin, &tgd, &iode);
    else
        ret = this->m_gls2xyz("ynn", cprn, wk, sow, NULL, sclk, &dtmin, &iode);
    return ret;
}
int BrdOrbitClkAdapter::v_readClk(const char *cprn, int mjd, double sod, double *sclk, int *iode)
{
    int wk, ret;
    double sow, tgd, dtmin;
    *sclk = 0.0;
    mjd2wksow(mjd, sod, &wk, &sow);
    std::unique_lock<std::mutex> lock(q_mutex);
    if (cprn[0] != 'R')
        if (cprn[0] == 'C' || cprn[0] == 'B')
            ret = this->m_brd2xyz("ynn", cprn, wk, sow - 14.0, NULL, sclk, &dtmin, &tgd, iode);
        else
            ret = this->m_brd2xyz("ynn", cprn, wk, sow, NULL, sclk, &dtmin, &tgd, iode);
    else
        ret = this->m_gls2xyz("ynn", cprn, wk, sow, NULL, sclk, &dtmin, iode);
    return ret;
}
int BrdOrbitClkAdapter::v_readOrbit(const char *cprn, int mjd, double sod, double *xsat)
{
    // using rinex navigation file to acquire orbit
    int wk, iode = -1, ret;
    double sow, tgd, dtmin, sclk;
    mjd2wksow(mjd, sod, &wk, &sow);
    memset(xsat, 0, sizeof(double) * 6);
    std::unique_lock<std::mutex> lock(q_mutex);
    if (cprn[0] != 'R')
        if (cprn[0] == 'C' || cprn[0] == 'B')
            ret = this->m_brd2xyz("yyy", cprn, wk, sow - 14.0, xsat, &sclk, &dtmin, &tgd, &iode);
        else
            ret = this->m_brd2xyz("yyy", cprn, wk, sow, xsat, &sclk, &dtmin, &tgd, &iode);
    else
        ret = this->m_gls2xyz("yyy", cprn, wk, sow, xsat, &sclk, &dtmin, &iode);

    return ret;
}
int BrdOrbitClkAdapter::v_readOrbit(const char *cprn, int mjd, double sod, double *xsat, int *iode)
{
    // using rinex navigation file to acquire orbit
    int wk, ret;
    double sow, tgd, dtmin, sclk;
    mjd2wksow(mjd, sod, &wk, &sow);
    memset(xsat, 0, sizeof(double) * 6);
    std::unique_lock<std::mutex> lock(q_mutex);
    if (cprn[0] != 'R')
        if (cprn[0] == 'C' || cprn[0] == 'B')
            ret = this->m_brd2xyz("yyy", cprn, wk, sow - 14.0, xsat, &sclk, &dtmin, &tgd, iode);
        else
            ret = this->m_brd2xyz("yyy", cprn, wk, sow, xsat, &sclk, &dtmin, &tgd, iode);
    else
        ret = this->m_gls2xyz("yyy", cprn, wk, sow, xsat, &sclk, &dtmin, iode);

    return ret;
}
int BrdOrbitClkAdapter::v_readClkDrift(const char *cprn, int mjd, double sod, double *vclk)
{
    int iode = -1;
    double sclk, sclk_inc, inc = 0.01;
    if (BrdOrbitClkAdapter::v_readClk(cprn, mjd, sod, &sclk, &iode))
    {
        if (BrdOrbitClkAdapter::v_readClk(cprn, mjd, sod + 0.01, &sclk_inc, &iode))
        {
            *vclk = (sclk_inc - sclk) / 0.01;
            return 1;
        }
    }
    return 0;
}
map<string, GPSEPH> BrdOrbitClkAdapter::v_getCurrentGpsEph(double ptime)
{
    std::unique_lock<std::mutex> lock(q_mutex);
    map<string, GPSEPH> r = m_getCurrentGpsEph(ptime);
    return r;
}
map<string, GLSEPH> BrdOrbitClkAdapter::v_getCurrentGlsEph(double ptime)
{
    std::unique_lock<std::mutex> lock(q_mutex);
    map<string, GLSEPH> r = m_getCurrentGlsEph(ptime);
    return r;
}
// the time check is performed outside this function
void BrdOrbitClkAdapter::v_inputEph(GPSEPH *ephgps, GLSEPH *ephgls)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    /* 去除非整秒星历 */
    if (ephgps && fmod(ephgps->sod, 300.0))
        return;
    m_inputEphPost(ephgps, ephgls);
}
void BrdOrbitClkAdapter::m_inputEphPost(GPSEPH *ephgps, GLSEPH *ephgls)
{
    int lexist, isys, mjd_now;
    double dt, ptmax = 0.0, sod_now;
    std::unique_lock<std::mutex> lock(q_mutex);
    if (ephgps != NULL)
    {
        // add the list here,
        lexist = false;
        isys = index_string(SYS, ephgps->cprn[0]);
        list<GPSEPH>::iterator gpsItr;
        ptmax = 0.0;
        ephgps->aode = genAode(SYS[isys], ephgps->mjd, ephgps->sod, ephgps->toe, ephgps->aode);
        if (gnssEph.find(ephgps->cprn) == gnssEph.end())
            gnssEph[ephgps->cprn] = list<GPSEPH>();
        list<GPSEPH> &eph_list = gnssEph[ephgps->cprn];
        lexist = false;
        for (gpsItr = eph_list.begin(); gpsItr != eph_list.end(); ++gpsItr)
        {
            if (ptmax < (*gpsItr).mjd + (*gpsItr).sod / 86400.0)
                ptmax = (*gpsItr).mjd + (*gpsItr).sod / 86400.0; /*find the max time tag of the ephemeris*/
        }

        for (gpsItr = eph_list.begin(); gpsItr != eph_list.end();)
        {
            if ((ptmax - (*gpsItr).mjd) * 86400.0 - (*gpsItr).sod > 86400.0)
            { /* one day interval */
                gpsItr = eph_list.erase(gpsItr);
                continue;
            }
            if (strncmp((*gpsItr).cprn, ephgps->cprn, 3) == 0)
            {
                dt = ((*gpsItr).mjd - ephgps->mjd) * 86400.0 + (*gpsItr).sod - ephgps->sod;
                if (fabs(dt) < 1e-3 && (*gpsItr).aode == ephgps->aode)
                {
                    lexist = true;
                    break;
                }
            }
            ++gpsItr;
        }
        // if (ephgps->cprn[0] == 'E')
        // {
        //     /** galileo Data sources
        //      * Bit 0 set: I/NAV E1-B
        //      * Bit 1 set: F/NAV E5a-I
        //      * Bit 2 set: I/NAV E5b-I
        //      * **/
        //     /**galileo system here**/
        //     int code = (int)ephgps->resvd0;
        //     if (!(code & (1 << 1)))
        //     { /**means this is a inav,will continue**/
        //         lexist = true;
        //     }
        // }
        if (!lexist)
        {
            eph_list.push_front(*ephgps);
        }
    }
    if (ephgls != NULL)
    {
        lexist = false;
        list<GLSEPH>::iterator glsItr;
        isys = index_string(SYS, ephgls->cprn[0]);
        ptmax = 0.0;
        ephgls->aode = genAode(SYS[isys], ephgls->mjd, ephgls->sod, 0.0, ephgls->aode);
        if (glsEph.find(ephgls->cprn) == glsEph.end())
            glsEph[ephgls->cprn] = list<GLSEPH>();
        list<GLSEPH> &eph_list = glsEph[ephgls->cprn];
        for (glsItr = eph_list.begin(); glsItr != eph_list.end(); ++glsItr)
        {
            if (ptmax < (*glsItr).mjd + (*glsItr).sod / 86400.0)
                ptmax = (*glsItr).mjd + (*glsItr).sod / 86400.0; /*find the max time tag of the ephemeris*/
        }
        for (glsItr = eph_list.begin(); glsItr != eph_list.end();)
        {
            if ((ptmax - (*glsItr).mjd) * 86400.0 - (*glsItr).sod > 86400.0)
            { /* one day interval */
                glsItr = eph_list.erase(glsItr);
                continue;
            }
            if (strncmp((*glsItr).cprn, ephgls->cprn, 3) == 0)
            {
                dt = ((*glsItr).mjd - ephgls->mjd) * 86400.0 + (*glsItr).sod - ephgls->sod;
                if (fabs(dt) < 1e-3)
                {
                    lexist = true;
                    break;
                }
            }
            ++glsItr;
        }
        if (!lexist)
        {
            eph_list.push_back(*ephgls);
        }
    }
}
