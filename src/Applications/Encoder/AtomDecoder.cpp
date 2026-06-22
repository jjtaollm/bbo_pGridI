#include "include/Applications/Applications.h"
#include "include/Applications/Encoder/AtomDecoder.h"
#include <cmath>
using namespace std;
using namespace iono;

/* debug begin */
static const double ssrudint[16] = {
    1, 2, 5, 10, 15, 30, 60, 120, 240, 300, 600, 900, 1800, 3600, 7200, 10800};
void AtomDecoder::adjweek(kpl_atmo_rtcm_t *rtcm, double tow)
{
    double tow_p;
    int week;

    /* if no time, get cpu time */
    if (rtcm->time.time == 0)
        rtcm->time = utc2gpst(timeget());
    tow_p = time2gpst(rtcm->time, &week);
    if (tow < tow_p - 302400.0)
        tow += 604800.0;
    else if (tow > tow_p + 302400.0)
        tow -= 604800.0;
    rtcm->time = gpst2time(week, tow);
}
void AtomDecoder::adjday_glot(kpl_atmo_rtcm_t *rtcm, double tod)
{
    gtime_t time;
    double tow, tod_p;
    int week;

    if (rtcm->time.time == 0)
        rtcm->time = utc2gpst(timeget());
    time = timeadd(gpst2utc(rtcm->time), 10800.0); /* glonass time */
    tow = time2gpst(time, &week);
    tod_p = fmod(tow, 86400.0);
    tow -= tod_p;
    if (tod < tod_p - 43200.0)
        tod += 86400.0;
    else if (tod > tod_p + 43200.0)
        tod -= 86400.0;
    time = gpst2time(week, tow + tod);
    rtcm->time = utc2gpst(timeadd(time, -10800.0));
}
int AtomDecoder::decode_ssr_epoch_model(kpl_atmo_rtcm_t *rtcm, int sys, int subtype)
{
    double tod, tow;
    int i = 24 + 12;

    if (subtype == 0)
    { /* RTCM SSR */

        if (SYS[sys] == 'R')
        {
            tod = getbitu(rtcm->buff, i, 17);
            i += 17;
            adjday_glot(rtcm, tod);
        }
        else
        {
            tow = getbitu(rtcm->buff, i, 20);
            i += 20;
            adjweek(rtcm, tow);
        }
    }
    else
    {
        /* IGS SSR */
        i += 3 + 8;
        tow = getbitu(rtcm->buff, i, 20);
        i += 20;
        adjweek(rtcm, tow);
    }
    return i;
}
int AtomDecoder::decode_ssr9_head_model(kpl_atmo_rtcm_t *rtcm, ionmodel_t *s_ionmodel, int sys, int subtype, int *sync, int *iod, double *udint, int *hsize)
{
    char *msg, tstr[64];
    int i = 24 + 12, nsat, udi, provid = 0, solid = 0, ns;
    double clat, clon;

    // if (subtype == 0)
    // { /* RTCM SSR */
    //     ns = (sys == SYS_QZS) ? 4 : 6;
    //     if (i + ((sys == SYS_GLO) ? 52 : 49 + ns) > rtcm->len * 8)
    //         return -1;
    // }
    // else
    // {
    //     ns = 6;
    //     if (i + 3 + 8 + 161 + ns * 48 > rtcm->len * 8)
    //         return -1;
    // }
    if (subtype == 0)
    { /* RTCM SSR */
        return -1;
    }
    i = decode_ssr_epoch_model(rtcm, sys, subtype); // obtain time (i=24+12+3+8+20)
    s_ionmodel->time0 = rtcm->time;
    udi = getbitu(rtcm->buff, i, 4); // SSR Update Interval. bit(4)
    *udint = ssrudint[udi];
    i += 4;
    *sync = getbitu(rtcm->buff, i, 1); // SSR Multiple Message Number. bit(1)
    i += 1;
    *iod = getbitu(rtcm->buff, i, 11); // STEC SSR IOD. 0: NaN, 1-1440: Time span (60s)
    i += 11;
    nsat = getbitu(rtcm->buff, i, 6); // satellite number. uint4
    i += 6;
    // model property parameter
    s_ionmodel->lat0 = getbits(rtcm->buff, i, 25) * 1e-5; // central lat. int25
    i += 25;
    s_ionmodel->lon0 = getbits(rtcm->buff, i, 26) * 1e-5; // cetral lon. int26
    i += 26;
    s_ionmodel->n = getbitu(rtcm->buff, i, 2); // degree of lat. uint2
    i += 2;
    s_ionmodel->m = getbitu(rtcm->buff, i, 2); // degree of lon. uint2
    i += 2;

    time2str(rtcm->time, tstr, 2);
    printf("decode_ssr2_head: time=%s sys=%d subtype=%d nsat=%d sync=%d iod=%d"
           "solid=%d\n",
           tstr, sys, subtype, nsat, *sync, *iod, solid);
    //    printf(4, "decode_ssr2_head: time=%s sys=%d subtype=%d nsat=%d sync=%d iod=%d"
    //              "solid=%d\n", tstr, sys, subtype, nsat, *sync, *iod, solid);

    if (rtcm->outtype)
    {
        msg = rtcm->msgtype + strlen(rtcm->msgtype);
        sprintf(msg, " %s nsat=%2d iod=%2d udi=%2d sync=%d", tstr, nsat, *iod, udi, *sync);
    }
    *hsize = i;
    return nsat;
}
int AtomDecoder::kpl_decode_ssr9_model(kpl_atmo_rtcm_t *rtcm, ionmodel_t *s_ionmodel, int sys, int subtype)
{
    int i, j, type, sync, iod, sat, ura, np, offp, prn, nsat;
    double udint, elev, lat = 0.0, lon = 0.0, hig = 0.0;
    int tc, ic, fc; // int/frac part of coef
    double stec, sigma;

    type = getbitu(rtcm->buff, 24, 12);

    if ((nsat = decode_ssr9_head_model(rtcm, s_ionmodel, sys, subtype, &sync, &iod, &udint, &i)) < 0)
    {
        printf("rtcm3 %d length error: len=%d\n", type, rtcm->len);
        // trace(2, "rtcm3 %d length error: len=%d\n", type, rtcm->len);
        return -1;
    }
    s_ionmodel->nsat = nsat;
    s_ionmodel->udint = udint;
    s_ionmodel->iod = iod;
    s_ionmodel->sync = sync;

    // model coffecient
    for (j = 0; j < nsat && i + 24 <= rtcm->len * 8; ++j)
    {
        prn = getbitu(rtcm->buff, i, 6); // number part of prn. uint6
        i += 6;

        char cprn[LEN_PRN] = {0};
        sprintf(cprn, "%c%02d", SYS[sys], prn);
        memcpy(s_ionmodel->cprn[j], cprn, sizeof(char) * LEN_PRN);

        for (int ii = 0; ii < s_ionmodel->n + 1; ++ii)
        {
            for (int jj = 0; jj < s_ionmodel->m + 1; ++jj)
            {
                tc = getbitu(rtcm->buff, i, 1); // uint1
                i += 1;
                ic = getbitu(rtcm->buff, i, 10); // uint10
                i += 10;
                fc = getbitu(rtcm->buff, i, 17); // uint17
                i += 17;
                s_ionmodel->coef[j][ii * (s_ionmodel->n + 1) + jj] = tc > 0 ? ic + fc * 1e-5 : -(ic + fc * 1e-5);
            }
        }
        s_ionmodel->sig[j] = getbitu(rtcm->buff, i, 9) * 1e-2f; // uint9
        i += 9;
    }
    return sync ? 0 : 9;
}
/* should reference the function decode_type4076 */
int AtomDecoder::s_decode_poly_iono(char *pack, int plen, int isys)
{
    int i = 24 + 12, ver, type, subtype, stat, week, epoch;
    double tow;
    kpl_atmo_rtcm_t rtcm = {0};
    ionmodel_t s_ionmodel = {0};

    /* prepare rtcm for decode_rtcm3 */
    if (plen <= 0)
        return -1;
    memcpy(rtcm.buff, pack, sizeof(char) * plen);
    rtcm.len = plen;

    /* function begin as decode_rtcm3 */
    type = getbitu(rtcm.buff, 24, 12);
    if (type != 4076)
        return -1;

    /* function begin as decode_type4076 */
    i = 24 + 12;
    if (i + 3 + 8 >= rtcm.len * 8)
    {
        printf("rtcm3 4076: length error len=%d\n", rtcm.len);
        return -1;
    }

    ver = getbitu(rtcm.buff, i, 3);
    i += 3;
    subtype = getbitu(rtcm.buff, i, 8);
    i += 8;

    switch (subtype)
    {
    case 28: /* GPS */
        stat = kpl_decode_ssr9_model(&rtcm, &s_ionmodel, isys, subtype);
        break;
    case 48: /* GLONASS */
        stat = kpl_decode_ssr9_model(&rtcm, &s_ionmodel, isys, subtype);
        break;
    case 68: /* Galileo */
        stat = kpl_decode_ssr9_model(&rtcm, &s_ionmodel, isys, subtype);
        break;
    case 108: /* BDS-2 */
        stat = kpl_decode_ssr9_model(&rtcm, &s_ionmodel, isys, subtype);
        break;
    case 109: /* BDS-3 */
        stat = kpl_decode_ssr9_model(&rtcm, &s_ionmodel, isys, subtype);
        break;
    case 88: /* QZSS */
        stat = kpl_decode_ssr9_model(&rtcm, &s_ionmodel, isys, subtype);
        break;

    default:
        return -1;
        break;
    }

    if (stat >= 0)
    {
        if (!Logtrace::s_defaultlogger.m_lexist("debug_ion_decode.out"))
            Logtrace::s_defaultlogger.m_openLog("debug_ion_decode.out");
        Logtrace::s_defaultlogger.m_wtMsg("@%s %s\n", "debug_ion_decode.out", "[SSR body begin]");
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_decode.out", "*RTCM Message Number:", 4076);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_decode.out", "*IGM/IM Version:", 1);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_decode.out", "*IGS Message Number:", subtype);
        tow = time2gpst(s_ionmodel.time0, &week);
        epoch = ((int)floor((tow) + 0.5)) % 604800;
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_decode.out", "*Epoch:", epoch);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_decode.out", "*SSR Update Interval:", s_ionmodel.udint);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_decode.out", "*SSR Multiple Message Indicator:", s_ionmodel.sync);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_decode.out", "*SSR STEC IOD:", s_ionmodel.iod);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.5f\n", "debug_ion_decode.out", "*Central Latitude:", s_ionmodel.lat0);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.5f\n", "debug_ion_decode.out", "*Central Longtitude:", s_ionmodel.lon0);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_decode.out", "*Order of Latitude:", s_ionmodel.n);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_decode.out", "*Order of Longtitude:", s_ionmodel.m);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_decode.out", "*Number of satellite:", s_ionmodel.nsat);
        for (int isat = 0; isat < s_ionmodel.nsat; ++isat)
        {
            Logtrace::s_defaultlogger.m_wtMsg("@%s %35s\n", "debug_ion_decode.out", "----------");
            Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %s\n", "debug_ion_decode.out", "*PRN:", s_ionmodel.cprn[isat]);

            for (int ipar = 0; ipar < (s_ionmodel.m + 1) * (s_ionmodel.n + 1); ++ipar)
            {
                Logtrace::s_defaultlogger.m_wtMsg("@%s %20.5f\n", "debug_ion_decode.out", s_ionmodel.coef[isat][ipar]);
            }

            Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.2f\n", "debug_ion_decode.out", "*Sigma:", s_ionmodel.sig[isat]);
        }
        Logtrace::s_defaultlogger.m_wtMsg("@%s %s\n", "debug_ion_decode.out", "[SSR body end]");
    }

    return stat;
}
/* Grid model ----------------------------------------------------------*/
int AtomDecoder::decode_ssr_epoch_grid(kpl_atmo_rtcm_t *rtcm, int sys, int subtype)
{
    double tod, tow;
    int i = 24 + 12;

    if (subtype == 0)
    { /* RTCM SSR */

        if (SYS[sys] == 'R')
        {
            tod = getbitu(rtcm->buff, i, 17);
            i += 17;
            adjday_glot(rtcm, tod);
        }
        else
        {
            tow = getbitu(rtcm->buff, i, 20);
            i += 20;
            adjweek(rtcm, tow);
        }
    }
    else
    {
        /* 4065 */
        i += 3 + 8 + 13;
        tow = getbitu(rtcm->buff, i, 20);
        i += 20;
        adjweek(rtcm, tow);
    }
    return i;
}
int AtomDecoder::decode_ssr9_head_grid(kpl_atmo_rtcm_t *rtcm, iongrid_t *s_iongrid, int sys, int subtype, int *sync, int *iod, double *udint, int *hsize)
{
    char *msg, tstr[64];
    int i = 24 + 12, nsat, udi, solid = 0, ns, dtype;

    // if (subtype == 0)
    // { /* RTCM SSR */
    //     ns = (sys == SYS_QZS) ? 4 : 6;
    //     if (i + ((sys == SYS_GLO) ? 52 : 49 + ns) > rtcm->len * 8)
    //         return -1;
    // }
    // else
    // {
    //     ns = 6;
    //     if (i + 3 + 8 + 161 + ns * 48 > rtcm->len * 8)
    //         return -1;
    // }
    if (subtype == 0)
    { /* RTCM SSR */
        return -1;
    }
    i = decode_ssr_epoch_grid(rtcm, sys, subtype); // obtain time
    s_iongrid->time = rtcm->time;
    dtype = getbitu(rtcm->buff, i, 3); /* Date Type Identification. 0: STEC (include BIAS error) */
    s_iongrid->dtype = dtype;
    i += 3;
    *iod = getbitu(rtcm->buff, i, 11); /* STEC SSR IOD. 0: NaN, 1-1440: Time span (60s) */
    i += 11;
    udi = getbitu(rtcm->buff, i, 4); /* SSR Update Interval */
    *udint = ssrudint[udi];
    i += 4;
    *sync = getbitu(rtcm->buff, i, 1); /* SSR Multiple Message Number */
    i += 1;
    solid = getbitu(rtcm->buff, i, 3); /* SSR Solution ID */
    s_iongrid->solid = solid;
    i += 3;
    // s_iongrid->geod[0] = getbits64(rtcm->buff, i, 38) * 1e-9; /* Reference latitude, 38 bits */
    // i += 38;
    // s_iongrid->geod[1] = getbits64(rtcm->buff, i, 39) * 1e-9; /* Reference longtitude, 39 bits */
    // i += 39;
    s_iongrid->geod[0] = getbits(rtcm->buff, i, 31) * 1e-6; /* Reference latitude, 38 bits */
    i += 38;
    s_iongrid->geod[1] = getbits(rtcm->buff, i, 31) * 1e-6; /* Reference longtitude, 39 bits */
    i += 39;
    s_iongrid->geod[2] = getbits(rtcm->buff, i, 23) * 1e-2; /* Reference Height, 23 bits */
    i += 23;
    nsat = getbitu(rtcm->buff, i, 6); /* number of satellites. uint6 */
    i += 6;

    time2str(rtcm->time, tstr, 2);
    printf("decode_ssr2_head: time=%s sys=%d subtype=%d nsat=%d sync=%d iod=%d"
           "solid=%d\n",
           tstr, sys, subtype, nsat, *sync, *iod, solid);
    //    printf(4, "decode_ssr2_head: time=%s sys=%d subtype=%d nsat=%d sync=%d iod=%d"
    //              "solid=%d\n", tstr, sys, subtype, nsat, *sync, *iod, solid);

    if (rtcm->outtype)
    {
        msg = rtcm->msgtype + strlen(rtcm->msgtype);
        sprintf(msg, " %s nsat=%2d iod=%2d udi=%2d sync=%d", tstr, nsat, *iod, udi,
                *sync);
    }
    *hsize = i;
    return nsat;
}
int AtomDecoder::kpl_decode_ssr9_grid(kpl_atmo_rtcm_t *rtcm, iongrid_t *s_iongrid, int sys, int subtype)
{
    double stec, sigma;
    double udint, elev, lat = 0.0, lon = 0.0, hig = 0.0;
    int i, j, type, sync, iod, sat, ura, np, offp, prn, nsat;

    type = getbitu(rtcm->buff, 24, 12);

    if ((nsat = decode_ssr9_head_grid(rtcm, s_iongrid, sys, subtype, &sync, &iod, &udint, &i)) < 0)
    {
        printf("rtcm3 %d length error: len=%d\n", type, rtcm->len);
        // trace(2, "rtcm3 %d length error: len=%d\n", type, rtcm->len);
        return -1;
    }
    s_iongrid->nsat = nsat;
    s_iongrid->udint = udint;
    s_iongrid->iod = iod;
    s_iongrid->sync = sync;

    for (j = 0; j < nsat && i + 24 <= rtcm->len * 8; ++j)
    {
        prn = getbitu(rtcm->buff, i, 6); // number part of prn. uint6
        i += 6;

        char cprn[LEN_PRN] = {0};
        sprintf(cprn, "%c%02d", SYS[sys], prn);
        memcpy(s_iongrid->cprn[j], cprn, sizeof(char) * LEN_PRN);

        s_iongrid->elev[j] = getbitu(rtcm->buff, i, 10) * 0.1; /* GNSS Satellite Elevation Angle, uint10, 0.1 degree */
        i += 10;
        s_iongrid->ionval[j] = getbits(rtcm->buff, i, 14) * 5e-2f; /* STEC, int14, unit:0.05 TECU */
        i += 14;
        s_iongrid->ionsig[j] = getbitu(rtcm->buff, i, 8) * 5e-2f; /* Sigma, uint8, unit:0.05 TECU */
        i += 8;

        i += 10;
    }
    return sync ? 0 : 9;
}
int AtomDecoder::s_decode_grid_iono(char *pack, int plen, int isys)
{
    int i = 24 + 12, ver, type, subtype, stat, week, epoch;
    double tow;
    kpl_atmo_rtcm_t rtcm = {0};
    iongrid_t s_iongrid = {0};

    /* prepare rtcm for decode_rtcm3 */
    if (plen <= 0)
        return -1;
    memcpy(rtcm.buff, pack, sizeof(char) * plen);
    rtcm.len = plen;

    /* function begin as decode_rtcm3 */
    type = getbitu(rtcm.buff, 24, 12);
    if (type != 4065)
        return -1;

    /* function begin as decode_type4076 */
    i = 24 + 12;
    if (i + 3 + 8 >= rtcm.len * 8)
    {
        printf("rtcm3 4076: length error len=%d\n", rtcm.len);
        return -1;
    }

    ver = getbitu(rtcm.buff, i, 3);
    i += 3;
    subtype = getbitu(rtcm.buff, i, 8);
    i += 8;

    switch (subtype)
    {
    case 101: /* GPS */
        stat = kpl_decode_ssr9_grid(&rtcm, &s_iongrid, isys, subtype);
        break;
    case 104: /* GLONASS */
        stat = kpl_decode_ssr9_grid(&rtcm, &s_iongrid, isys, subtype);
        break;
    case 103: /* Galileo */
        stat = kpl_decode_ssr9_grid(&rtcm, &s_iongrid, isys, subtype);
        break;
    case 102: /* BDS */
        stat = kpl_decode_ssr9_grid(&rtcm, &s_iongrid, isys, subtype);
        break;
    case 105: /* QZSS */
        stat = kpl_decode_ssr9_grid(&rtcm, &s_iongrid, isys, subtype);
        break;

    default:
        return -1;
        break;
    }

    if (stat >= 0)
    {
        if (!Logtrace::s_defaultlogger.m_lexist("debug_ion_grid_decode.out"))
            Logtrace::s_defaultlogger.m_openLog("debug_ion_grid_decode.out");
        Logtrace::s_defaultlogger.m_wtMsg("@%s %s\n", "debug_ion_grid_decode.out", "[SSR body begin]");
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_decode.out", "*RTCM Message Number:", 4056);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_decode.out", "*IGM/IM Version:", 0);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_decode.out", "*IGS Message Number:", subtype);
        tow = time2gpst(s_iongrid.time, &week);
        epoch = ((int)floor((tow) + 0.5)) % 604800;
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_decode.out", "*Epoch:", epoch);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_decode.out", "*Date Type Identification:", s_iongrid.dtype);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_decode.out", "*STEC SSR IOD:", s_iongrid.iod);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_decode.out", "*SSR Update Interval:", s_iongrid.udint);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_decode.out", "*SSR Multiple Message Indicator:", s_iongrid.sync);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.6f\n", "debug_ion_grid_decode.out", "*Grid Latitude:", s_iongrid.geod[0]);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.6f\n", "debug_ion_grid_decode.out", "*Grid Longtitude:", s_iongrid.geod[1]);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.6f\n", "debug_ion_grid_decode.out", "*Grid Height:", s_iongrid.geod[2]);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_decode.out", "*Number of satellite:", s_iongrid.nsat);
        for (int isat = 0; isat < s_iongrid.nsat; ++isat)
        {

            Logtrace::s_defaultlogger.m_wtMsg("@%s %35s\n", "debug_ion_grid_decode.out", "----------");
            Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %3s%17.3f\n", "debug_ion_grid_decode.out", "*PRN:", s_iongrid.cprn[isat], s_iongrid.ionval[isat]);
            Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.3f\n", "debug_ion_grid_decode.out", "*Sigma:", s_iongrid.ionsig[isat]);
        }

        Logtrace::s_defaultlogger.m_wtMsg("@%s %s\n", "debug_ion_grid_decode.out", "[SSR body end]");
    }

    return stat;
}
/* debug end */