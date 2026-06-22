#include "include/Applications/Applications.h"
#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
#include "include/Applications/Encoder/GridEncoder.h"
#include <cmath>
using namespace std;
using namespace iono;
/* SSR update intervals ------------------------------------------------------*/
static const double ssrudint[16] = {
    1, 2, 5, 10, 15, 30, 60, 120, 240, 300, 600, 900, 1800, 3600, 7200, 10800};
char *GridEncoder::s_encode_slant_iono(map<string, pair<double, double>> &m_v, const int isys, const int iod, const int udint, const double mlat0, const double mlon0, const double mheight0, gtime_t t0, int sync, int &ilen)
{
    int udi, mjd, week, epoch;
    unsigned int crc, submsgid, n_bsize = 8168, i = 0, nsat = 0;
    double tow;
    bool ldebug = false;
    list<int> union_brd;
    char *buff = (char *)calloc(n_bsize, sizeof(char));
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    /* step 1, check GNSS system type */
    ilen = 0;
    if (isys < MAXSYS && isys >= 0)
    {
        switch (SYS[isys])
        {
        case 'G': /* GPS */
            submsgid = 101;
            break;
        case 'R': /* GLONASS */
            submsgid = 104;
            break;
        case 'E': /* Galileo */
            submsgid = 103;
            break;
        case 'C': /* BDS */
        case 'B':
            submsgid = 102;
            break;
        case 'J': /* QZSS */
            submsgid = 105;
            break;
        default:
            free(buff);
            return NULL;
            break;
        }
    }
    else
    {
        free(buff);
        return NULL;
    }

    /* step 2, get number of satellites with parameters limitation */
    nsat = 0;
    for (auto &kv : m_v)
    {
        if (kv.first[0] != SYS[isys])
            continue;
        if (fabs(kv.second.first) > 409.55 || fabs(kv.second.second) > 12.75)
            continue;
        union_brd.push_back(atoi(kv.first.c_str() + 1));
    }
    nsat = union_brd.size();

    /* step 3, header of RTCM, sum: 24 bits */
    /* Preamble, 8 bits. */
    setbitu((unsigned char *)buff, i, 8, RTCM3PREAMB); /* The Preamble is aifxed 8-bit sequence, for RTCM, it is 11010011 */
    i += 8;
    /* Reserved, 6 bits */
    setbitu((unsigned char *)buff, i, 6, 0); /* Not fefined - set to 000000 */
    i += 6;
    /* Message Length, 10 bits */
    setbitu((unsigned char *)buff, i, 10, ilen); /* Message length in bytes, it will be update later */
    i += 10;

    /* step 3, KPL header at rtcm 3 message body */
    /* RTCM Message Number, always 4065 for KPL STEC. 12 bits */
    setbitu((unsigned char *)buff, i, 12, 4065);
    i += 12;
    /* IGM/IM Version, set as 0. uint3 */
    setbitu((unsigned char *)buff, i, 3, 0);
    i += 3;
    /* KPL Message Number. uint8 */
    setbitu((unsigned char *)buff, i, 8, submsgid);
    i += 8;
    /* Reserved 1 */
    setbitu((unsigned char *)buff, i, 13, 0);
    i += 13;
    /* epoch time. uint20 */
    tow = time2gpst(t0, &week);
    epoch = ((int)floor((tow) + 0.5)) % 604800;
    setbitu((unsigned char *)buff, i, 20, epoch);
    i += 20;
    /* Date Type Identification. 0: STEC (include BIAS error) */
    setbitu((unsigned char *)buff, i, 3, 0);
    i += 3;
    /* STEC SSR IOD. 0: NaN, 1-1440: Time span (60s) */
    // sod = time2mjd(t0, &mjd);
    // iod = (int)(sod / 60) + 1;
    setbitu((unsigned char *)buff, i, 11, iod);
    i += 11;
    /* SSR Update Interval. bit(4) */
    for (udi = 0; udi < 15; udi++)
    {
        if (ssrudint[udi] >= udint)
            break;
    }
    setbitu((unsigned char *)buff, i, 4, udi);
    i += 4;
    /* SSR Multiple Message Indicator. bit(1) */
    setbitu((unsigned char *)buff, i, 1, sync);
    i += 1;
    /* SSR Solution ID */
    setbitu((unsigned char *)buff, i, 3, 0);
    i += 3;
    /* Reference latitude, 38 bits (31+7) */
    // setbits64((unsigned char *)buff, i, 38, NINT64(mlat0 * 1e9));
    // i += 38;
    setbits((unsigned char *)buff, i, 31, NINT(mlat0 * 1e6));
    i += 31;
    setbitu((unsigned char *)buff, i, 7, 0);
    i += 7;
    /* Reference longtitude, 39 bits (31+8) */
    // setbits64((unsigned char *)buff, i, 39, NINT64(mlon0 * 1e9));
    // i += 39;
    setbits((unsigned char *)buff, i, 31, NINT(mlon0 * 1e6));
    i += 31;
    setbitu((unsigned char *)buff, i, 8, 0);
    i += 8;
    /* Reference Height, 23 bits */
    setbits((unsigned char *)buff, i, 23, NINT(mheight0 * 1e2));
    i += 23;
    /* number of satellites. uint6 */
    setbitu((unsigned char *)buff, i, 6, nsat);
    i += 6;

    /* step 4, KPL body at rtcm 3 message body */
    if (nsat > 0)
    {
        for (auto &kv : m_v)
        {
            if (kv.first[0] != SYS[isys])
                continue;

            int s = atoi(kv.first.c_str() + 1);
            if (!bbo_contains_list(union_brd, s))
                continue;
            /* number of PRN. uint6 */
            setbitu((unsigned char *)buff, i, 6, s);
            i += 6;
            /* GNSS Satellite Elevation Angle, uint10, 0.1 degree */
            setbitu((unsigned char *)buff, i, 10, 0);
            i += 10;
            /* STEC, int14, unit:0.05 TECU */
            setbits((unsigned char *)buff, i, 14, NINT(kv.second.first / 0.05));
            i += 14;
            /* Sigma, uint8, unit:0.05 TECU */
            setbitu((unsigned char *)buff, i, 8, NINT(kv.second.second / 0.05));
            i += 8;
            /* Reserved 2, 10 bits */
            setbitu((unsigned char *)buff, i, 10, 0);
            i += 10;
        }
    }

    /* step 5, padding to align 8 bit boundary */
    for (; i % 8; i++)
    {
        setbitu((unsigned char *)buff, i, 1, 0);
    }

    /* step 6, check length of header and body and update the length of body (bytes) */
    ilen = (i / 8);
    if (ilen >= 3 + 1024)
    {
        ilen = 0;
        free(buff);
        return NULL;
    }
    setbitu((unsigned char *)buff, 14, 10, (ilen - 3));

    /* step 7, crc */
    crc = rtk_crc24q((unsigned char *)buff, ilen);
    setbitu((unsigned char *)buff, i, 24, crc);
    i += 24;

    /* debug */
    if (ldebug)
    {
        if (!Logtrace::s_defaultlogger.m_lexist("debug_ion_grid_encode.out"))
            Logtrace::s_defaultlogger.m_openLog("debug_ion_grid_encode.out");
        Logtrace::s_defaultlogger.m_wtMsg("@%s %s\n", "debug_ion_grid_encode.out", "[SSR body begin]");
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_encode.out", "*RTCM Message Number:", 4056);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_encode.out", "*IGM/IM Version:", 0);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_encode.out", "*IGS Message Number:", submsgid);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_encode.out", "*Epoch:", epoch);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_encode.out", "*Date Type Identification:", 0);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_encode.out", "*STEC SSR IOD:", iod);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_encode.out", "*SSR Update Interval:", udint);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_encode.out", "*SSR Multiple Message Indicator:", sync);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.6f\n", "debug_ion_grid_encode.out", "*Grid Latitude:", NINT(mlat0 * 1e6) * 1e-6);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.6f\n", "debug_ion_grid_encode.out", "*Grid Longtitude:", NINT(mlon0 * 1e6) * 1e-6);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.6f\n", "debug_ion_grid_encode.out", "*Grid Height:", NINT(mheight0 * 1e2) * 1e-2);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_grid_encode.out", "*Number of satellite:", nsat);
        if (nsat > 0)
        {
            for (auto &kv : m_v)
            {
                if (kv.first[0] != SYS[isys])
                    continue;

                int s = atoi(kv.first.c_str() + 1);
                if (!bbo_contains_list(union_brd, s))
                    continue;
                Logtrace::s_defaultlogger.m_wtMsg("@%s %35s\n", "debug_ion_grid_encode.out", "----------");
                Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %3s%17.3f\n", "debug_ion_grid_encode.out", "*PRN:", kv.first.c_str(), NINT(kv.second.first / 0.05) * 0.05);
                Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.3f\n", "debug_ion_grid_encode.out", "*Sigma:", NINT(kv.second.second / 0.05) * 0.05);
            }
        }

        Logtrace::s_defaultlogger.m_wtMsg("@%s %s\n", "debug_ion_grid_encode.out", "[SSR body end]");
    }
    /* step 8, get the total length and output RTCM data */
    ilen = ilen + 3;
    char *buff_tosend = (char *)calloc(ilen, sizeof(char));
    memcpy(buff_tosend, buff, sizeof(char) * ilen);
    free(buff);
    return buff_tosend;
}
char *GridEncoder::s_encode_intern_send(char *buff_in, const int blen, const int msgid, int &ilen)
{
    ilen = 0;
    if (blen <= 0 || buff_in == NULL)
        return NULL;

    int i = 0, j;
    unsigned int crc, n_bsize = 8168;
    char *buff = (char *)calloc(n_bsize, sizeof(char));

    setbitu((unsigned char *)buff, i, 8, VRS_PREAMB);
    i += 8;
    setbitu((unsigned char *)buff, i, 16, 0);
    i += 16;
    setbitu((unsigned char *)buff, 24, 8, 104);
    i += 8;
    setbitu((unsigned char *)buff, i, 32, msgid);
    i += 32;
    setbitu((unsigned char *)buff, i, 32, 0);
    i += 32;
    setbitu((unsigned char *)buff, i, 32, 0);
    i += 32;
    setbitu((unsigned char *)buff, i, 32, 0);
    i += 32;
    setbitu((unsigned char *)buff, i, 32, 0);
    i += 32;
    setbitu((unsigned char *)buff, i, 16, 0);
    i += 16;
    for (j = 0; j < MAXSYS; ++j) /// set the number of corrections for each constellation
    {
        setbitu((unsigned char *)buff, i, 8, 0);
        i += 8;
    }
    setbitu((unsigned char *)buff, i, 16, 0);
    i += 16; // date complete

    /* the among buff (248 bit) and the input buff for user is already align 8 bit */
    memcpy(buff + (i / 8), buff_in, sizeof(char) * blen);
    i += blen * 8;

    /* padding to align 8 bit boundary */
    for (; i % 8; i++)
    {
        setbitu((unsigned char *)buff, i, 1, 0);
    }

    /* message length (header+data) (bytes) */
    if ((ilen = i / 8) > n_bsize - 4)
    {
        ilen = 0;
        free(buff);
        return NULL;
    }
    /* message length without header and parity */
    setbitu((unsigned char *)buff, 8, 16, ilen - 3);

    /* crc-24q */
    crc = rtk_crc24q((unsigned char *)buff, ilen);
    setbitu((unsigned char *)buff, i, 32, crc);
    i += 32;

    ilen = ilen + 4;
    char *buff_tosend = (char *)calloc(ilen, sizeof(char));
    memcpy(buff_tosend, buff, sizeof(char) * ilen);
    free(buff);
    return buff_tosend;
}
