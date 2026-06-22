#ifndef INCLUDE_FRAME_CONST_H_
#define INCLUDE_FRAME_CONST_H_
#include <sstream>
#include <list>
#include <vector>
#include <map>
#include "include/Frame/Config/Utils/Com/Logtrace.h"
namespace iono
{

#define DEBUG
#define REDIS
    // #define MQTT

#define _IF_0(f1, f2) ((f1 * f1) / (f1 * f1 - f2 * f2))
#define _IF_1(f1, f2) (-(f2 * f2) / (f1 * f1 - f2 * f2))
#define _WL_0(f1, f2) (f1 / (f1 - f2))
#define _WL_1(f1, f2) (-f2 / (f1 - f2))
#define _NL_0(f1, f2) (f1 / (f1 + f2))
#define _NL_1(f1, f2) (f2 / (f1 + f2))

    const unsigned char RTCM2PREAMB = 0x66;   /* rtcm ver.2 frame preamble */
    const unsigned char RTCM3PREAMB = 0xD3;   /* rtcm ver.3 frame preamble */
    const unsigned char ATOMPREAMB = 0xDB;    /* atom preamble */
    const unsigned char ATOMPREAMB_UD = 0xDA; /* atom preamble */
    const int VRS_PREAMB = 0xDA;
    const int MAXFREQ = 4;
    const int MAXSYS = 7;
    const int MAXSAT = 160;
    const int MAXSIT = 1200;
    const int MAXPORT = 10;
    const int LEN_OBSTYPE = 4;
    const int MAXOBSTYP = 64;

    const int MeanEarthRadius = 6371000;
    const int IONO_SHELL_H = 450000;

    const char SYS[] = "GRECBJS"; /// G:GPS R:GLONASS E:GALILEO B:BDS-2 C:BDS-3 J:QZSS L:LEO
    const double MAXWND = 1E-2;

    const double INVALID_VAL = 999;

    const double PI = 3.1415926535897932;
    const double VEL_LIGHT = 299792458.0;
    const double E_MAJAXIS = 6378136.60;

    const double OMGE = 7.2921151467E-5; /* earth angular velocity (IS-GPS) (rad/s) */

    const double RE_WGS84 = 6378137.0;             /* earth semimajor axis (WGS84) (m) */
    const double FE_WGS84 = (1.0 / 298.257223563); /* earth flattening (WGS84) */

    const double DEG2RAD = PI / 180.0;
    const double RAD2DEG = 180.0 / PI;

    const double D2R = PI / 180.0;
    const double R2D = 180.0 / PI;

    const double EARTH_A = 6378137;
    const double EARTH_ALPHA = 1 / 298.257223563;
    const double ESQUARE = (EARTH_A * EARTH_A - (EARTH_A - EARTH_A * EARTH_ALPHA) * (EARTH_A - EARTH_A * EARTH_ALPHA)) / (EARTH_A * EARTH_A);

    const double OFF_GPS2TAI = 19.0;
    const double OFF_TAI2TT = 32.184;
    const double OFF_GPS2TT = OFF_GPS2TAI + OFF_TAI2TT;
    const double OFF_MJD2JD = 2400000.50;

    const double E_ROTATE = 7.2921151467E-5;
    const double GME = 3.986004415E14; // TT-compatible
    const double GPS_FREQ = 10230000.0;
    const double GPS_L1 = 154 * GPS_FREQ;
    const double GPS_L2 = 120 * GPS_FREQ;
    const double GPS_L5 = 115 * GPS_FREQ;

    const double GLS_FREQ = 178000000.0;
    const double GLS_L1 = 9 * GLS_FREQ;
    const double GLS_L2 = 7 * GLS_FREQ;
    const double GLS_dL1 = 562500.0;
    const double GLS_dL2 = 437500.0;

    const double GAL_E1 = GPS_L1;
    const double GAL_E5 = 1191795000.0;
    const double GAL_E5a = GPS_L5;
    const double GAL_E5b = 1207140000.0;
    const double GAL_E6 = 1278750000.0;

    const double BDS_B1 = 1561098000.0;
    const double BDS_B2b = 1207140000.0;
    const double BDS_B3 = 1268520000.0;
    const double BDS_B1c = 1575420000.0;
    const double BDS_B2a = 1176450000.0;
    const double BDS_B2 = 1191795000.0;
    const double QZS_L1 = GPS_L1;
    const double QZS_L2 = GPS_L2;
    const double QZS_L5 = GPS_L5;
    const double QZS_LEX = 1278750000.0;

