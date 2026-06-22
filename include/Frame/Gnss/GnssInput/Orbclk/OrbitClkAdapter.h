//
// Created by juntao, at wuhan university   on 2020/11/2.
//

#ifndef BAMBOO_ORBITCLKADAPTER_H
#define BAMBOO_ORBITCLKADAPTER_H
#include <string>
#include <iostream>
#include "include/Frame/Config/Const.h"
namespace iono
{
    /// Interface
    class GPSEPH
    {
    public:
        GPSEPH()
        {
            mjd = 0;
            sod = 0.0;
            memset(cprn, 0, sizeof(cprn));
            signal_idx = 0;
            time(&gent);
        }
        char cprn[LEN_PRN];
        int mjd;
        double sod;
        // a[0]: SV clock offset
        // a[1]: SV clock drift
        // a[2]: SV clock drift rate
        double a0, a1, a2;
        // aode: age of ephemeris upload
        // crs, crc: Ortital radius correction
        // dn: Mean motion difference
        // m0: Mean anomaly at reference epoch
        // e: Eccentricity
        // cuc, cus: Latitude argument correction
        // roota: Square root of semi-major axis
        unsigned int aode;
        double crs, dn;
        double m0, cuc, e;
        double cus, roota;
        // toe, week: Ephemerides reference epoch in seconds with the week
        // cis, cic: Inclination correction
        // omega0: Longtitude of ascending node at the begining of the week
        // i0: Inclination at reference epoch
        // omega: Argument of perigee
        // omegadot: Rate of node's right ascension
        double toe, cic, omega0;
        double cis, i0, crc;
        double omega, omegadot;
        // idot: Rate of inclination angle
        // sesvd0:
        // resvd1:
        // accu: SV accuracy
        // hlth: SV health
        // tgd: Time group delay
        // aodc: Age of clock parameter upload
        double idot, resvd0, week, resvd1;
        double accu, hlth, tgd, tgd1, aodc;
        double iodc, signal_idx, delta_A, A_DOT, delta_n_dot;
        time_t gent;
    };
    class GLSEPH
    {
    public:
        GLSEPH()
        {
            mjd = 0;
            sod = 0.0;
            memset(cprn, 0, sizeof(cprn));
            time(&gent);
        }
        char cprn[LEN_PRN];
        int mjd, aode;
        double sod;
        // tau: SV clock bias
        // gama: SV relative frequency bias
        // tk: message frame time (tk+nd*86400) in seconds of the UTC week
        // pos: coordinate at ephemerides reference epoch in PZ-90
        // vel: velocity at ephemerides reference epoch in PZ-90
        // acc: acceleration at ephemerides reference epoch in PZ-90
        // health: SV health
        // frenum: frequency number
        // age: age of operation information
        double tau;
        double gamma;
        double tk;
        double pos[3];
        double vel[3];
        double acc[3];
        double health;
        double frenum;
        double age;
        time_t gent;
    };

    class OrbitAdapter
    {
    public:
        OrbitAdapter()
        {
            orbType = ORB_NONE;
        }
        virtual ~OrbitAdapter()
        {
        }
        virtual int v_readOrbit(const char *cprn, int mjd, double sod, double *xsat)
        {
            cout << "***(ERROR):Base Class is not implement!" << endl;
            return 0;
        }
        virtual int v_readOrbit(const char *cprn, int mjd, double sod, double *xsat, int *iode)
        {
            cout << "***(ERROR):Base Class is not implement!" << endl;
            return 0;
        }
        int m_getOrtType()
        {
            return orbType;
        }

    protected:
        int orbType;
    };
    class ClkAdapter
    {
    public:
        ClkAdapter()
        {
            clkType = CLK_NONE;
        }
        virtual ~ClkAdapter()
        {
        }
        virtual int v_readClk(const char *cprn, int mjd, double sod, double *sclk)
        {
            std::cout << "***(ERROR):Base Class is not implement!" << std::endl;
            return 0;
        }
        virtual int v_readClk(const char *cprn, int mjd, double sod, double *sclk, int *iode)
        {
            std::cout << "***(ERROR):Base Class is not implement!" << std::endl;
            return 0;
        }
        virtual int v_readClkDrift(const char *cprn, int mjd, double sod, double *vclk)
        {
            *vclk = 0.0;
            std::cout << "***(ERROR):Base class is not implement" << std::endl;
            return 0;
        }
        int m_getClkType()
        {
            return clkType;
        }

    protected:
        int clkType;
    };
    class OrbitClkAdapter : public ClkAdapter, public OrbitAdapter
    {
    public:
        OrbitClkAdapter()
        {
        }
        virtual ~OrbitClkAdapter()
        {
        }

        /// get broadcast ephemeris here
        virtual map<string, GPSEPH> v_getCurrentGpsEph(double ptime) { return {}; }
        virtual map<string, GLSEPH> v_getCurrentGlsEph(double ptime) { return {}; }

        /// input function here
        virtual void v_inputEph(GPSEPH *, GLSEPH *) {} // broadcast ephemeris
    };
} // namespace bamboo
#endif // BAMBOO_ORBITCLKADAPTER_H
