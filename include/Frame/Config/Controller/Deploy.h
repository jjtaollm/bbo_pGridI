#ifndef INCLUDE_FRAME_DEPLOY_H_
#define INCLUDE_FRAME_DEPLOY_H_
#include <string>
#include <iostream>
#include <cstdlib>
#include <stdlib.h>
#include <vector>
#include <map>
#include <fstream>
#include "include/Frame/Config/Const.h"
#include "include/Frame/Config/Controller/Satellite.h"
#include "include/Frame/Config/Controller/Station.h"
using namespace std;
namespace iono
{
    class Deploy
    {
    public:
        Deploy();
        ~Deploy();
        bool lpost;
        bool ltrace;
        bool ldebug;
        int mjd;
        int mjd0;
        int mjd1;
        double sod;
        double sod0;
        double sod1;
        double seslen;
        double dintv;
        int nprn;
        string cprn[MAXSAT];
        string prn_alias[MAXSAT];
        char freq[MAXSYS][MAXFREQ][3];
        int nfreq[MAXSYS];
        char system[MAXSYS];
        int nsys;
        int tracelevel;
        char m_mode[256];
        char m_addargs[1024]; /* added parameters configures */
        char tracefile[256];  /* trace log file */
        char statfile[256];   /* stat file */
        char dcbfile[256];    /* DCB FILE */
        char externaldir[256];
        char tracedir[256];
        map<string, Station> mSta;
        vector<Satellite> SAT;          /* Satellite configure */
        map<string, Station> mPostReqs; /* post requests */
        string iono_url;                /* real-time iono modeling url */
        int iref;
        double gap; /* qc parameters*/
        double lg;
        double lw;
        double dtec;
        char obsdir[256];
        char ephdir[256];
        char outdir[256];

        int t_ogrid_intv;
        int t_opoly_intv;

    public:
        void m_printUsage();
        void m_readJsonFile(int argc, char *args[]);
        void m_assign_default(int, char *[]);
        void m_parse_config(string config);
        void m_assign_coor(string sitf);
        void m_parse_request(string sitf);
        void m_updateSit(map<string, Station> &stalist);
    };
}
#endif