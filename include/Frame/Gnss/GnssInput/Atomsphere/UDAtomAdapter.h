#ifndef INCLUDE_FRAME_IONOSPHERE_SLANT_ADAPTER_H__
#define INCLUDE_FRAME_IONOSPHERE_SLANT_ADAPTER_H__
#include "include/Frame/Config/Const.h"
#include <string>
#include <mutex>
#include <condition_variable>
#include <map>
using namespace std;
namespace iono
{
    class StecC
    {
    public:
        StecC()
        {
            isel = 1;
            isat = -1;
            ionva = 0.0;
            ionvm = 0.0;
            R = 0.0;
            memset(name, 0, sizeof(name));
            memset(x, 0, sizeof(x));
            memset(gausx, 0, sizeof(gausx));
        };
        int isat;        /* current satellite */
        int isel;        /* whether is selected */
        char name[8];    /* current name */
        double ionva;    /* current ion value on L1, unit in stec */
        double ionvm;    /* current model value */
        double elev;     /* current elevation,in degrees */
        double azim;     /* current azim, in degrees */
        double x[3];     /* current coordinates in plane, in kilometers */
        double gausx[3]; /* current coordinates in plane, in kilometers */
        double R;        /* weight of observation*/
        gtime_t t_r;     /* current tt */
    };
    class UDAtomAdapter
    {
    public:
        using t_keysit = map<int, StecC>;
        UDAtomAdapter() {}
        ~UDAtomAdapter() {}

        map<string, t_keysit> m_inquireIono(int &mjd, double &sod, double span);
        void m_inputIono(StecC &item);           /* input ionosphere */
        void m_inputIono(map<int, StecC> &item); /* input ionosphere */

    public:
        time_t t_lastsel = 0;
        time_t t_lastrcv = 0;

        time_t t_lastlog = 0;

        map<time_t, map<string, t_keysit>> m_iondata; /* time: sta: sat : value */
        std::mutex mltx;
        std::condition_variable mcv;
    };
}

#endif