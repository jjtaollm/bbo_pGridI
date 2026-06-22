/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-28 20:14:03
 * @ Modified by: Jun Tao
 * @ Modified time: 2024-12-07 15:25:00
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */

#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <functional>
#include <unistd.h>
#include <algorithm>
using namespace std;
using namespace iono;
Deploy::Deploy()
{
    tracelevel = 0;
    nprn = 0;
    nsys = 0;
    iref = -1;
    lpost = 0;
    ltrace = 0;
    ldebug = 0;
    lg = 2.0;
    lw = 2.0;
    gap = 1200;
    dtec = 0.176;
    mjd = mjd0 = mjd1 = 0;
    sod = sod0 = sod1 = 0;
    t_ogrid_intv = t_opoly_intv = 1;
    memset(m_mode, 0, sizeof(m_mode));
    memset(system, 0, sizeof(system));
    memset(freq, 0, sizeof(freq));
    memset(dcbfile, 0, sizeof(dcbfile));
    memset(nfreq, 0, sizeof(nfreq));
    strcpy(this->outdir, "./");
}
Deploy::~Deploy()
{
}
void Deploy::m_printUsage()
{
    cout << "GNSS ionosphere real-time & post data process program@jtaowhu" << endl;
    cout << "     usage : [POST MODE] iono -mode POL_SD+INTERP/POL_SD/INTERP files.json -pams -outpoly[=10] -outgrid[=10] -outpolyres -skipR (do not contain request input)" << endl;
    cout << "     usage : [Rt   MODE] iono -mode POL_SD+INTERP/POL_SD/INTERP -intv 5 -url $url -pams -outpoly[=10] -outgrid[=10] " << endl;
    cout << "Compiled on: " << __DATE__ << " at " << __TIME__ << std::endl;
    cout << "Contact: jtaowhu@whu.edu.cn" << endl;
    exit(1);
}
void Deploy::m_parse_config(string config)
{
    int nprn = 0;
    char line[1024] = {0}, prn[8] = {0}, _prn_alias[8] = {0}, sitn[8] = {0};
    FILE *fp = NULL;
    if (!(fp = fopen(config.c_str(), "r")))
    {
        LOGPRINT_EX("file = %s, cant open config file", config.c_str());
        exit(1);
    }
    /* for satellite */
    bool b_start = false;
    while (!feof(fp))
    {
        fgets(line, 1024, fp);
        if (strstr(line, "+Satellite used"))
        {
            b_start = true;
            continue;
        }
        if (strstr(line, "-Satellite used"))
            break;
        if (line[0] == ' ' && b_start)
        {
            sscanf(line, "%s", prn);
            if (-1 != pointer_string(nprn, this->cprn, string(prn)))
            {
                LOGPRINT_EX("cprn = %s,exist the same satellite", prn);
                continue;
            }
            strcpy(_prn_alias, prn);
            if (prn[0] == 'C' && atoi(prn + 1) < 19)
                _prn_alias[0] = 'B';
            cprn[nprn] = string(prn);
            prn_alias[nprn] = string(_prn_alias);

            Satellite sat;
            strcpy(sat.cprn, cprn[nprn].c_str());
            strcpy(sat.prn_alias, prn_alias[nprn].c_str());
            SAT.push_back(sat);

            if (-1 == (index_string(this->system, _prn_alias[0])))
            {
                this->system[this->nsys] = _prn_alias[0];
                this->nsys = this->nsys + 1;
            }
            ++nprn;
        }
    }
    this->nprn = nprn;
    this->iref = index_string(SYS, this->system[0]);
    /* for station */
    b_start = false;
    while (!feof(fp))
    {
        fgets(line, 1024, fp);
        if (strstr(line, "+Station used"))
        {
            b_start = true;
            continue;
        }
        if (strstr(line, "-Station used"))
            break;
        if (line[0] == ' ' && b_start)
        {
            sscanf(line, "%s", sitn);
            if (mSta.find(sitn) != mSta.end())
            {
                LOGPRINT_EX("cprn = %s,exist the same station", sitn);
                continue;
            }
            Station sta;
            sta.name = sitn;
            mSta[sitn] = sta;
        }
    }
    fclose(fp);
}
void Deploy::m_assign_coor(string sitf)
{
    for (auto &kv : mSta)
    {
        read_siteinfo(sitf, kv.second.name, kv.second.x);
        ecef2pos(kv.second.x, kv.second.geod);
    }

    FILE *fp = fopen("Stablh", "w");
    /* update gaussx coordinates */
    for (auto &kv : mSta)
    {
        fprintf(fp, "%s %9.4lf %9.4lf %9.4lf\n", kv.first.c_str(), kv.second.geod[0] * R2D, kv.second.geod[1] * R2D, kv.second.geod[2]);
    }
    fclose(fp);
}
void Deploy::m_parse_request(string sitf)
{
    char buff[1024] = {0}, name[8] = {0};
    FILE *fp = fopen(sitf.c_str(), "r");
    if (fp == NULL)
    {
        LOGPRINT("file = %s,can't open file to read!", sitf.c_str());
        return;
    }
    mPostReqs.clear();
    while (!feof(fp))
    {
        Station sta;
        memset(buff, 0, sizeof(buff));
        fgets(buff, 1024, fp);
        if (strlen(buff) < 10)
            continue;
        sscanf(buff, "%s%lf%lf%lf", name, &sta.x[0], &sta.x[1], &sta.x[2]);
        sta.name = name;
        ecef2pos(sta.x, sta.geod);
        mPostReqs[name] = sta;
    }
    fclose(fp);
}
void Deploy::m_readJsonFile(int argc, char *args[])
{
    //
    Json::CharReaderBuilder rbuilder;
    rbuilder["collectComments"] = false;
    Json::Value root;
    JSONCPP_STRING errs;
    fstream f;
    double dsec;
    int lhelp = 0, iy, im, id, ih, imin, isys;
    char config[1024] = {0}, reqf[1024] = {0}, sitf[1024] = {0}, value[256] = {0};
    string _jsonFile;
    for (int iargc = 1; iargc < argc; iargc++)
    {
        if (strstr(args[iargc], "-mode"))
        {
            strcpy(m_mode, args[++iargc]);
            _jsonFile = string(args[++iargc]);
        }
        if (strstr(args[iargc], "-post"))
            lpost = true;
        if (strstr(args[iargc], "-debug"))
            ldebug = true;

        if (!strncmp(args[iargc], "-help", 5))
            lhelp = true;
        if (strstr(args[iargc], "-pams"))
        {
            for (int ic = iargc + 1; ic < argc; ++ic)
                sprintf(m_addargs + strlen(m_addargs), "%s ", args[ic]);
            iargc = argc;
        }
    }

    if (strstr(m_addargs, "-outgrid="))
        sscanf(strstr(m_addargs, "-outgrid="), "-outgrid=%d", &this->t_ogrid_intv);

    if (strstr(m_addargs, "-outpoly="))
        sscanf(strstr(m_addargs, "-outpoly="), "-outpoly=%d", &this->t_opoly_intv);

    if (lhelp)
    {
        m_printUsage();
        exit(1);
    }
    f.open(_jsonFile.c_str(), ios::in);
    if (!f.is_open())
    {
        LOGPRINT_EX("file = %s,can't open file to read!\n", _jsonFile.c_str());
        exit(1);
    }
    bool parse_ok = Json::parseFromStream(rbuilder, f, &root, &errs);
    f.close();
    if (!parse_ok)
    {
        LOGPRINT_EX("file = %s,can't parse file!\n", _jsonFile.c_str());
        exit(1);
    }
    try
    {
        /* */
        excludeAnnoValue(config,
                         root["files"]["configf"].asCString());
        excludeAnnoValue(sitf,
                         root["files"]["coorf"].asCString());
        excludeAnnoValue(reqf,
                         root["files"]["reqf"].asCString());
        excludeAnnoValue(dcbfile,
                         root["files"]["siteDCBf"].asCString());
        /* path */
        excludeAnnoValue(externaldir,
                         root["path"]["external_input"].asCString());
        excludeAnnoValue(tracedir,
                         root["path"]["trace"].asCString());

        excludeAnnoValue(obsdir,
                         root["path"]["obsdir"].asCString());

        excludeAnnoValue(ephdir,
                         root["path"]["ephdir"].asCString());

        excludeAnnoValue(outdir,
                         root["path"]["output"].asCString());

        /* config */
        excludeAnnoValue(value, root["config"]["time"].asCString());
        sscanf(value, "%d%d%d%d%d%lf%lf", &iy, &im, &id, &ih, &imin, &dsec,
               &seslen);
        this->mjd0 = ymd2mjd(iy, im, id);
        this->sod0 = ih * 3600 + imin * 60 + dsec;
        this->mjd1 = (int)(this->mjd0 + (this->sod0 + seslen) / 86400);
        this->sod1 =
            this->sod0 + seslen - (this->mjd1 - this->mjd0) * 86400.0;
        excludeAnnoValue(value, root["config"]["intv"].asCString());
        sscanf(value, "%lf", &dintv);

        excludeAnnoValue(value, root["config"]["turboedit"].asCString());
        sscanf(value, "%lf%lf%lf%lf", &lg, &lw, &gap, &dtec);

        /* parse config */
        m_parse_config(config);

        /* get station coordinates */
        m_assign_coor(sitf);

        /* get requests */
        m_parse_request(reqf);

        /* assign frequency */
        for (int jsys = 0; jsys < this->nsys; ++jsys)
        {
            isys = index_string(SYS, system[jsys]);
            switch (SYS[isys])
            {
            case 'G':
                nfreq[isys] = 2;
                strcpy(freq[isys][0], "L1"); // L1
                strcpy(freq[isys][1], "L2"); // L2
                break;
            case 'E':
                nfreq[isys] = 2;
                strcpy(freq[isys][0], "L1"); // E1
                strcpy(freq[isys][1], "L5"); // E5a
                break;
            case 'C':
                nfreq[isys] = 2;
                strcpy(freq[isys][0], "L2"); // B1I
                strcpy(freq[isys][1], "L6"); // B3I
                break;
            case 'B':
                nfreq[isys] = 2;
                strcpy(freq[isys][0], "L2"); // B1I
                strcpy(freq[isys][1], "L6"); // B3I
                break;
            }
        }

        /* OPEN LOG FILE */
        sprintf(statfile, "%s%cbamboo.stat", tracedir, FILEPATHSEP);
        if (access(tracedir, 0) != 0)
        {
            LOGPRINT_EX("tracedir = %s,trace dir is not exist!", tracedir);
            callExit();
        }
        Logtrace::s_defaultlogger.m_openLog(statfile, true, 7200);
    }
    catch (...)
    {
        LOGPRINT_EX("item = %s,failed to parse item", _jsonFile.c_str());
        exit(1);
    }
}

