/*
 * Rnxobs.cpp
 *
 *  Created on: 2018/5/5/
 *      Author: juntao, at wuhan university
 */
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace std;
using namespace iono;
// only support dual-frequency
bool Rnxobs::s_checkObs(int psat, double *obs, double *dop, double *snr)
{
    int ifreq, isys, i, j, k, nrc, npc;
    bool breset = false;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    // isys = index_string(SYS, dly->cprn[psat][0]);
    isys = index_string(SYS, dly->prn_alias[psat][0]);
    if (fabs(obs[MAXFREQ]) < MAXWND || fabs(obs[MAXFREQ + 1]) < MAXWND || fabs(obs[MAXFREQ] - obs[MAXFREQ + 1]) > 50.0)
    {
        if (fabs(obs[MAXFREQ]) > MAXWND || fabs(obs[MAXFREQ + 1]) > MAXWND)
        {
            breset = true;
            bool b_got_4 = fabs(obs[MAXFREQ]) > MAXWND && fabs(obs[MAXFREQ + 1]) > MAXWND;
        }
        memset(obs, 0, sizeof(double) * 2 * MAXFREQ);
        memset(dop, 0, sizeof(double) * MAXFREQ);
        memset(snr, 0, sizeof(double) * MAXFREQ);
    }

    if (fabs(obs[0]) < MAXWND || fabs(obs[1]) < MAXWND)
    {
        if (fabs(obs[0]) > MAXWND || fabs(obs[1]) > MAXWND)
        {
            breset = true;
        }
        memset(obs, 0, sizeof(double) * 2 * MAXFREQ);
        memset(dop, 0, sizeof(double) * MAXFREQ);
        memset(snr, 0, sizeof(double) * MAXFREQ);
    }
    return breset;
}

void Rnxobs::m_wl_azel(int mjd, double sod, double *elevation, double *azim, double (*obs)[MAXFREQ * 2])
{
    int i, isat, mjd_send, iq, isys;
    double sod_send, r1[3], dump[3], rot_l2f[3][3], geod[3];
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    /// compute the azim and elev
    memset(elevation, 0, sizeof(double) * MAXSAT);
    memset(azim, 0, sizeof(double) * MAXSAT);
    if (x[0] * x[1] * x[2] == 0.0)
        return;
    xyzblh(x, 1.0, 0, 0, 0, 0, 0, geod);
    rot_enu2xyz(geod[0], geod[1], rot_l2f);
    for (isat = 0; isat < dly->nprn; ++isat)
    {
        Satellite &sat = dly->SAT[isat];
        isys = index_string(SYS, dly->prn_alias[isat][0]);
        for (iq = 0; iq < MAXFREQ; ++iq)
            if (obs[isat][iq + MAXFREQ] != 0.0)
                break;
        if (iq != MAXFREQ)
        {
            timinc(mjd, sod, -obs[isat][MAXFREQ + iq] / VEL_LIGHT, &mjd_send, &sod_send);
            if (sat.satpos[0] != 0.0)
            {
                for (i = 0; i < 3; i++)
                    r1[i] = sat.satpos[i] - x[i];
                matmpy(r1, (double *)rot_l2f, dump, 1, 3, 3);
                elevation[isat] = atan(dump[2] / sqrt(dump[0] * dump[0] + dump[1] * dump[1]));
                azim[isat] = atan2(dump[0], dump[1]);
            }
        }
    }
}

void Rnxobs::m_onDataQC()
{
    int isat, isys, ifreq, nflag = 0;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    m_wl_azel(mjd, sod, elev, azim, obs);
    m_qcDetectSlip_Df(mjd, sod, staname, obs, elev, qc, slip);
}
