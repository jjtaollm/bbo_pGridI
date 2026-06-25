#include "include/Frame/Frame.h"
#include "include/Applications/Applications.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace iono;

/* compute raw GNSS L4 STEC observations */
bbo_dstec::bbo_dstec()
{
    reader = new RnxEphBrdReader;
    brd = new BrdOrbitClkAdapter;
    reader->v_addOrbclkAdapter(brd);
}
bbo_dstec::~bbo_dstec()
{
    if (reader)
        delete reader;
    if (brd)
        delete brd;
}
void bbo_dstec::m_process()
{
    int mjdSync = -1;
    Controller *ctl = Controller::s_getInstance();
    Deploy *mDly = &ctl->mDly;
    mDly->mjd = mDly->mjd0;
    mDly->sod = mDly->sod0;
    for (auto &s : mDly->mSta)
    {
        s2rnx[s.first] = new RnxobsFile(s.first);
        memcpy(s2rnx[s.first]->x, s.second.x, sizeof(double) * 3);
    }
    while (true)
    {
        m_initInputs(mDly->mjd, mDly->sod, mjdSync != mDly->mjd);
        mjdSync = mDly->mjd;
        for (auto &s : s2rnx)
        {
            s.second->v_readEpoch(mDly->mjd, mDly->sod);
            s.second->m_onDataQC();
        }
        map<string, map<string, double>> s2stec = m_computeRawStec();
        m_outputStec(mDly->mjd, mDly->sod, s2stec);
        timinc(mDly->mjd, mDly->sod, mDly->dintv, &mDly->mjd, &mDly->sod);
        if ((mDly->mjd - mDly->mjd1) * 86400.0 + mDly->sod - mDly->sod1 >= 0.0)
            break;
    }
}

void bbo_dstec::m_initInputs(int mjd, double sod, int b_reset)
{
    Controller *ctl = Controller::s_getInstance();
    if (b_reset)
    {
        for (auto &s : s2rnx)
        {
            if (s.second->v_isOpen())
                s.second->v_closeRnx();
        }
        if (reader && reader->v_isOpen())
            reader->v_closeRnxEph();
    }

    for (auto &s : s2rnx)
    {
        if (!s.second->v_isOpen())
            s.second->v_openRnx(toString(mjd) + ":" + toString(sod));
    }
    if (!reader->v_isOpen())
        reader->v_openRnxEph(toString(mjd) + ":" + toString(sod));

    for (auto &sat : ctl->mDly.SAT)
    {
        sat.xclk = 0.0;
        memset(sat.satpos, 0, sizeof(sat.satpos));
        if (!brd->v_readClk(sat.cprn, mjd, sod, &sat.xclk))
        {
            ;
        }
        if (!brd->v_readOrbit(sat.cprn, mjd, sod, sat.satpos))
        {
            ;
        }
    }
}
map<string, map<string, double>> bbo_dstec::m_computeRawStec()
{
    int isat, isys;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    map<string, map<string, double>> r2v;
    for (auto &kv : s2rnx)
    {
        Rnxobs *sta = kv.second;
        map<string, double> s2v;
        for (isat = 0; isat < dly->nprn; ++isat)
        {
            string cprn = dly->cprn[isat];
            isys = index_string(SYS, dly->prn_alias[isat][0]);
            sta->flag[isat] = sta->slip[isat][0] || sta->slip[isat][1];
            if (sta->obs[isat][0] != 0.0 && sta->obs[isat][1] != 0.0 && sta->elev[isat] * R2D > 10.0)
            {
                double fq0 = freqbysys(isys, -1, dly->freq[isys][0]), fq1 = freqbysys(isys, -1, dly->freq[isys][1]);
                double lam1 = VEL_LIGHT / fq0, lam2 = VEL_LIGHT / fq1;

                // double ss = (lam1 * sta->obs[isat][0] - lam2 * sta->obs[isat][1]);
                // double s2 = ss * fq0 * fq0 * fq1 * fq1 / (fq0 * fq0 - fq1 * fq1) / 40.28 / 1e16;
                double stec = -1 * _IF_1(fq0, fq1) * (lam1 * sta->obs[isat][0] - lam2 * sta->obs[isat][1]); /* in meters */
                stec = m2stec(stec, fq0);                                                                   /* in stec */
                s2v[cprn] = stec;
            }
        }
        r2v[kv.first] = s2v;
    }
    return r2v;
}

void bbo_dstec::m_outputStec(int mjd, double sod, map<string, map<string, double>> &r2v)
{
    double ep[6] = {0};
    int iyear, idoy;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    char filename[1024] = {0};
    idoy = mjd2doy(mjd, &iyear);
    time2epoch(mjd2time(mjd, sod), ep);
    if (KvContainer::getv("RAW_STEC_OUTPUT") != toString(idoy))
    {
        Logtrace::s_defaultlogger.m_closeLog(KvContainer::getv("raw_stec"));
        sprintf(filename, "%s/raw_stec_%d%d", dly->outdir, iyear, idoy);

        Logtrace::s_defaultlogger.m_openLog(filename, dly->lpost ? false : true, 0);
        KvContainer::setv("raw_stec", filename);

        KvContainer::setv("RAW_STEC_OUTPUT", toString(idoy));
    }

    string fv = KvContainer::getv("raw_stec");
    /////////////////////////////////// clock part //////////////////////////////////////
    if (fv != "" && Logtrace::s_defaultlogger.m_lexist(fv))
    {
        for (auto &kv : r2v)
        {
            Rnxobs *ob = s2rnx[kv.first];
            for (auto &s2v : kv.second)
            {
                int isat = pointer_string(dly->nprn, dly->cprn, s2v.first);
                Logtrace::s_defaultlogger.m_wtMsg("@%s STEC %s %d   %7.1lf %10s %s %9.2lf %9.2lf %12.3lf %10s %02d\n", fv.c_str(), kv.first.c_str(), mjd, sod, " ", s2v.first.c_str(),
                                                  ob->elev[isat] * R2D, ob->azim[isat] * R2D, s2v.second, " ", ob->flag[isat]);
                ob->flag[isat] = 0; /* means this slip is handled (output) */
            }
        }
    }
}
