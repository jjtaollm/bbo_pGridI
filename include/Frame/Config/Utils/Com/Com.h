/*
 * Com.h
 *
 *  Created on: 2018/2/12
 *      Author: juntao, at wuhan university
 */

#ifndef INCLUDE_COM_COM_H_
#define INCLUDE_COM_COM_H_

#include <iostream>
#include <vector>

using namespace std;
namespace iono
{

    extern uint32_t tickget(void);

    extern gtime_t epoch2time(const double *ep);

    extern void time2epoch(gtime_t t, double *ep);

    extern void time2epoch_n(gtime_t t, double *ep, int n);

    extern gtime_t gpst2time(int week, double sec);

    extern double time2gpst(gtime_t t, int *week);

    extern char *run_timefmt(int mjd, double sod, char *buf);

    extern char *run_timefmt(time_t tt, char *buf);

    extern void time2str(gtime_t t, char *s, int n);

    extern char *time2str(time_t t_i, char *s, int n);

    extern gtime_t gpst2utc(gtime_t t);

    extern gtime_t utc2gpst(gtime_t t);

    extern char *runlocaltime(char *buf);

    extern void timinc(int jd, double sec, double delt, int *jd1, double *sec1);

    extern double mjd2wksow(int mjd, double sod, int *week, double *sow);

    extern void matmpy(double *A, double *B, double *C, int row, int colA, int colB);

    extern void wksow2mjd(int week, double sow, int *mjd, double *sod);

    extern gtime_t mjd2time(int mjd, double sod);

    extern double time2mjd(gtime_t t, int *mjd);

    extern double time2mjd(time_t tt, int *mjd);

    extern void sleepms(int ms);

    extern int ymd2mjd(int iyear, int imonth, int iday);

    extern int mjd2doy(int jd, int *iy);

    extern gtime_t bdt2time(int week, double sec);

    extern double time2bdt(gtime_t t, int *week);

    extern gtime_t timeget(void);

    extern gtime_t timeadd(gtime_t t, double sec);

    extern double timediff(gtime_t t1, gtime_t t2);

    extern double dot(const double *a, const double *b, int n);

    extern void ecef2pos(const double *r, double *pos);

    extern void pos2ecef(const double *pos, double *r);

    extern void xyz2enu(const double *pos, double *E);

    extern void ecef2enu(const double *pos, const double *r, double *e);

    extern void enu2ecef(const double *pos, const double *e, double *r);

    extern int pointer_string(int cnt, string string_array[], string str);

    extern int pointer_charstr(int row, int col, const char *string_array, const char *string);

    extern int index_string(const char *src, char key);

    extern double freqbysys(int isys, int ifreq, const char *freq);

    extern int read_siteinfo(string fil, string name, double *x);

    extern long double GetX(double RadLat, double eSquare, double a);

    extern bool bl2Gaussxy(double RadLat, double RadLon, double *x, double *y,
                           double RadLon0, double eSquare, double a, double heightchange);

    extern unsigned long CRC24(long size, const unsigned char *buf);

    extern uint32_t getbitu(const uint8_t *buff, int pos, int len);

    extern int32_t getbits(const uint8_t *buff, int pos, int len);

    extern void setbitu(uint8_t *buff, int pos, int len, uint32_t data);

    extern void setbits(uint8_t *buff, int pos, int len, int32_t data);

    extern uint32_t rtk_crc24q(const uint8_t *buff, int len);

    extern char *trim(char *pStr);

    extern void excludeAnnoValue(char *value, const char *in);

    extern string zipJson(string json);

    extern int len_trim(const char *pStr);

    extern void get_compile_date_base(uint8_t *Year, uint8_t *Month, uint8_t *Day);

    extern char *get_compile_date(char *g_date_buf);

    extern char *substringEx(char *dest, const char *src, int start, int length);

    extern void split_string(bool lnoempty, char *string, char c_start, char c_end,
                             char seperator, int *nword, char *keys, int len);

    extern void decodetcppath(const char *path, char *addr, char *port, char *user,
                              char *passwd, char *mntpnt, char *str);

    extern void s2ipp(double *pos, double azim, double elev, double r, double hion, double *posp);

    extern bool bl2Gaussxy(double RadLat, double RadLon, double *x, double *y,
                           double RadLon0, double eSquare, double a, double heightchange);

    extern void fillobs(char *line, int nobs, int itemlen, double ver);

    extern void filleph(char *line, double ver);

    extern void yr2year(int &yr);

    extern double timdif(int jd2, double sod2, int jd1, double sod1);

    extern void brdtime(char *cprn, int *mjd, double *sod);

    extern int genAode(char csys, int mjd, double sod, double toe, int inade);

    extern void xyzblh(double *x, double scale, double a0, double b0, double dx, double dy,
                       double dz, double *geod);

    extern void rot_enu2xyz(double lat, double lon, double (*rotmat)[3]);

    extern void xyzblh(double *x, double scale, double a0, double b0, double dx, double dy,
                       double dz, double *geod);

    extern void blhxyz(double *geod, double a0, double b0, double *x);

    double m2stec(double delay, double fq);

    void callExit();

} // namespace iono
#endif /* INCLUDE_COM_COM_H_ */
