#ifndef INCLUDE_GNSSINPUT_GNSSOBS_QCTURBOEDIT_H_
#define INCLUDE_GNSSINPUT_GNSSOBS_QCTURBOEDIT_H_
#include "include/Frame/Config/Const.h"
#include "SmoothAdapter.h"
namespace iono
{
    class QcSlip
    {
    public:
        QcSlip()
        {
            mjd = 0;
            sod = 0;
            tec = 0.0;
            dpr = 0.0;
            ptime_lastobs = 0.0;
        }
        int mjd, n_mw;
        double tec, sod, mean_mw, dpr, ptime_lastobs;
        SmoothAdapter smtecr;
    };
    class QcTurboEdit
    {
    public:
        QcTurboEdit()
        {
            memset(m_dt_span, 0, sizeof(m_dt_span));
        }
        void m_qcDetectSlip_Sf(int mjd, double sod, string staname, double (*obs)[2 * MAXFREQ], double *elev, QcSlip[MAXSAT][MAXFREQ], int flag[MAXSAT][MAXFREQ]);
        void m_qcDetectSlip_Df(int mjd, double sod, string staname, double (*obs)[2 * MAXFREQ], double *elev, QcSlip[MAXSAT][MAXFREQ], int flag[MAXSAT][MAXFREQ]);

        void m_qcDetectSlip_Tf(int mjd, double sod, string staname, double (*obs)[2 * MAXFREQ], double *elev);
        double m_dt_span[MAXSAT][MAXFREQ];
    };
} // namespace bamboo
#endif