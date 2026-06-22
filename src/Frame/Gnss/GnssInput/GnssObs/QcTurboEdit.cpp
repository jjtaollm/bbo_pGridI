#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace std;
using namespace iono;
void QcTurboEdit::m_qcDetectSlip_Sf(int mjd, double sod, string staname, double (*obs)[2 * MAXFREQ], double *elev, QcSlip qc[MAXSAT][MAXFREQ], int slip[MAXSAT][MAXFREQ])
{
    int ifq, jfq, nct, isat, isys, ifreq, indx, k;
    double f1, dt[MAXSAT] = {0}, tec, lamda1, dpr;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    memset(slip, 0, sizeof(int) * MAXSAT * MAXFREQ);
    for (isat = 0; isat < dly->nprn; isat++)
    {
        // isys = index_string(SYS, dly->cprn[isat][0]);
        isys = index_string(SYS, dly->prn_alias[isat][0]);
        if (obs[isat][0] == 0) /* single frequency, will only check the first frequency */
            continue;
        f1 = freqbysys(isys, -1, dly->freq[isys][0]);
        lamda1 = VEL_LIGHT / f1;
        dpr = obs[isat][0] - obs[isat][MAXFREQ] / lamda1;
        dt[isat] = (mjd - qc[isat][0].mjd) * 86400.0 + sod - qc[isat][0].sod;
        if (dt[isat] > dly->gap)
        {
            for (ifreq = 0; ifreq < dly->nfreq[isys]; ifreq++)
                slip[isat][ifreq] = 1;
            LOGPRINT("[%d %8.1lf] [%s %s] reset as new ambiguity since gap",
                     mjd, sod, staname.c_str(), dly->cprn[isat].c_str());
        }
        else
        {
            if (fabs(dpr - qc[isat][0].dpr) > dly->lg * sqrt(dt[isat]))
            {
                for (ifreq = 0; ifreq < dly->nfreq[isys]; ifreq++)
                    slip[isat][ifreq] = 1;
                LOGPRINT(
                    "[%d %8.1lf] [%s %s] new ambiguity: (c)%6.2lf vs "
                    "(m)%6.2lf(dpr) (c)%6.2lf vs (m)%6.2lf(dtec) (c)%6.2lf vs "
                    "(m)%6.2lf(lg) ",
                    mjd, sod, staname.c_str(), dly->cprn[isat].c_str(), dpr,
                    qc[isat][0].dpr, dly->lg * sqrt(dt[isat]));
            }
        }
        qc[isat][0].dpr = dpr;
        qc[isat][0].mjd = mjd;
        qc[isat][0].sod = sod;
        m_dt_span[isat][0] = dt[isat];
    }
}
void QcTurboEdit::m_qcDetectSlip_Df(int mjd, double sod, string staname, double (*obs)[2 * MAXFREQ], double *elev, QcSlip qc[MAXSAT][MAXFREQ], int slip[MAXSAT][MAXFREQ])
{
    int isat, isys, ifreq, ifq, jfq, nct, inconsistent;
    double dt[MAXSAT][MAXFREQ] = {0}, f1, f2, lamda1, lamda2, g, lamdw, tec, mw, om, oxsig, dmw, dtec, tecr, mm, tecth, mwth;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    memset(slip, 0, sizeof(int) * MAXSAT * MAXFREQ);
    memset(dt, 0, sizeof(dt));
    for (isat = 0; isat < dly->nprn; ++isat)
    {
        // isys = index_string(SYS, dly->cprn[isat][0]);
        isys = index_string(SYS, dly->prn_alias[isat][0]);
        for (ifreq = 0; ifreq < dly->nfreq[isys]; ++ifreq)
        {
            if (obs[isat][ifreq] != 0.0)
            {
                if ((mjd - qc[isat][ifreq].ptime_lastobs) * 86400.0 + sod > dly->gap)
                {
                    slip[isat][ifreq] = 1;

                    LOGPRINT("[%d %8.1lf] [%s %s %s] reset as new ambiguity since gap", mjd, sod, staname.c_str(), dly->cprn[isat].c_str(), dly->freq[isys][ifreq]);
                }
            }
        }
        for (ifq = 0, nct = 0; ifq < dly->nfreq[isys]; ifq++)
        {
            for (jfq = ifq + 1; jfq < dly->nfreq[isys]; jfq++)
            {
                if (obs[isat][ifq] != 0.0 && obs[isat][jfq] != 0.0 && obs[isat][ifq + MAXFREQ] != 0.0 && obs[isat][jfq + MAXFREQ] != 0.0)
                {
                    qc[isat][nct].smtecr.m_remove([this, mjd, sod](int c) -> bool
                                                  { return c < (mjd2time(mjd, sod).time - 301.0); });

                    f1 = freqbysys(isys, -1, dly->freq[isys][ifq]);
                    f2 = freqbysys(isys, -1, dly->freq[isys][jfq]);
                    lamda1 = VEL_LIGHT / f1;
                    lamda2 = VEL_LIGHT / f2;
                    g = f1 / f2;
                    lamdw = VEL_LIGHT / (f1 - f2);

                    tec = lamda1 * obs[isat][ifq] - lamda2 * obs[isat][jfq];
                    mw = obs[isat][ifq] - obs[isat][jfq] - (g * obs[isat][MAXFREQ + ifq] + obs[isat][MAXFREQ + jfq]) / (1.0 + g) / lamdw;
                    dt[isat][nct] = fabs((mjd - qc[isat][nct].mjd) * 86400.0 + sod - qc[isat][nct].sod);
                    if (dt[isat][nct] > dly->gap)
                    {
                        qc[isat][nct].n_mw = 0;
                        qc[isat][nct].mean_mw = 0;
                        qc[isat][nct].smtecr.m_reset();
                    }
                    else
                    {
                        dmw = mw - qc[isat][nct].mean_mw;
                        dtec = tec - qc[isat][nct].tec;
                        tecr = dtec / dt[isat][nct];
                        mm = qc[isat][nct].smtecr.m_getcount() == 0 ? 0.0 : qc[isat][nct].smtecr.m_getmean(); /* mean tecr */
                                                                                                              // dtec = dtec - qc[isat][nct].tecr * dt[isat];
                        if (fabs(dt[isat][nct]) < 10)
                            dtec = dtec - mm * dt[isat][nct];
                        mwth = (nct == 2) ? 0.8 : dly->lw;
                        if (mm == 0.0)
                            tecth = 0.15; // 0.15m difference
                        else
                        {
                            tecth = dly->dtec * (1 - exp(-dt[isat][nct] / 60.0) / 2);
                        }
                        if (elev[isat] != 0.0 && elev[isat] * RAD2DEG < 30)
                        {
                            mwth = MIN(10.0, mwth / (2 * sin(elev[isat])));
                        }
                        if (SYS[isys] == 'R')
                            mwth = mwth * 2;

                        if (fabs(dmw) > mwth || fabs(dtec) > tecth)
                        {

                            // slip[isat][ifq] = slip[isat][jfq] = 1;

                            slip[isat][ifq] = fabs(dtec) > tecth ? 2 : 3;
                            slip[isat][jfq] = fabs(dtec) > tecth ? 2 : 3;

                            qc[isat][nct].n_mw = 0;
                            qc[isat][nct].mean_mw = 0;
                            qc[isat][nct].smtecr.m_reset();
                            if (elev[isat] == 0 || elev[isat] * RAD2DEG >= 7)
                            {
                                LOGPRINT("[%d %8.1lf] [%s %s %s %s] new ambiguity: "
                                         "(c)%6.2lf vs (m)%6.2lf(mw) (c)%6.2lf vs "
                                         "(m)%6.2lf(dtec) %7.2lf(elev) %8.1lf(dt)",
                                         mjd, sod, staname.c_str(),
                                         dly->cprn[isat].c_str(),
                                         dly->freq[isys][ifq],
                                         dly->freq[isys][jfq], fabs(dmw), mwth,
                                         fabs(dtec), tecth,
                                         elev[isat] * RAD2DEG, dt[isat][nct]);
                            }
                        }
                        else
                        {

                            if (fmod(sod, 30) == 0.0)
                                qc[isat][nct].smtecr.m_smooth((int)mjd2time(mjd, sod).time, tecr, om, oxsig);
                        }
                    }
                    qc[isat][nct].mjd = mjd;
                    qc[isat][nct].sod = sod;
                    qc[isat][nct].tec = tec;
                    qc[isat][nct].mean_mw = (qc[isat][nct].mean_mw * qc[isat][nct].n_mw + mw) / (qc[isat][nct].n_mw + 1);
                    ++qc[isat][nct].n_mw;
                }
                ++nct;
            }
        }

        for (ifreq = 0; ifreq < dly->nfreq[isys]; ifreq++)
        {
            if (obs[isat][ifreq] != 0.0)
            {
                qc[isat][ifreq].ptime_lastobs = mjd + sod / 86400.0;
            }
        }
    }
    memcpy(m_dt_span, dt, sizeof(double) * MAXSAT * MAXFREQ);
}
void QcTurboEdit::m_qcDetectSlip_Tf(int mjd, double sod, string staname, double (*obs)[2 * MAXFREQ], double *elev)
{
    /* triple-frequency slip detection */
}