/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-29 12:02:50
 * @ Modified by: Jun Tao
 * @ Modified time: 2024-08-23 19:47:19
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */

#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
#include "include/Applications/Encoder/SurfEncoder.h"
#include <cmath>
using namespace std;
using namespace iono;
/* SSR update intervals ------------------------------------------------------*/
static const double ssrudint[16] = {
    1, 2, 5, 10, 15, 30, 60, 120, 240, 300, 600, 900, 1800, 3600, 7200, 10800};
char *SurfEncoder::s_encode_slant_iono(std::map<std::string, polyCoefficient> &coef, const int isys, const int udint, const double mlat0, const double mlon0, const int orderlat, const int orderlon, gtime_t t0, int sync, int &ilen)
{
    int udi, iod, mjd, week, epoch;
    unsigned int crc, submsgid, n_bsize = 8168, i = 0, nsat = 0;
    double sod, tow;
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
            submsgid = 28;
            break;
        case 'R': /* GLONASS */
            submsgid = 48;
            break;
        case 'E': /* Galileo */
            submsgid = 68;
            break;
        case 'C': /* BDS-3 */
            submsgid = 109;
            break;
        case 'B': /* BDS-2 */
            submsgid = 108;
            break;
        case 'J': /* QZSS */
            submsgid = 88;
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
    for (const auto &kv : coef)
    {
        if (kv.first[0] != SYS[isys])
            continue;
        if (kv.second.coef.size() != (orderlat + 1) * (orderlon + 1))
            continue;
        bool lout = true;
        for (const auto &val : kv.second.coef)
        {
            if (std::fabs(val) > 1000)
            {
                lout = false;
                break;
            }
        }
        if (lout == false)
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

    /* step 3, rtcm 3 message body */
    /* RTCM Message Number, always 4076 for IGS SSR. 12 bits */
    setbitu((unsigned char *)buff, i, 12, 4076);
    i += 12;
    /* IGM/IM Version, set as 1. uint3 */
    setbitu((unsigned char *)buff, i, 3, 1);
    i += 3;
    /* IGS Message Number. uint8 */
    setbitu((unsigned char *)buff, i, 8, submsgid);
    i += 8;
    /* epoch time. uint20 */
    tow = time2gpst(t0, &week);
    epoch = ((int)floor((tow) + 0.5)) % 604800;
    setbitu((unsigned char *)buff, i, 20, epoch);
    i += 20;
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
    /* STEC SSR IOD. 0: NaN, 1-1440: Time span (60s) */
    sod = time2mjd(t0, &mjd);
    iod = (int)(sod / 60) + 1;
    setbitu((unsigned char *)buff, i, 11, iod);
    i += 11;
    /* number of satellites. uint4 */
    setbitu((unsigned char *)buff, i, 6, nsat);
    i += 6;
    /* Central latitude. int25 */
    setbits((unsigned char *)buff, i, 25, NINT(mlat0 * 1e5));
    i += 25;
    /* Central longtitude. int26 */
    setbits((unsigned char *)buff, i, 26, NINT(mlon0 * 1e5));
    i += 26;
    /* The maximum degree of latitude parameter of polynomial fitting. uint2 */
    setbitu((unsigned char *)buff, i, 2, orderlat);
    i += 2;
    /* The maximum degree of longtitude parameter of polynomial fitting. uint2 */
    setbitu((unsigned char *)buff, i, 2, orderlon);
    i += 2;

    if (nsat > 0)
    {
        for (const auto &kv : coef)
        {
            if (kv.first[0] != SYS[isys])
                continue;
            int s = atoi(kv.first.c_str() + 1);
            if (!bbo_contains_list(union_brd, s))
                continue;
            /* number of PRN. uint6 */
            setbitu((unsigned char *)buff, i, 6, std::stoi(kv.first.substr(1, 2).c_str()));
            i += 6;
            /* parameter of each order. bit(1), uint10 and uint17 */
            for (const auto &val : kv.second.coef)
            {
                if (val >= 0)
                    setbitu((unsigned char *)buff, i, 1, 1);
                else
                    setbitu((unsigned char *)buff, i, 1, 0);
                i += 1;
                setbitu((unsigned char *)buff, i, 10, (int)(fabs(val)));
                i += 10;
                setbitu((unsigned char *)buff, i, 17, NINT((fabs(val) - (int)(fabs(val))) * 1e5));
                i += 17;
            }
            /* sigma. uint9 */
            int sigma = int(kv.second.esig * 1e2);
            setbitu((unsigned char *)buff, i, 9, sigma);
            i += 9;
        }
    }

    /* step 4, padding to align 8 bit boundary */
    for (; i % 8; i++)
    {
        setbitu((unsigned char *)buff, i, 1, 0);
    }

    /* step 5, check length of header and body and update the length of body (bytes) */
    ilen = (i / 8);
    if (ilen >= 3 + 1024)
    {
        ilen = 0;
        free(buff);
        return NULL;
    }
    setbitu((unsigned char *)buff, 14, 10, (ilen - 3));

    /* step 6, crc */
    crc = rtk_crc24q((unsigned char *)buff, ilen);
    setbitu((unsigned char *)buff, i, 24, crc);
    i += 24;

    /* debug */
    if (ldebug)
    {
        if (!Logtrace::s_defaultlogger.m_lexist("debug_ion_encode.out"))
            Logtrace::s_defaultlogger.m_openLog("debug_ion_encode.out");
        Logtrace::s_defaultlogger.m_wtMsg("@%s %s\n", "debug_ion_encode.out", "[SSR body begin]");
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_encode.out", "*RTCM Message Number:", 4076);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_encode.out", "*IGM/IM Version:", 1);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_encode.out", "*IGS Message Number:", submsgid);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_encode.out", "*Epoch:", epoch);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_encode.out", "*SSR Update Interval:", udint);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_encode.out", "*SSR Multiple Message Indicator:", sync);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_encode.out", "*SSR STEC IOD:", iod);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.5f\n", "debug_ion_encode.out", "*Central Latitude:", mlat0);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.5f\n", "debug_ion_encode.out", "*Central Longtitude:", mlon0);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_encode.out", "*Order of Latitude:", orderlat);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_encode.out", "*Order of Longtitude:", orderlon);
        Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %d\n", "debug_ion_encode.out", "*Number of satellite:", nsat);
        for (const auto &kv : coef)
        {
            if (kv.first[0] == SYS[isys])
            {
                bool lout = false;
                if (kv.second.coef.size() != (orderlat + 1) * (orderlon + 1))
                    continue;
                for (const auto &val : kv.second.coef)
                {
                    if (std::fabs(val) > 1000)
                    {
                        lout = true;
                        break;
                    }
                }
                if (lout)
                    continue;

                Logtrace::s_defaultlogger.m_wtMsg("@%s %35s\n", "debug_ion_encode.out", "----------");
                Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %s\n", "debug_ion_encode.out", "*PRN:", kv.first.c_str());

                for (const auto &val : kv.second.coef)
                {
                    Logtrace::s_defaultlogger.m_wtMsg("@%s %20.5f\n", "debug_ion_encode.out", val);
                }
                Logtrace::s_defaultlogger.m_wtMsg("@%s %35s %20.2f\n", "debug_ion_encode.out", "*Sigma:", (int)(kv.second.esig * 1e2) * 1e-2);
            }
        }
        Logtrace::s_defaultlogger.m_wtMsg("@%s %s\n", "debug_ion_encode.out", "[SSR body end]");
    }
    /* step 7, get the total length and output RTCM data */
    ilen = ilen + 3;
    char *buff_tosend = (char *)calloc(ilen, sizeof(char));
    memcpy(buff_tosend, buff, sizeof(char) * ilen);
    free(buff);
    return buff_tosend;
}