    const int AMB_FIX = 5;

    const int MAX_GRID = 73;
    const double HION = 450000.0;
    const double EARTH_RADIUS = 6371000;

    const int ORB_NONE = 0;
    const int CLK_NONE = 0;
    const int ORB_BRD = 1;
    const int CLK_BRD = 1;

    const int LEN_PRN = 5;
    const int LEN_EPHION = 4;

#define FILEPATHSEP '/'
#define LOGPRINT(format, ...)                                                             \
    {                                                                                     \
        Logtrace::m_logPrint("[%s,%d]: " format "\n", __func__, __LINE__, ##__VA_ARGS__); \
    }

#define LOGPRINT_EX(format, ...)                                                          \
    do                                                                                    \
    {                                                                                     \
        Logtrace::m_logPrint("[%s,%d]: " format "\n", __func__, __LINE__, ##__VA_ARGS__); \
        Logtrace::m_logStatInfo(format "\n", ##__VA_ARGS__);                              \
    } while (0)

    template <typename T>
    string toString(const T &t)
    {
        ostringstream oss; //
        oss << t;          //
        return oss.str();
    }
    template <class T>
    inline T MAX(T a, T b)
    {
        return a > b ? a : b;
    }
    template <class T>
    inline T MIN(T a, T b)
    {
        return a > b ? b : a;
    }

    template <class T, class P>
    inline T SIGN(T a, P b)
    {
        return b > 0 ? a : -a;
    }
    template <class T>
    inline int NINT(T a)
    {
        return (int)(a + SIGN(1, a) * 0.5);
    }
    template <class T>
    inline int FLOOR(T a)
    {
        return fabs(a - ((int)a)) > 0.5 ? NINT(a) - SIGN(1, a) : NINT(a) + SIGN(1, a);
    }

    template <class T>
    inline T ABS(T a)
    {
        return a > 0 ? a : -a;
    }

    template <class T>
    inline void bbo_dellist(list<T> &in, T del)
    {
        for (auto p = begin(in); p != end(in);)
        {
            if (del == (*p))
            {
                p = in.erase(p);
                break;
            }
            ++p;
        }
    }

    template <class T>
    inline int bbo_index_vector(vector<T> &in, T check)
    {
        for (auto p = begin(in); p != end(in);)
        {
            if (check == (*p))
            {
                return p - begin(in);
            }
            ++p;
        }
        return -1;
    }

    template <class T>
    inline int bbo_index_list(list<T> &in, T check)
    {
        for (auto p = begin(in); p != end(in);)
        {
            if (check == (*p))
            {
                return p - begin(in);
            }
            ++p;
        }
        return -1;
    }

    template <class T>
    inline bool bbo_contains_list(list<T> &in, T check)
    {
        for (auto p = begin(in); p != end(in);)
        {
            if (check == (*p))
            {
                return true;
            }
            ++p;
        }
        return false;
    }
    template <class T>
    inline void bbo_delvector(vector<T> &in, T del)
    {
        for (auto p = begin(in); p != end(in);)
        {
            if (del == (*p))
            {
                p = in.erase(p);
                break;
            }
            ++p;
        }
    }
    template <class T>
    inline bool bbo_contains_vector(vector<T> &in, T check)
    {
        for (auto p = begin(in); p != end(in);)
        {
            if (check == (*p))
            {
                return true;
            }
            ++p;
        }
        return false;
    }

    const char OBSTYPE[MAXSYS][MAXFREQ][32] = { // WPCIXSAQLDBYMZN  //"WPCIXSAQLDBYMZN ";
        {                                       // G:
         //   {"WPCIXSAQLDBYMZN "},
         //   {"WPCIXSAQLDBYMZN "},
         //   {"WPCIXSAQLDBYMZN "}},
         {"WPCISXAQLDBYMZN "},
         {"WPCISXAQLDBYMZN "},
         {"WPCISXAQLDBYMZN "},
         {"WPCISXAQLDBYMZN "}},
        {// R:
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "}},
        {// E:
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "}},
        {// C:
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "}},
        {// B:
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "}},
        {// J:
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "}},
        {// S:
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "},
         {"WPCIXSAQLDBYMZN "}}};

    typedef struct
    {                /* time struct */
        time_t time; /* time (s) expressed by standard time_t */
        double sec;  /* fraction of second under 1 s */
    } gtime_t;
}
#endif