/* assign default configures */
void Deploy::m_assign_default(int argc, char *args[])
{
    int lhelp = 0, ltracedir = 0, lexterndir = 0, loutdir = 0;
    string _jsonFile;
    lpost = false;
    this->dintv = 10;
    for (int iargc = 1; iargc < argc; iargc++)
    {
        if (strstr(args[iargc], "-mode"))
        {
            strcpy(m_mode, args[++iargc]);
        }
        if (strstr(args[iargc], "-url"))
        {
            /* saving the ion url here */
            this->iono_url = args[++iargc];
        }
        if (strstr(args[iargc], "-tracedir"))
        {
            ltracedir = true;
            strcpy(tracedir, args[++iargc]);
        }
        if (strstr(args[iargc], "-outdir"))
        {
            loutdir = true;
            strcpy(outdir, args[++iargc]);
        }
        if (strstr(args[iargc], "-intv"))
        {
            this->dintv = atof(args[++iargc]);
            LOGPRINT_EX("interval: %lf", dintv);
        }
        if (strstr(args[iargc], "-externdir"))
        {
            lexterndir = true;
            strcpy(externaldir, args[++iargc]);
        }
        if (!strncmp(args[iargc], "-help", 5))
            lhelp = true;
        if (strstr(args[iargc], "-pams"))
        {
            for (int ic = iargc + 1; ic < argc; ++ic)
                sprintf(m_addargs + strlen(m_addargs), "%s ", args[ic]);
            iargc = argc;
        }
    }
    if (lhelp)
    {
        m_printUsage();
        exit(1);
    }

    if (strstr(m_addargs, "-outgrid="))
        sscanf(strstr(m_addargs, "-outgrid="), "-outgrid=%d", &this->t_ogrid_intv);

    if (strstr(m_addargs, "-outpoly="))
        sscanf(strstr(m_addargs, "-outpoly="), "-outpoly=%d", &this->t_opoly_intv);

    int isys;
    char prn[8] = {0};

    /* step 1, initial cprn */
    nprn = 0;
    for (int i = 0; i < 32; ++i) /* GPS */
    {
        sprintf(prn, "G%02d", i + 1);
        this->cprn[nprn] = prn;
        this->prn_alias[nprn] = prn;

        Satellite sat;
        strcpy(sat.cprn, cprn[nprn].c_str());
        strcpy(sat.prn_alias, prn_alias[nprn].c_str());
        SAT.push_back(sat);

        ++nprn;
    }

    for (int i = 5; i < 16; ++i) /* BD2*/
    {
        sprintf(prn, "C%02d", i + 1);
        this->cprn[nprn] = prn;
        sprintf(prn, "B%02d", i + 1);
        this->prn_alias[nprn] = prn;

        Satellite sat;
        strcpy(sat.cprn, cprn[nprn].c_str());
        strcpy(sat.prn_alias, prn_alias[nprn].c_str());
        SAT.push_back(sat);

        ++nprn;
    }

    for (int i = 19; i < 46; ++i) /* BD3*/
    {
        sprintf(prn, "C%02d", i + 1);
        this->cprn[nprn] = prn;
        this->prn_alias[nprn] = prn;

        Satellite sat;
        strcpy(sat.cprn, cprn[nprn].c_str());
        strcpy(sat.prn_alias, prn_alias[nprn].c_str());
        SAT.push_back(sat);
        ++nprn;
    }

    for (int i = 0; i < 36; ++i) /* GAL */
    {
        sprintf(prn, "E%02d", i + 1);
        this->cprn[nprn] = prn;
        this->prn_alias[nprn] = prn;

        Satellite sat;
        strcpy(sat.cprn, cprn[nprn].c_str());
        strcpy(sat.prn_alias, prn_alias[nprn].c_str());
        SAT.push_back(sat);

        ++nprn;
    }

    /* step 2, initla system */
    strcpy(system, "GECB");
    this->nsys = 4;

    /* assign dirs */
    if (!ltracedir)
        strcpy(tracedir, "./");
    if (!lexterndir)
        strcpy(externaldir, "./");
    if (!loutdir)
        strcpy(outdir, "./");

    /* assign frequency */
    for (int jsys = 0; jsys < this->nsys; ++jsys)
    {
        isys = index_string(SYS, system[jsys]);
        switch (SYS[isys])
        {
        case 'G':
            nfreq[isys] = 2;
            strcpy(freq[isys][0], "L1"); // L1
            strcpy(freq[isys][1], "L2"); // L2
            break;
        case 'E':
            nfreq[isys] = 2;
            strcpy(freq[isys][0], "L1"); // E1
            strcpy(freq[isys][1], "L5"); // E5a
            break;
        case 'C':
            nfreq[isys] = 2;
            strcpy(freq[isys][0], "L2"); // B1I
            strcpy(freq[isys][1], "L6"); // B3I
            break;
        case 'B':
            nfreq[isys] = 2;
            strcpy(freq[isys][0], "L2"); // B1I
            strcpy(freq[isys][1], "L6"); // B3I
            break;
        }
    }

    /* OPEN LOG FILE */
    sprintf(statfile, "%s%cbamboo.stat", tracedir, FILEPATHSEP);
    if (access(tracedir, 0) != 0)
    {
        LOGPRINT_EX("tracedir = %s,trace dir is not exist!", tracedir);
        callExit();
    }
    Logtrace::s_defaultlogger.m_openLog(statfile, true, 7200);
}

void Deploy::m_updateSit(map<string, Station> &stalist)
{
    mSta = stalist;
}