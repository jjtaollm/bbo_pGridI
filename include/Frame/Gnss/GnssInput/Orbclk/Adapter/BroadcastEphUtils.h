//
// Created by juntao, at wuhan university   on 2020/11/3.
//

#ifndef BAMBOO_BROADCASTEPHUTILS_H
#define BAMBOO_BROADCASTEPHUTILS_H
#include <string>
#include <map>
#include "include/Frame/Config/Const.h"
namespace iono
{
    class BroadcastEphUtils
    {
    public:
        BroadcastEphUtils();
        virtual ~BroadcastEphUtils();
        virtual int
        m_brd2xyz(const char *mode, const char *cprn, int wk, double sow, double *xsat, double *clk, double *dtmin,
                  double *tgd, int *iode);
        virtual int
        m_gls2xyz(const char *mode, const char *cprn, int wk, double sow, double *xsat, double *clk, double *dtmin,
                  int *iode);
        static void m_geph2xyz(GPSEPH *, const char *mode, int wk, double sow, double *xsat, double *clk);
        static void m_reph2xyz(GLSEPH *, const char *mode, int wk, double sow, double *xsat, double *clk);
        std::map<std::string, GPSEPH> m_getCurrentGpsEph(double ptime);
        std::map<std::string, GLSEPH> m_getCurrentGlsEph(double ptime);
        static void glsinit(double *x, GLSEPH &eph);
        static void glsrkf4(double h, double *x, GLSEPH &eph);
        static void glsfright(double *x, double *acc, GLSEPH &eph);
        static void pz902wgs84(int mjd, double sod, double *pos, double *xsat, const char *trans);

        // protected:
    public:
        map<string, list<GPSEPH>> gnssEph;
        map<string, list<GLSEPH>> glsEph;
    };
} // namespace bamboo
#endif // BAMBOO_BROADCASTEPHUTILS_H
