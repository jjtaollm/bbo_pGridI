/*
 * utils.cpp
 *
 *  Created on: 2018/2/4
 *      Author: juntao, at wuhan university
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <string>
#include <map>
#include <fstream>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include "include/Frame/Frame.h"
using namespace std;
namespace iono
{
#define MAXLEAPS 64                         /* max number of leap seconds table */
    static double leaps[MAXLEAPS + 1][7] = {/* leap seconds (y,m,d,h,m,s,utc-gpst) */
                                            {2017, 1, 1, 0, 0, 0, -18},
                                            {2015, 7, 1, 0, 0, 0, -17},
                                            {2012, 7, 1, 0, 0, 0, -16},
                                            {2009, 1, 1, 0, 0, 0, -15},
                                            {2006, 1, 1, 0, 0, 0, -14},
                                            {1999, 1, 1, 0, 0, 0, -13},
                                            {1997, 7, 1, 0, 0, 0, -12},
                                            {1996, 1, 1, 0, 0, 0, -11},
                                            {1994, 7, 1, 0, 0, 0, -10},
                                            {1993, 7, 1, 0, 0, 0, -9},
                                            {1992, 7, 1, 0, 0, 0, -8},
                                            {1991, 1, 1, 0, 0, 0, -7},
                                            {1990, 1, 1, 0, 0, 0, -6},
                                            {1988, 1, 1, 0, 0, 0, -5},
                                            {1985, 7, 1, 0, 0, 0, -4},
                                            {1983, 7, 1, 0, 0, 0, -3},
                                            {1982, 7, 1, 0, 0, 0, -2},
                                            {1981, 7, 1, 0, 0, 0, -1},
                                            {0}};
    static const uint32_t tbl_CRC24Q[] = {
        0x000000, 0x864CFB, 0x8AD50D, 0x0C99F6, 0x93E6E1, 0x15AA1A, 0x1933EC, 0x9F7F17,
        0xA18139, 0x27CDC2, 0x2B5434, 0xAD18CF, 0x3267D8, 0xB42B23, 0xB8B2D5, 0x3EFE2E,
        0xC54E89, 0x430272, 0x4F9B84, 0xC9D77F, 0x56A868, 0xD0E493, 0xDC7D65, 0x5A319E,
        0x64CFB0, 0xE2834B, 0xEE1ABD, 0x685646, 0xF72951, 0x7165AA, 0x7DFC5C, 0xFBB0A7,
        0x0CD1E9, 0x8A9D12, 0x8604E4, 0x00481F, 0x9F3708, 0x197BF3, 0x15E205, 0x93AEFE,
        0xAD50D0, 0x2B1C2B, 0x2785DD, 0xA1C926, 0x3EB631, 0xB8FACA, 0xB4633C, 0x322FC7,
        0xC99F60, 0x4FD39B, 0x434A6D, 0xC50696, 0x5A7981, 0xDC357A, 0xD0AC8C, 0x56E077,
        0x681E59, 0xEE52A2, 0xE2CB54, 0x6487AF, 0xFBF8B8, 0x7DB443, 0x712DB5, 0xF7614E,
        0x19A3D2, 0x9FEF29, 0x9376DF, 0x153A24, 0x8A4533, 0x0C09C8, 0x00903E, 0x86DCC5,
        0xB822EB, 0x3E6E10, 0x32F7E6, 0xB4BB1D, 0x2BC40A, 0xAD88F1, 0xA11107, 0x275DFC,
        0xDCED5B, 0x5AA1A0, 0x563856, 0xD074AD, 0x4F0BBA, 0xC94741, 0xC5DEB7, 0x43924C,
        0x7D6C62, 0xFB2099, 0xF7B96F, 0x71F594, 0xEE8A83, 0x68C678, 0x645F8E, 0xE21375,
        0x15723B, 0x933EC0, 0x9FA736, 0x19EBCD, 0x8694DA, 0x00D821, 0x0C41D7, 0x8A0D2C,
        0xB4F302, 0x32BFF9, 0x3E260F, 0xB86AF4, 0x2715E3, 0xA15918, 0xADC0EE, 0x2B8C15,
        0xD03CB2, 0x567049, 0x5AE9BF, 0xDCA544, 0x43DA53, 0xC596A8, 0xC90F5E, 0x4F43A5,
        0x71BD8B, 0xF7F170, 0xFB6886, 0x7D247D, 0xE25B6A, 0x641791, 0x688E67, 0xEEC29C,
        0x3347A4, 0xB50B5F, 0xB992A9, 0x3FDE52, 0xA0A145, 0x26EDBE, 0x2A7448, 0xAC38B3,
        0x92C69D, 0x148A66, 0x181390, 0x9E5F6B, 0x01207C, 0x876C87, 0x8BF571, 0x0DB98A,
        0xF6092D, 0x7045D6, 0x7CDC20, 0xFA90DB, 0x65EFCC, 0xE3A337, 0xEF3AC1, 0x69763A,
        0x578814, 0xD1C4EF, 0xDD5D19, 0x5B11E2, 0xC46EF5, 0x42220E, 0x4EBBF8, 0xC8F703,
        0x3F964D, 0xB9DAB6, 0xB54340, 0x330FBB, 0xAC70AC, 0x2A3C57, 0x26A5A1, 0xA0E95A,
        0x9E1774, 0x185B8F, 0x14C279, 0x928E82, 0x0DF195, 0x8BBD6E, 0x872498, 0x016863,
        0xFAD8C4, 0x7C943F, 0x700DC9, 0xF64132, 0x693E25, 0xEF72DE, 0xE3EB28, 0x65A7D3,
        0x5B59FD, 0xDD1506, 0xD18CF0, 0x57C00B, 0xC8BF1C, 0x4EF3E7, 0x426A11, 0xC426EA,
        0x2AE476, 0xACA88D, 0xA0317B, 0x267D80, 0xB90297, 0x3F4E6C, 0x33D79A, 0xB59B61,
        0x8B654F, 0x0D29B4, 0x01B042, 0x87FCB9, 0x1883AE, 0x9ECF55, 0x9256A3, 0x141A58,
        0xEFAAFF, 0x69E604, 0x657FF2, 0xE33309, 0x7C4C1E, 0xFA00E5, 0xF69913, 0x70D5E8,
        0x4E2BC6, 0xC8673D, 0xC4FECB, 0x42B230, 0xDDCD27, 0x5B81DC, 0x57182A, 0xD154D1,
        0x26359F, 0xA07964, 0xACE092, 0x2AAC69, 0xB5D37E, 0x339F85, 0x3F0673, 0xB94A88,
        0x87B4A6, 0x01F85D, 0x0D61AB, 0x8B2D50, 0x145247, 0x921EBC, 0x9E874A, 0x18CBB1,
        0xE37B16, 0x6537ED, 0x69AE1B, 0xEFE2E0, 0x709DF7, 0xF6D10C, 0xFA48FA, 0x7C0401,
        0x42FA2F, 0xC4B6D4, 0xC82F22, 0x4E63D9, 0xD11CCE, 0x575035, 0x5BC9C3, 0xDD8538};
    /* function prototypes -------------------------------------------------------*/
    static const double gpst0[] = {1980, 1, 6, 0, 0, 0}; /* gps time reference */
    static const double gst0[] = {1999, 8, 22, 0, 0, 0}; /* galileo system time reference */
    static const double bdt0[] = {2006, 1, 1, 0, 0, 0};  /* beidou time reference */

    /* get tick time ---------------------------------------------------------------
     * get current tick in ms
     * args   : none
     * return : current tick in ms
     *-----------------------------------------------------------------------------*/
    extern uint32_t tickget(void)
    {
#ifdef WIN32
        return (uint32_t)timeGetTime();
#else
        struct timespec tp = {0};
        struct timeval tv = {0};

#ifdef CLOCK_MONOTONIC_RAW
        /* linux kernel > 2.6.28 */
        if (!clock_gettime(CLOCK_MONOTONIC_RAW, &tp))
        {
            return tp.tv_sec * 1000u + tp.tv_nsec / 1000000u;
        }
        else
        {
            gettimeofday(&tv, NULL);
            return tv.tv_sec * 1000u + tv.tv_usec / 1000u;
        }
#else
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000u + tv.tv_usec / 1000u;
#endif
#endif /* WIN32 */
    }
    /* time to string --------------------------------------------------------------
     * convert gtime_t struct to string
     * args   : gtime_t t        I   gtime_t struct
     *          char   *s        O   string ("yyyy/mm/dd hh:mm:ss.ssss")
     *          int    n         I   number of decimals
     * return : none
     *-----------------------------------------------------------------------------*/
    extern void time2str(gtime_t t, char *s, int n)
    {
        double ep[6];

        if (n < 0)
            n = 0;
        else if (n > 12)
            n = 12;
        if (1.0 - t.sec < 0.5 / pow(10.0, n))
        {
            t.time++;
            t.sec = 0.0;
        };
        time2epoch(t, ep);
        sprintf(s, "%04.0f %02.0f %02.0f %02.0f %02.0f %0*.*f", ep[0], ep[1], ep[2],
                ep[3], ep[4], n <= 0 ? 2 : n + 3, n <= 0 ? 0 : n, ep[5]);
    }
    extern char *time2str(time_t t_i, char *s, int n)
    {
        double ep[6];
        gtime_t t = {0};
        t.time = t_i;
        if (n < 0)
            n = 0;
        else if (n > 12)
            n = 12;
        if (1.0 - t.sec < 0.5 / pow(10.0, n))
        {
            t.time++;
            t.sec = 0.0;
        };
        time2epoch(t, ep);
        sprintf(s, "%04.0f/%02.0f/%02.0f %02.0f:%02.0f:%0*.*f", ep[0], ep[1], ep[2],
                ep[3], ep[4], n <= 0 ? 2 : n + 3, n <= 0 ? 0 : n, ep[5]);
        return s;
    }
    char *run_timefmt(time_t tt, char *buf)
    {
        gtime_t gt = {0};
        double ep[6] = {0};
        gt.time = tt;
        time2epoch(gt, ep);
        if (buf != NULL)
            sprintf(buf, "%02d-%02d-%02d/%02d:%02d:%02d", (int)ep[0], (int)ep[1], (int)ep[2], (int)ep[3], (int)ep[4], (int)ep[5]);
        return buf;
    }
    char *run_timefmt(int mjd, double sod, char *buf)
    {
        gtime_t tt = mjd2time(mjd, sod);
        return run_timefmt(tt.time, buf);
    }
    /* convert calendar day/time to time -------------------------------------------
     * convert calendar day/time to gtime_t struct
     * args   : double *ep       I   day/time {year,month,day,hour,min,sec}
     * return : gtime_t struct
     * notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
     *-----------------------------------------------------------------------------*/
    extern gtime_t epoch2time(const double *ep)
    {
        const int doy[] = {1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
        gtime_t time = {0};
        int days, sec, year = (int)ep[0], mon = (int)ep[1], day = (int)ep[2];

        if (year < 1970 || 2099 < year || mon < 1 || 12 < mon)
            return time;

        /* leap year if year%4==0 in 1901-2099 */
        days = (year - 1970) * 365 + (year - 1969) / 4 + doy[mon - 1] + day - 2 + (year % 4 == 0 && mon >= 3 ? 1 : 0);
        sec = (int)floor(ep[5]);
        time.time = (time_t)days * 86400 + (int)ep[3] * 3600 + (int)ep[4] * 60 + sec;
        time.sec = ep[5] - sec;
        return time;
    }

    /* time to calendar day/time ---------------------------------------------------
     * convert gtime_t struct to calendar day/time
     * args   : gtime_t t        I   gtime_t struct
     *          double *ep       O   day/time {year,month,day,hour,min,sec}
     * return : none
     * notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
     *-----------------------------------------------------------------------------*/
    extern void time2epoch(gtime_t t, double *ep)
    {
        const int mday[] = {/* # of days in a month */
                            31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
                            31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int days, sec, mon, day;

        /* leap year if year%4==0 in 1901-2099 */
        days = (int)(t.time / 86400);
        sec = (int)(t.time - (time_t)days * 86400);
        for (day = days % 1461, mon = 0; mon < 48; mon++)
        {
            if (day >= mday[mon])
                day -= mday[mon];
            else
                break;
        }
        ep[0] = 1970 + days / 1461 * 4 + mon / 12;
        ep[1] = mon % 12 + 1;
        ep[2] = day + 1;
        ep[3] = sec / 3600;
        ep[4] = sec % 3600 / 60;
        ep[5] = sec % 60 + t.sec;
    }

    /* same as above but output limited to n decimals for formatted output */
    extern void time2epoch_n(gtime_t t, double *ep, int n)
    {
        if (n < 0)
            n = 0;
        else if (n > 12)
            n = 12;
        if (1.0 - t.sec < 0.5 / pow(10.0, n))
        {
            t.time++;
            t.sec = 0.0;
        };
        time2epoch(t, ep);
    }

    /* gps time to time ------------------------------------------------------------
     * convert week and tow in gps time to gtime_t struct
     * args   : int    week      I   week number in gps time
     *          double sec       I   time of week in gps time (s)
     * return : gtime_t struct
     *-----------------------------------------------------------------------------*/
    extern gtime_t gpst2time(int week, double sec)
    {
        gtime_t t = epoch2time(gpst0);

        if (sec < -1E9 || 1E9 < sec)
            sec = 0.0;
        t.time += (time_t)86400 * 7 * week + (int)sec;
        t.sec = sec - (int)sec;
        return t;
    }

    /* time to gps time ------------------------------------------------------------
     * convert gtime_t struct to week and tow in gps time
     * args   : gtime_t t        I   gtime_t struct
     *          int    *week     IO  week number in gps time (NULL: no output)
     * return : time of week in gps time (s)
     *-----------------------------------------------------------------------------*/
    extern double time2gpst(gtime_t t, int *week)
    {
        gtime_t t0 = epoch2time(gpst0);
        time_t sec = t.time - t0.time;
        int w = (int)(sec / (86400 * 7));

        if (week)
            *week = w;
        return (double)(sec - (double)w * 86400 * 7) + t.sec;
    }

    /* time to string ------------------------------------------------------------
     * convert current to string
     * args   : buff   empty
     * return : buff   filled with formatted string
     *-----------------------------------------------------------------------------*/
    char *runlocaltime(char *buf)
    {
        struct tm *ptr;
        time_t rawtime;
        time(&rawtime);
        ptr = localtime(&rawtime);
        double ep[6] = {ptr->tm_year + 1900.0, (double)ptr->tm_mon + 1, (double)ptr->tm_mday, (double)ptr->tm_hour, (double)ptr->tm_min, (double)ptr->tm_sec};
        gtime_t t = epoch2time(ep);
        time2str(gpst2utc(t), buf, 1);
        return buf;
    }

    /* mjd to gtime ------------------------------------------------------------
     * convert mjd to gtime
     * args   : mjd   I int
     *          sod   I double
     * return : gtime_t
     *-----------------------------------------------------------------------------*/
    gtime_t mjd2time(int mjd, double sod)
    {
        gtime_t time = {0};
        int wk;
        double sow;
        sow = mjd2wksow(mjd, sod, &wk, NULL);
        time = gpst2time(wk, sow);
        return time;
    }
    /* gpstime to utc --------------------------------------------------------------
     * convert gpstime to utc considering leap seconds
     * args   : gtime_t t        I   time expressed in gpstime
     * return : time expressed in utc
     * notes  : ignore slight time offset under 100 ns
     *-----------------------------------------------------------------------------*/
    extern gtime_t gpst2utc(gtime_t t)
    {
        gtime_t tu;
        int i;

        for (i = 0; leaps[i][0] > 0; i++)
        {
            tu = timeadd(t, leaps[i][6]);
            if (timediff(tu, epoch2time(leaps[i])) >= 0.0)
                return tu;
        }
        return t;
    }
    /* utc to gpstime --------------------------------------------------------------
     * convert utc to gpstime considering leap seconds
     * args   : gtime_t t        I   time expressed in utc
     * return : time expressed in gpstime
     * notes  : ignore slight time offset under 100 ns
     *-----------------------------------------------------------------------------*/
    extern gtime_t utc2gpst(gtime_t t)
    {
        int i;

        for (i = 0; leaps[i][0] > 0; i++)
        {
            if (timediff(t, epoch2time(leaps[i])) >= 0.0)
                return timeadd(t, -leaps[i][6]);
        }
        return t;
    }
    /* mjd to week and sow ------------------------------------------------------------
     * convert mjd to gtime
     * args   : mjd   I int
     *          sod   I double
     *          week  O int
     *          sow   O double
     * return : sow
     *-----------------------------------------------------------------------------*/
    double mjd2wksow(int mjd, double sod, int *week, double *sow)
    {
        double wk, sw;
        wk = (int)((mjd + sod / 86400.0 - 44244.0) / 7.0);
        sw = (mjd - 44244.0 - wk * 7) * 86400.0 + sod;
        if (week)
            *week = wk;
        if (sow)
            *sow = sw;
        return sw;
    }

    void matmpy(double *A, double *B, double *C, int row, int colA, int colB)
    {
        int i, j, k;
        double value;
        double *dest = (double *)calloc(row * colB, sizeof(double));
        for (i = 0; i < row; i++)
        {
            for (j = 0; j < colB; j++)
            {
                value = 0;
                for (k = 0; k < colA; k++)
                {
                    value += A[i * colA + k] * B[k * colB + j];
                }
                dest[i * colB + j] = value;
            }
        }
        for (i = 0; i < row; i++)
        {
            for (j = 0; j < colB; j++)
            {
                C[i * colB + j] = dest[i * colB + j];
            }
        }
        free(dest);
    }

    void wksow2mjd(int week, double sow, int *mjd, double *sod)
    {
        if (mjd != NULL)
            *mjd = (int)(sow / 86400.0) + week * 7 + 44244;
        if (sod != NULL)
            *sod = fmod(sow, 86400.0);
    }

    /* time to mjd time ------------------------------------------------------------
     * convert gtime_t struct to mjd in double
     * args   : gtime_t t        I   gtime_t struct
     * return : mjd
     *-----------------------------------------------------------------------------*/
    extern double time2mjd(time_t tt, int *mjd)
    {
        double ep[6];
        gtime_t t = {0};
        t.time = tt;
        time2epoch(t, ep);
        if (mjd)
            *mjd = ymd2mjd((int)ep[0], (int)ep[1], (int)ep[2]);
        return (ep[3] * 3600.0 + ep[4] * 60.0 + ep[5]);
    }

    /* time to mjd time ------------------------------------------------------------
     * convert gtime_t struct to mjd in double
     * args   : gtime_t t        I   gtime_t struct
     * return : mjd
     *-----------------------------------------------------------------------------*/
    extern double time2mjd(gtime_t t, int *mjd)
    {
        double ep[6];
        time2epoch(t, ep);
        if (mjd)
            *mjd = ymd2mjd((int)ep[0], (int)ep[1], (int)ep[2]);
        return (ep[3] * 3600.0 + ep[4] * 60.0 + ep[5]);
    }

    /* year,mon,day to mjd time ------------------------------------------------------------
     * convert year,mon,day to mjd time
     * args   : iyear    I int
     *          imonth   I int
     *          iday     I int
     * return : mjd
     *-----------------------------------------------------------------------------*/
    extern int ymd2mjd(int iyear, int imonth, int iday)
    {
        int iyr, result;
        int doy_of_month[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304,
                                334};
        if (iyear < 0 || imonth < 0 || iday < 0 || imonth > 12 || iday > 366 || (imonth != 0 && iday > 31))
        {
            printf("ymd2mjd, incorrect argument %d %d %d\n", iyear, imonth, iday);
            exit(1);
        }
        iyr = iyear;
        if (imonth <= 2)
            iyr -= 1;
        result = 365 * iyear - 678941 + iyr / 4 - iyr / 100 + iyr / 400 + iday;
        if (imonth != 0)
            result = result + doy_of_month[imonth - 1];
        return result;
    }
    /* mjd 2 year doy  ------------------------------------------------------------
     * convert year,mon,day to mjd time
     * args   : mjd    I int
     *          iyear  O int
     * return : doy
     *-----------------------------------------------------------------------------*/
    extern int mjd2doy(int jd, int *iyear)
    {
        int iy, idoy;
        iy = (jd + 678940) / 365;
        idoy = jd - ymd2mjd(iy, 1, 1);
        while (idoy <= 0)
        {
            iy--;
            idoy = jd - ymd2mjd(iy, 1, 1) + 1;
        }
        if (iyear)
            *iyear = iy;
        return idoy;
    }
    /* extract unsigned/signed bits ------------------------------------------------
     * extract unsigned/signed bits from byte data
     * args   : uint8_t *buff    I   byte data
     *          int    pos       I   bit position from start of data (bits)
     *          int    len       I   bit length (bits) (len<=32)
     * return : extracted unsigned/signed bits
     *-----------------------------------------------------------------------------*/
    extern uint32_t getbitu(const uint8_t *buff, int pos, int len)
    {
        uint32_t bits = 0;
        int i;
        for (i = pos; i < pos + len; i++)
            bits = (bits << 1) + ((buff[i / 8] >> (7 - i % 8)) & 1u);
        return bits;
    }
    extern int32_t getbits(const uint8_t *buff, int pos, int len)
    {
        uint32_t bits = getbitu(buff, pos, len);
        if (len <= 0 || 32 <= len || !(bits & (1u << (len - 1))))
            return (int32_t)bits;
        return (int32_t)(bits | (~0u << len)); /* extend sign */
    }

    /* set unsigned/signed bits ----------------------------------------------------
     * set unsigned/signed bits to byte data
     * args   : uint8_t *buff IO byte data
     *          int    pos       I   bit position from start of data (bits)
     *          int    len       I   bit length (bits) (len<=32)
     *          [u]int32_t data  I   unsigned/signed data
     * return : none
     *-----------------------------------------------------------------------------*/
    extern void setbitu(uint8_t *buff, int pos, int len, uint32_t data)
    {
        uint32_t mask = 1u << (len - 1);
        int i;
        if (len <= 0 || 32 < len)
            return;
        for (i = pos; i < pos + len; i++, mask >>= 1)
        {
            if (data & mask)
                buff[i / 8] |= 1u << (7 - i % 8);
            else
                buff[i / 8] &= ~(1u << (7 - i % 8));
        }
    }
    extern void setbits(uint8_t *buff, int pos, int len, int32_t data)
    {
        if (data < 0)
            data |= 1 << (len - 1);
        else
            data &= ~(1 << (len - 1)); /* set sign bit */
        setbitu(buff, pos, len, (uint32_t)data);
    }

    /* crc-24q parity --------------------------------------------------------------
     * compute crc-24q parity for sbas, rtcm3
     * args   : uint8_t *buff    I   data
     *          int    len       I   data length (bytes)
     * return : crc-24Q parity
     * notes  : see reference [2] A.4.3.3 Parity
     *-----------------------------------------------------------------------------*/
    extern uint32_t rtk_crc24q(const uint8_t *buff, int len)
    {
        uint32_t crc = 0;
        int i;

        for (i = 0; i < len; i++)
            crc = ((crc << 8) & 0xFFFFFF) ^ tbl_CRC24Q[(crc >> 16) ^ buff[i]];
        return crc;
    }

    /* sleep ms --------------------------------------------------------------------
     * sleep ms
     * args   : int   ms         I   miliseconds to sleep (<0:no sleep)
     * return : none
     *-----------------------------------------------------------------------------*/
    extern void sleepms(int ms)
    {
#ifdef WIN32
        if (ms < 5)
            Sleep(1);
        else
            Sleep(ms);
#else
        struct timespec ts;
        if (ms <= 0)
            return;
        ts.tv_sec = (time_t)(ms / 1000);
        ts.tv_nsec = (long)(ms % 1000 * 1000000);
        nanosleep(&ts, NULL);
#endif
    }

    void timinc(int jd, double sec, double delt, int *jd1, double *sec1)
    {
        *sec1 = sec + delt;
        int inc = (int)(*sec1 / 86400.0);
        *jd1 = jd + inc;
        *sec1 = *sec1 - inc * 86400.0;
        if (*sec1 >= 0)
            return;
        *jd1 = *jd1 - 1;
        *sec1 = *sec1 + 86400;
    }

    /* beidou time (bdt) to time ---------------------------------------------------
     * convert week and tow in beidou time (bdt) to gtime_t struct
     * args   : int    week      I   week number in bdt
     *          double sec       I   time of week in bdt (s)
     * return : gtime_t struct
     *-----------------------------------------------------------------------------*/
    extern gtime_t bdt2time(int week, double sec)
    {
        gtime_t t = epoch2time(bdt0);

        if (sec < -1E9 || 1E9 < sec)
            sec = 0.0;
        t.time += (time_t)86400 * 7 * week + (int)sec;
        t.sec = sec - (int)sec;
        return t;
    }
    /* time to beidouo time (bdt) --------------------------------------------------
     * convert gtime_t struct to week and tow in beidou time (bdt)
     * args   : gtime_t t        I   gtime_t struct
     *          int    *week     IO  week number in bdt (NULL: no output)
     * return : time of week in bdt (s)
     *-----------------------------------------------------------------------------*/
    extern double time2bdt(gtime_t t, int *week)
    {
        gtime_t t0 = epoch2time(bdt0);
        time_t sec = t.time - t0.time;
        int w = (int)(sec / (86400 * 7));

        if (week)
            *week = w;
        return (double)(sec - (double)w * 86400 * 7) + t.sec;
    }
    extern gtime_t timeget(void)
    {
        gtime_t time;
        double ep[6] = {0};
#ifdef WIN32
        SYSTEMTIME ts;

        GetSystemTime(&ts); /* utc */
        ep[0] = ts.wYear;
        ep[1] = ts.wMonth;
        ep[2] = ts.wDay;
        ep[3] = ts.wHour;
        ep[4] = ts.wMinute;
        ep[5] = ts.wSecond + ts.wMilliseconds * 1E-3;
#else
        struct timeval tv;
        struct tm *tt;

        if (!gettimeofday(&tv, NULL) && (tt = gmtime(&tv.tv_sec)))
        {
            ep[0] = tt->tm_year + 1900;
            ep[1] = tt->tm_mon + 1;
            ep[2] = tt->tm_mday;
            ep[3] = tt->tm_hour;
            ep[4] = tt->tm_min;
            ep[5] = tt->tm_sec + tv.tv_usec * 1E-6;
        }
#endif
        time = epoch2time(ep);

#ifdef CPUTIME_IN_GPST /* cputime operated in gpst */
        time = gpst2utc(time);
#endif
        return time;
    }

    /* add time --------------------------------------------------------------------
     * add time to gtime_t struct
     * args   : gtime_t t        I   gtime_t struct
     *          double sec       I   time to add (s)
     * return : gtime_t struct (t+sec)
     *-----------------------------------------------------------------------------*/
    extern gtime_t timeadd(gtime_t t, double sec)
    {
        double tt;

        t.sec += sec;
        tt = floor(t.sec);
        t.time += (time_t)tt; // by zzy
        t.sec -= tt;
        return t;
    }
    /* time difference -------------------------------------------------------------
     * difference between gtime_t structs
     * args   : gtime_t t1,t2    I   gtime_t structs
     * return : time difference (t1-t2) (s)
     *-----------------------------------------------------------------------------*/
    extern double timediff(gtime_t t1, gtime_t t2)
    {
        return difftime(t1.time, t2.time) + t1.sec - t2.sec;
    }

    /* inner product ---------------------------------------------------------------
     * inner product of vectors
     * args   : double *a,*b     I   vector a,b (n x 1)
     *          int    n         I   size of vector a,b
     * return : a'*b
     *-----------------------------------------------------------------------------*/
    extern double dot(const double *a, const double *b, int n)
    {
        double c = 0.0;

        while (--n >= 0)
            c += a[n] * b[n];
        return c;
    }

    /* transform ecef to geodetic postion ------------------------------------------
     * transform ecef position to geodetic position
     * args   : double *r        I   ecef position {x,y,z} (m)
     *          double *pos      O   geodetic position {lat,lon,h} (rad,m)
     * return : none
     * notes  : WGS84, ellipsoidal height
     *-----------------------------------------------------------------------------*/
    extern void ecef2pos(const double *r, double *pos)
    {
        double e2 = FE_WGS84 * (2.0 - FE_WGS84), r2 = dot(r, r, 2), z, zk, v = RE_WGS84, sinp;

        for (z = r[2], zk = 0.0; fabs(z - zk) >= 1E-4;)
        {
            zk = z;
            sinp = z / sqrt(r2 + z * z);
            v = RE_WGS84 / sqrt(1.0 - e2 * sinp * sinp);
            z = r[2] + v * e2 * sinp;
        }
        pos[0] = r2 > 1E-12 ? atan(z / sqrt(r2)) : (r[2] > 0.0 ? PI / 2.0 : -PI / 2.0);
        pos[1] = r2 > 1E-12 ? atan2(r[1], r[0]) : 0.0;
        pos[2] = sqrt(r2 + z * z) - v;
    }

    /* transform geodetic to ecef position -----------------------------------------
     * transform geodetic position to ecef position
     * args   : double *pos      I   geodetic position {lat,lon,h} (rad,m)
     *          double *r        O   ecef position {x,y,z} (m)
     * return : none
     * notes  : WGS84, ellipsoidal height
     *-----------------------------------------------------------------------------*/
    extern void pos2ecef(const double *pos, double *r)
    {
        double sinp = sin(pos[0]), cosp = cos(pos[0]), sinl = sin(pos[1]), cosl = cos(pos[1]);
        double e2 = FE_WGS84 * (2.0 - FE_WGS84), v = RE_WGS84 / sqrt(1.0 - e2 * sinp * sinp);

        r[0] = (v + pos[2]) * cosp * cosl;
        r[1] = (v + pos[2]) * cosp * sinl;
        r[2] = (v * (1.0 - e2) + pos[2]) * sinp;
    }

    /* ecef to local coordinate transfromation matrix ------------------------------
     * compute ecef to local coordinate transfromation matrix
     * args   : double *pos      I   geodetic position {lat,lon} (rad)
     *          double *E        O   ecef to local coord transformation matrix (3x3)
     * return : none
     * notes  : matirix stored by column-major order (fortran convention)
     *-----------------------------------------------------------------------------*/
    extern void xyz2enu(const double *pos, double *E)
    {
        double sinp = sin(pos[0]), cosp = cos(pos[0]), sinl = sin(pos[1]), cosl = cos(pos[1]);

        E[0] = -sinl;
        E[3] = cosl;
        E[6] = 0.0;
        E[1] = -sinp * cosl;
        E[4] = -sinp * sinl;
        E[7] = cosp;
        E[2] = cosp * cosl;
        E[5] = cosp * sinl;
        E[8] = sinp;
    }

    /* transform ecef vector to local tangental coordinate -------------------------
     * transform ecef vector to local tangental coordinate
     * args   : double *pos      I   geodetic position {lat,lon} (rad)
     *          double *r        I   vector in ecef coordinate {x,y,z}
     *          double *e        O   vector in local tangental coordinate {e,n,u}
     * return : none
     *-----------------------------------------------------------------------------*/
    extern void ecef2enu(const double *pos, const double *r, double *e)
    {
        double E[9];

        xyz2enu(pos, E);
        CMat::matmul("NN", 3, 1, 3, 1.0, E, r, 0.0, e);
    }

    /* transform local vector to ecef coordinate -----------------------------------
     * transform local tangental coordinate vector to ecef
     * args   : double *pos      I   geodetic position {lat,lon} (rad)
     *          double *e        I   vector in local tangental coordinate {e,n,u}
     *          double *r        O   vector in ecef coordinate {x,y,z}
     * return : none
     *-----------------------------------------------------------------------------*/
    extern void enu2ecef(const double *pos, const double *e, double *r)
    {
        double E[9];

        xyz2enu(pos, E);
        CMat::matmul("TN", 3, 1, 3, 1.0, E, e, 0.0, r);
    }

    /// @brief  string functions
    /// @param cnt
    /// @param string_array
    /// @param str
    /// @return
    int pointer_string(int cnt, string string_array[], string str)
    {
        int itr, idx = -1;
        for (itr = 0; itr < cnt; itr++)
        {
            if (string_array[itr] == str)
            {
                idx = itr;
                break;
            }
        }
        return idx;
    }
    /**********************CHANGED*****************************/
    int pointer_charstr(int row, int col, const char *string_array, const char *string)
    {
        int i;
        const char *pStr = (const char *)string_array;
        for (i = 0; i < row; i++)
        {
            if (strncmp(pStr + i * col, string, strlen(string)) == 0)
                break;
        }
        if (i == row)
            i = -1;
        return i;
    }
    int index_string(const char *src, char key)
    {
        int len = strlen(src);
        int i;
        for (i = 0; i < len; i++)
        {
            if (src[i] == key)
                break;
        }
        if (i == len)
            return -1;
        else
            return i;
    }

    double freqbysys(int isys, int ifreq, const char *freq)
    {
        switch (SYS[isys])
        {
        case 'G':
            if (strstr(freq, "L1") != NULL)
                return GPS_L1;
            else if (strstr(freq, "L2") != NULL)
                return GPS_L2;
            else if (strstr(freq, "L5") != NULL)
                return GPS_L5;
            else
            {
                LOGPRINT("freq = %s,unknow frequency for GPS", freq);
                exit(1);
            }
            break;
        case 'B':
        case 'C':
            if (strstr(freq, "L1") != NULL)
                return BDS_B1c;
            else if (strstr(freq, "L2") != NULL)
                return BDS_B1;
            else if (strstr(freq, "L7") != NULL)
                return BDS_B2b;
            else if (strstr(freq, "L6") != NULL)
                return BDS_B3;
            else if (strstr(freq, "L5") != NULL)
                return BDS_B2a;
            else if (strstr(freq, "L8") != NULL)
                return BDS_B2;
            else
            {
                LOGPRINT("freq = %s,unknow frequency for BDS", freq);
                exit(1);
            }
            break;
        case 'R':
            if (strstr(freq, "L1") != NULL)
                return GLS_L1 + ifreq * GLS_dL1;
            else if (strstr(freq, "L2") != NULL)
                return GLS_L2 + ifreq * GLS_dL2;
            else
            {
                LOGPRINT("freq = %s,unknow frequency for GLONASS", freq);
                exit(1);
            }
            break;
        case 'E':
            if (strstr(freq, "L1") != NULL)
                return GAL_E1;
            else if (strstr(freq, "L8") != NULL)
                return GAL_E5;
            else if (strstr(freq, "L6") != NULL)
                return GAL_E6;
            else if (strstr(freq, "L5") != NULL)
                return GAL_E5a;
            else if (strstr(freq, "L7") != NULL)
                return GAL_E5b;
            else
            {
                LOGPRINT("freq = %s,unknow frequency for GALILEO", freq);
                exit(1);
            }
            break;
        case 'J':
            if (strstr(freq, "L1") != NULL)
                return QZS_L1;
            else if (strstr(freq, "L2") != NULL)
                return QZS_L2;
            else if (strstr(freq, "L5") != NULL)
                return QZS_L5;
            else if (strstr(freq, "L6") != NULL)
                return QZS_LEX;
            else
            {
                LOGPRINT("freq = %s,unknow frequency for QZSS", freq);
                exit(1);
            }
            break;
        default:
            LOGPRINT("type = %s,unknow observation type", freq);
            break;
        }
        return 0;
    }
    int read_siteinfo(string fil, string name, double *x)
    {
        int lfound;
        string line;
        map<string, string>::iterator itr;
        ifstream in;
        /// gnss.cfg part

        in.open(fil, ios::in);
        if (!in.is_open())
        {
            LOGPRINT("file = %s,can't access site file", (*itr).second.c_str());
            return 0;
        }

        lfound = 0;
        while (getline(in, line))
        {
            if (strstr(line.c_str(), (name + "_POS").c_str()))
            {
                lfound = 1;
                break;
            }
        }
        if (!lfound)
        {
            LOGPRINT("staname = %s,file = %s,station is not exist in the site file", name.c_str(), (*itr).second.c_str());
            in.close();
            return 0;
        }
        sscanf(line.c_str(), "%*s%lf%lf%lf", x, x + 1, x + 2);
        in.close();
        if (x[0] == 0 || x[1] == 0 || x[2] == 0)
        {
            LOGPRINT("staname = %s,file = %s,no coordinate information for station in site file", name.c_str(),
                     (*itr).second.c_str());
        }
        return 1;
    }

    long double GetX(double RadLat, double eSquare, double a)
    {
        long double X;
        long double A0;
        long double A2;
        long double A4;
        long double A6;
        long double A8; ///*  from hsq
        A0 = 1 + 3.0 / 4.0 * eSquare + 45.0 / 64.0 * eSquare * eSquare + 350.0 / 512.0 * eSquare * eSquare * eSquare + 11025.0 / 16384.0 * eSquare * eSquare * eSquare * eSquare;
        A2 =
            -1.0 / 2.0 * (3.0 / 4.0 * eSquare + 60.0 / 64.0 * eSquare * eSquare + 525.0 / 512.0 * eSquare * eSquare * eSquare + 17640.0 / 16384.0 * eSquare * eSquare * eSquare * eSquare);
        A4 = 1.0 / 4.0 * (15.0 / 64.0 * eSquare * eSquare + 210.0 / 512.0 * eSquare * eSquare * eSquare + 8820.0 / 16384.0 * eSquare * eSquare * eSquare * eSquare);
        A6 = -1.0 / 6.0 * (35.0 / 512.0 * eSquare * eSquare * eSquare + 2520.0 / 16384.0 * eSquare * eSquare * eSquare * eSquare);
        A8 = 1.0 / 8.0 * 315.0 / 16384.0 * eSquare * eSquare * eSquare * eSquare;
        X = a * (1 - eSquare) * (A0 * RadLat + A2 * sin(2 * RadLat) + A4 * sin(4 * RadLat) + A6 * sin(6 * RadLat) + A8 * sin(8 * RadLat));
        return X;
    }
    bool bl2Gaussxy(double RadLat, double RadLon, double *x, double *y,
                    double RadLon0, double eSquare, double a, double heightchange)
    {
        long double m0, t, etaSquare, RadDeltaLon;
        long double N, X0;
        long double ePrimeSquare;
        long double aa, da, db; // ykoky2k
        RadDeltaLon = RadLon - RadLon0;
        ePrimeSquare = eSquare / (1 - eSquare);
        N = (a / sqrt(1 - eSquare * sin(RadLat) * sin(RadLat))) + heightchange;
        aa = N * sqrt(1 - eSquare * sin(RadLat) * sin(RadLat));
        da = aa - a;
        db = eSquare * sin(RadLat) * cos(RadLat) * (1 - eSquare * sin(RadLat) * cos(RadLat)) * da / (a * (1 - eSquare));
        RadLat = RadLat + db;
        a = aa;
        m0 = RadDeltaLon * cos(RadLat);
        t = tan(RadLat);
        etaSquare = ePrimeSquare * cos(RadLat) * cos(RadLat);
        X0 = GetX(RadLat, eSquare, aa);
        *x = X0 + 1 / 2.0 * N * t * pow(m0, 2) + 1 / 24.0 * (5.0 - pow(t, 2) + 9 * etaSquare + 4 * pow(etaSquare, 2)) * N * t * pow(m0, 4) + 1 / 720.0 * (61.0 - 58 * pow(t, 2) + pow(t, 4) + 270 * etaSquare - 330 * etaSquare * pow(t, 2)) * N * t * pow(m0, 6) + 1 / 40320 * N * t * pow(m0, 8) * (1385 - 3111 * pow(t, 2) + 543 * pow(t, 4) - pow(t, 6));
        *y = 500000.0 + N * m0 + 1 / 6.0 * (1.0 - pow(t, 2) + etaSquare) * N * pow(m0, 3) + 1 / 120.0 * (5.0 - 18.0 * pow(t, 2) + pow(t, 4) + 14.0 * etaSquare - 58.0 * pow(t, 2) * etaSquare) * N * pow(m0, 5) + 1 / 5040.0 * pow(m0, 7) * (61 - 479 * pow(t, 2) + 179 * pow(t, 4) - pow(t, 6));
        return true;
    }
    unsigned long CRC24(long size, const unsigned char *buf)
    {
        unsigned long crc = 0;
        int ii;
        while (size--)
        {
            crc ^= (*buf++) << (16);
            for (ii = 0; ii < 8; ii++)
            {
                crc <<= 1;
                if (crc & 0x1000000)
                {
                    crc ^= 0x01864cfb;
                }
            }
        }
        return crc;
    }
    char *trim(char *pStr)
    {
        int len = strlen(pStr);
        char *pIndex = pStr + len - 1;
        while (*pIndex == '\n' || *pIndex == '\r' || *pIndex == '\0' || *pIndex == ' ')
        {
            *pIndex = '\0';
            if (pStr == pIndex)
                break;
            pIndex--;
        }
        return pStr;
    }
    void excludeAnnoValue(char *value, const char *in)
    {
        int num = 0, lstart = false;
        const char *ptr = in;
        while (*ptr != '\0')
        {
            if (lstart == false && (*ptr == ' ' || *ptr == '\n' || *ptr == '\t'))
            {
                ++ptr;
                continue;
            }
            lstart = true;
            if (*ptr != '#' && *ptr != '!')
            {
                value[num++] = *ptr;
                ++ptr;
            }
            else
                break;
        }
        value[num++] = '\0';
        trim(value);
    }
    string zipJson(string json)
    {
        string zipStr = "";
        int isize = 0, jsize = json.size();
        char *zipPtr = new char[jsize];

        memset(zipPtr, 0, sizeof(char) * jsize);
        for (char ch : json)
        {
            if (ch != ' ' && ch != '\t' && ch != '\n')
            {
                zipPtr[isize++] = ch;
            }
        }
        zipStr = string(zipPtr);
        delete[] zipPtr;
        return zipStr;
    }

    void get_compile_date_base(uint8_t *Year, uint8_t *Month, uint8_t *Day)
    {
        const char *pMonth[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        const char Date[12] = __DATE__; // 取编译时间
        uint8_t i;
        for (i = 0; i < 12; i++)
            if (memcmp(Date, pMonth[i], 3) == 0)
                *Month = i + 1, i = 12;
        *Year = (uint8_t)atoi(Date + 9); // Date[9]为２位年份，Date[7]为完整年份
        *Day = (uint8_t)atoi(Date + 4);
    }
    char *get_compile_date(char *g_date_buf)
    {
        uint8_t Year, Month, Day;
        get_compile_date_base(&Year, &Month, &Day);            // 取编译时间
        sprintf(g_date_buf, "%02d%02d%02d", Year, Month, Day); // 任意格式化
        return g_date_buf;
    }

    char *substringEx(char *dest, const char *src, int start, int length)
    {
        int i, j = 0;
        int len = strlen(src);
        if (start < 0 || start >= len || start + length > len)
        {
            dest[0] = '\0';
            return NULL;
        }
        for (i = start; i < start + length; i++)
        {
            dest[j] = src[i];
            j++;
        }
        dest[j] = '\0';
        return dest;
    }
    /* decode tcp/ntrip path (path=[user[:passwd]@]addr[:port][/mntpnt[:str]]) ---*/
    void decodetcppath(const char *path, char *addr, char *port, char *user,
                       char *passwd, char *mntpnt, char *str)
    {
        int NTRIP_MAXSTR = 256;
        char buff[1024], *p, *q;

        if (port)
            *port = '\0';
        if (user)
            *user = '\0';
        if (passwd)
            *passwd = '\0';
        if (mntpnt)
            *mntpnt = '\0';
        if (str)
            *str = '\0';

        strcpy(buff, path);

        if (!(p = strrchr(buff, '@')))
            p = buff;

        if ((p = strchr(p, '/')))
        {
            if ((q = strchr(p + 1, ':')))
            {
                *q = '\0';
                if (str)
                    sprintf(str, "%.*s", NTRIP_MAXSTR - 1, q + 1);
            }
            *p = '\0';
            if (mntpnt)
                sprintf(mntpnt, "%.255s", p + 1);
        }
        if ((p = strrchr(buff, '@')))
        {
            *p++ = '\0';
            if ((q = strchr(buff, ':')))
            {
                *q = '\0';
                if (passwd)
                    sprintf(passwd, "%.255s", q + 1);
            }
            if (user)
                sprintf(user, "%.255s", buff);
        }
        else
            p = buff;

        if ((q = strchr(p, ':')))
        {
            *q = '\0';
            if (port)
                sprintf(port, "%.255s", q + 1);
        }
        if (addr)
            sprintf(addr, "%.255s", p);
    }
    void s2ipp(double *pos, double azim, double elev, double r, double hion, double *posp)
    {
        if (r == 0.0)
            r = EARTH_RADIUS;
        double rp = r / (r + hion) * std::cos(elev);   // sin Z
        double ap = M_PI / 2.0 - elev - std::asin(rp); // 穿刺点与地心和测站和地心的夹角
        double sinap = std::sin(ap);
        double cosap = std::cos(ap);
        double tanap = std::tan(ap);
        double cosaz = std::cos(azim);

        posp[0] = std::asin(std::sin(pos[0]) * cosap + std::cos(pos[0]) * sinap * cosaz); // 严密公式

        double sindlm = sinap * std::sin(azim) / std::cos(posp[0]);
        double cosdlm = (cosap - std::sin(pos[0]) * std::sin(posp[0])) / (std::cos(pos[0]) * std::cos(posp[0]));
        posp[1] = pos[1] + std::atan2(sindlm, cosdlm);
        posp[2] = 1.0 / std::sqrt(1.0 - rp * rp);
    }
    int len_trim(const char *pStr)
    {
        int length = strlen(pStr);
        int count = length;
        int i;
        for (i = length - 1; i >= 0; i--)
        {
            if (pStr[i] == '\0' || pStr[i] == '\n' || pStr[i] == '\r' || pStr[i] == ' ')
                count--;
            else
                break;
        }
        return count;
    }
    void split_string(bool lnoempty, char *string, char c_start, char c_end,
                      char seperator, int *nword, char *keys, int len)
    {
        int i0 = 0, i1, ilast, i;
        char varword[1024];
        char *word = (char *)keys;
        i1 = len_trim(string) - 1;
        if (c_start != ' ')
            i0 = index_string(string, c_start);
        if (c_end != ' ')
            i1 = index_string(string, c_end);
        *nword = 0;
        if (i1 == -1 || i0 > i1)
            return;

        ilast = i0;
        if (string[i1] != seperator)
            string[i1 + 1] = seperator;
        string[i1 + 2] = '\0';

        for (i = i0; i < i1 + 2; i++)
        {
            if (string[i] == seperator)
            {
                if (i - 1 >= ilast)
                {
                    substringEx(varword, string, ilast, i - ilast);
                    strcpy(word + (*nword) * len, varword);
                    (*nword)++;
                }
                else
                {
                    if (!lnoempty)
                    {
                        strcpy(word + (*nword) * len, "  ");
                        (*nword)++;
                    }
                }
                ilast = i + 1;
            }
        }
    }
    void fillobs(char *line, int nobs, int itemlen, double ver)
    {
        int OFFSET = 0, len, i;
        char tmp[256];
        if (ver > 3.0)
            OFFSET = 3;
        len = len_trim(line);
        for (i = len; i < itemlen * nobs + OFFSET; i++)
            line[i] = ' ';
        line[itemlen * nobs + OFFSET] = '\0';
        for (i = 0; i < nobs; i++)
        {
            memset(tmp, 0, sizeof(char) * 256);
            substringEx(tmp, line + OFFSET + i * itemlen, 0, itemlen - 2); // last is signal strength
            if (len_trim(tmp) == 0)
            {
                line[itemlen * i + OFFSET + 1] = '0';
            }
        }
    }

    void filleph(char *line, double ver)
    {
        int i, len, cpre = 4;
        char tmp[128];
        if (ver < 3.0)
            cpre = 3;
        len = len_trim(line);
        for (i = len; i < cpre + 19 * 4; i++)
        {
            line[i] = ' ';
        }
        line[cpre + 19 * 4] = '\0';
        for (i = 0; i < 4; i++)
        {
            substringEx(tmp, line + cpre + 19 * i, 0, 19);
            if (len_trim(tmp) == 0)
            {
                line[cpre + 19 * i + 1] = '0';
            }
        }
    }

    void yr2year(int &yr)
    {
        if (yr > 1900)
            return;
        if (yr <= 30)
            yr += 2000;
        else
            yr += 1900;
    }

    double timdif(int jd2, double sod2, int jd1, double sod1)
    {
        return 86400.0 * (jd2 - jd1) + sod2 - sod1;
    }

    void brdtime(char *cprn, int *mjd, double *sod)
    {
        switch (cprn[0])
        {
        case 'C':
        case 'B':
            timinc(*mjd, *sod, 14.0, mjd, sod);
            break;
        case 'R':
            *sod = time2mjd(utc2gpst(mjd2time(*mjd, *sod)), mjd);
            break;
        }
    }

    int genAode(char csys, int mjd, double sod, double toe, int inade)
    {
        int mjd_r, ret = -1;
        double sod_r;
        if (csys == 'G' || csys == 'J')
            ret = inade;
        else if (csys == 'R')
        {
            sod_r = time2mjd(gpst2utc(mjd2time(mjd, sod)), &mjd_r);
            //		ret = NINT(sod / 900.0) + 1;
            // IGMAS:
            ret = NINT(fmod(sod_r + 10800.0, 86400.0) / 900.0);
        }
        else if (csys == 'E')
        {
            // ret = NINT(sod / 600.0) + 1;
            // IGMAS:
            ret = inade;
        }
        else if (csys == 'C')
        {
            ret = NINT(fmod(toe, 86400.0) / 1800.0) + 1; // zhbd
            //		 ret = NINT(sod / 1800.0) + 1;
            // IGMAS:
            // ret = NINT(fmod(toe, 86400.0) / 450.0); // geng laoshi
            //		ret = getbdsiode(*eph);
            //        ret = (int)eph->iodc;
        }
        return ret;
    }

    void blhxyz(double *geod, double a0, double b0, double *x)
    {
        double a, b, e2, N, W;
        if (a0 == 0 || b0 == 0)
        {
            a = 6378137.0;
            b = 298.257223563; // alpha = (a-b)/a
        }
        else
        {
            a = a0;
            b = b0;
        }
        if (b <= 6000000)
            b = a - a / b;
        e2 = (a * a - b * b) / (a * a);

        W = sqrt(1 - e2 * pow(sin(geod[0]), 2));
        N = a / W;
        x[0] = (N + geod[2]) * cos(geod[0]) * cos(geod[1]);
        x[1] = (N + geod[2]) * cos(geod[0]) * sin(geod[1]);
        x[2] = (N * (1 - e2) + geod[2]) * sin(geod[0]);
    }

    void xyzblh(double *x, double scale, double a0, double b0, double dx, double dy,
                double dz, double *geod)
    {
        double a, b;
        long double xp, yp, zp, s, e2, rhd, rbd, n, zps, tmp1, tmp2;
        int i;

        if (a0 == 0 || b0 == 0)
        {
            a = 6378137.0;
            b = 298.257223563; // alpha = (a-b)/a
        }
        else
        {
            a = a0;
            b = b0;
        }
        for (i = 0; i < 3; i++)
            geod[i] = 0;
        xp = x[0] * scale + dx;
        yp = x[1] * scale + dy;
        zp = x[2] * scale + dz;

        if (b <= 6000000)
            b = a - a / b;
        e2 = (a * a - b * b) / (a * a);
        s = sqrt(xp * xp + yp * yp);
        geod[1] = atan(yp / xp);
        if (geod[1] < 0)
        {
            if (yp > 0)
                geod[1] += PI;
            if (yp < 0)
                geod[1] += 2 * PI;
        }
        else
        {
            if (yp < 0)
                geod[1] += PI;
        }
        zps = zp / s;
        geod[2] = sqrt(xp * xp + yp * yp + zp * zp) - a;
        geod[0] = atan(zps / (1.0 - e2 * a / (a + geod[2])));
        n = 1;
        rhd = rbd = 1;
        while (rbd * n > 1e-4 || rhd > 1e-4)
        {
            n = a / sqrt(1.0 - e2 * sin(geod[0]) * sin(geod[0]));
            tmp1 = geod[0];
            tmp2 = geod[2];
            geod[2] = s / cos(geod[0]) - n;
            geod[0] = atan(zps / (1.0 - e2 * n / (n + geod[2])));
            rbd = ABS(tmp1 - geod[0]);
            rhd = ABS(tmp2 - geod[2]);
        }
    }
    void rot_enu2xyz(double lat, double lon, double (*rotmat)[3])
    {
        double coslat, sinlat, coslon, sinlon;
        coslat = cos(lat - PI / 2);
        sinlat = sin(lat - PI / 2);
        coslon = cos(-PI / 2 - lon);
        sinlon = sin(-PI / 2 - lon);

        rotmat[0][0] = coslon;
        rotmat[0][1] = sinlon * coslat;
        rotmat[0][2] = sinlon * sinlat;
        rotmat[1][0] = -sinlon;
        rotmat[1][1] = coslon * coslat;
        rotmat[1][2] = coslon * sinlat;
        rotmat[2][0] = 0;
        rotmat[2][1] = -sinlat;
        rotmat[2][2] = coslat;
    }

    double m2stec(double delay, double fq)
    {
        return delay * fq * fq / 40.28 / 1e16;
    }

    void callExit()
    {
        /* terminate the program */
        LOGPRINT_EX("Terminated by inner fault!");
        exit(1);
    }
} // namespace iono
