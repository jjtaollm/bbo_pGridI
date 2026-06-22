/*
 * Satellite.h
 *
 *  Created on: 2018/8/26
 *      Author: juntao, at wuhan university
 */

#ifndef INCLUDE_BAMBOO_SATELLITE_H_
#define INCLUDE_BAMBOO_SATELLITE_H_
#include <iostream>
#include <map>
#include <string>
#include "include/Frame/Config/Const.h"
using namespace std;
namespace iono
{
    class Satellite
    {
    public:
        Satellite();
        ~Satellite();
        char cprn[LEN_PRN];      /* PRN for satellite, G01/C01/C19/E01 ... */
        char prn_alias[LEN_PRN]; /* prn alias name */
        double offs[3];          /* */
        double xyz[MAXFREQ][3];  /* PCO for each frequency */
        double xscf[3];          /* attitude transformation unit vector for x*/
        double yscf[3];          /* attitude transformation unit vector for y*/
        double zscf[3];          /* attitude transformation unit vector for z*/
        double lamda[MAXFREQ];   /* lambda in meters for each frequency */
        double freq[MAXFREQ];    /* frequency for each frequency*/
        int ifreq;               /* ifreq for glonass */
        double fac[2];           /* ionosphere free combination coefficient, f1*f1 / (f1*f1 - f2*f2); f2 * f2 / (f1 * f1 - f2*f2)*/
        double mass;             /* mass of this satellite */
        double satpos[6];        /* current position for iterated time */
        double xclk;             /* current satellite clock for iterated time */
    };
} // namespace bamboo
#endif /* INCLUDE_BAMBOO_SATELLITE_H_ */
