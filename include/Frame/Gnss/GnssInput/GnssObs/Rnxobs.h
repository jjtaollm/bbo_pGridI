/*
 * Rnxobs.h
 *
 *  Created on: 2018/2/3
 *      Author: juntao, at wuhan university
 */

#ifndef INCLUDE_BAMBOO_RNXOBS_H_
#define INCLUDE_BAMBOO_RNXOBS_H_
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "QcTurboEdit.h"
#include "include/Frame/Config/Const.h"
using namespace std;
namespace iono
{
    class Rnxobs : public QcTurboEdit
    {
    public:
        Rnxobs()
        {
            // Reset Varibles
            mjd = 0;
            sod = 0;
            memset(x, 0, sizeof(x));
            memset(flag, 0, sizeof(flag));
            memset(slip, 0, sizeof(slip));
            memset(obs, 0, sizeof(double) * MAXSAT * 2 * MAXFREQ);
            memset(fob, 0, sizeof(char) * MAXSAT * MAXFREQ * 2 * LEN_OBSTYPE);
            memset(snr, 0, sizeof(snr));

            memset(usetype, 0, sizeof(usetype));
            memset(tstore, 0, sizeof(tstore));
            memset(elev, 0, sizeof(elev));
        }
        Rnxobs(string name)
        {
            // Reset Varibles
            staname = name;
            mjd = 0;
            sod = 0;
            memset(x, 0, sizeof(x));
            memset(flag, 0, sizeof(flag));
            memset(slip, 0, sizeof(slip));
            memset(obs, 0, sizeof(double) * MAXSAT * 2 * MAXFREQ);
            memset(fob, 0, sizeof(char) * MAXSAT * MAXFREQ * 2 * LEN_OBSTYPE);
            memset(snr, 0, sizeof(snr));
            memset(usetype, 0, sizeof(usetype));
            memset(tstore, 0, sizeof(tstore));
            memset(elev, 0, sizeof(elev));
        }
        virtual ~Rnxobs()
        {
        }

    public:
        virtual void v_openRnx(string) {}
        virtual void v_readEpoch(int &mjd, double &sod)
        {
            memset(obs, 0, sizeof(obs));
            memset(dop, 0, sizeof(dop));
            memset(snr, 0, sizeof(snr));
        }
        virtual void v_closeRnx() {}
        virtual int v_isOpen() { return 0; }
        void m_wl_azel(int mjd, double sod, double *elevation, double *azim, double (*obs)[MAXFREQ * 2]);
        static bool s_checkObs(int isat, double *obs, double *dop, double *snr);
        /// quality control
        void m_onDataQC();
        double x[6];
        QcSlip qc[MAXSAT][MAXFREQ];                                                             /* for quality control */
        double obs[MAXSAT][2 * MAXFREQ], dop[MAXSAT][MAXFREQ], sod, elev[MAXSAT], azim[MAXSAT]; //,ph_base[MAXSAT][MAXFREQ];
        char fob[MAXSAT][2 * MAXFREQ][LEN_OBSTYPE];
        double snr[MAXSAT][MAXFREQ];
        string staname, rectype, anttype;
        int mjd, flag[MAXSAT], slip[MAXSAT][MAXFREQ];
        char usetype[MAXSYS][MAXFREQ];
        int tstore[MAXSYS][MAXFREQ];
    };
    class RnxobsFile : public Rnxobs
    {
    public:
        RnxobsFile(string); //
        virtual ~RnxobsFile() { v_closeRnx(); };
        virtual void v_openRnx(string); //
        virtual void v_readEpoch(int &, double &);
        virtual void v_closeRnx();
        inline int v_isOpen() { return in.is_open(); }

    protected:
        // RnxFile Head
        void m_readRnxHead();
        double ver;
        char sys[4];
        string recnum, antnum, mark;
        double x, y, z, e, n, h, intv;
        int nprn, fact1, fact2, nobstype[MAXSYS];
        string obstype[MAXSYS][MAXOBSTYP], cprn_ob[MAXSAT];
        int t0[6], t1[6], nsys;

    private:
        ifstream in;
        string dir;
    };

} // namespace bamboo
#endif /* INCLUDE_BAMBOO_RNXOBS_H_ */
