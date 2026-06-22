#ifndef INCLUDE_FRAME_CONNECTION_IONOSPHERE_ATOMDECODE_H_
#define INCLUDE_FRAME_CONNECTION_IONOSPHERE_ATOMDECODE_H_

#include "include/Applications/Applications.h"

using namespace std;
namespace iono
{
    class AtomDecoder
    {
    public:
        /* debug begin */
        typedef struct
        {
            int nsat;
            int n; // degree of lat
            int m; // degree of lon
            int udint;
            int iod;
            int sync;
            double lat0;
            double lon0;
            double tow;
            gtime_t time0;
            char cprn[MAXSAT][LEN_PRN];
            double coef[MAXSAT][9];
            float sig[MAXSAT];
        } ionmodel_t;

        typedef struct
        {
            int nsat;
            gtime_t time; /* time of atmo (GPST) */
            int dtype;
            int udint;
            int iod;
            int sync;
            int solid;
            double geod[3]; /* station blh */
            char cprn[MAXSAT][LEN_PRN];
            double elev[MAXSAT];
            float ionval[MAXSAT]; /* ionospher correction value */
            float ionsig[MAXSAT]; /* ionospher correction sigma */
        } iongrid_t;

        typedef struct kpl_atmo_rtcm_t
        {
            int len;
            int outtype;       /* output message type */
            char msgtype[256]; /* last message type */
            uint8_t buff[1200];
            gtime_t time; /* message time */
        } kpl_atmo_rtcm_t;

        static void adjweek(kpl_atmo_rtcm_t *rtcm, double tow);
        static void adjday_glot(kpl_atmo_rtcm_t *rtcm, double tod);

        /* polynomial */
        static int decode_ssr_epoch_model(kpl_atmo_rtcm_t *rtcm, int sys, int subtype);
        static int decode_ssr9_head_model(kpl_atmo_rtcm_t *rtcm, ionmodel_t *s_ionmodel, int sys, int subtype, int *sync, int *iod, double *udint, int *hsize);
        static int kpl_decode_ssr9_model(kpl_atmo_rtcm_t *rtcm, ionmodel_t *s_ionmodel, int sys, int subtype);
        static int s_decode_poly_iono(char *pack, int plen, int isys);

        /* grid */
        static int decode_ssr_epoch_grid(kpl_atmo_rtcm_t *rtcm, int sys, int subtype);
        static int decode_ssr9_head_grid(kpl_atmo_rtcm_t *rtcm, iongrid_t *s_iongrid, int sys, int subtype, int *sync, int *iod, double *udint, int *hsize);
        static int kpl_decode_ssr9_grid(kpl_atmo_rtcm_t *rtcm, iongrid_t *s_iongrid, int sys, int subtype);
        static int s_decode_grid_iono(char *pack, int plen, int isys);
        /* debug end */
    };
}

#endif
