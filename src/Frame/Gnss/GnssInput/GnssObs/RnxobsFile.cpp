/*
 * RnxobsFile.cpp
 *
 *  Created on: 2018/2/4/
 *      Author: doublestring
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace iono;
using namespace std;

// Done at 8.26
RnxobsFile::RnxobsFile(string name) : Rnxobs(name)
{
    this->staname = name;
    intv = 1.0;
    ver = 2.0;
    x = y = z = e = n = h = 0.0;
    nprn = fact1 = fact2 = nsys = 0;
}
void RnxobsFile::v_openRnx(string obscmd)
{
    int curmjd;
    double cursod, ep[6] = {0};
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    if (this->in.is_open())
        this->in.close();
    char obsFile[1024] = {0}, key[256];
    curmjd = atoi(obscmd.substr(0, index_string(obscmd.c_str(), ':')).c_str());
    cursod = atof(obscmd.substr(index_string(obscmd.c_str(), ':') + 1).c_str());

    string name = staname;
    time2epoch(mjd2time(curmjd, cursod), ep);
    int idoy = mjd2doy(curmjd, NULL);

    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    sprintf(obsFile, "%s%c/%s%03d0.%02do", dly->obsdir, FILEPATHSEP, name.c_str(), idoy, ep[0] > 2000 ? NINT(ep[0] - 2000) : NINT(ep[0] - 1900));
    this->dir = string(obsFile);
    this->in.open(this->dir.c_str(), ios::in | ios::binary);
    if (!in)
    {
        cout << "***WARNING(RnxobsFile):cant open file " << this->dir
             << " to read!" << endl;
        return;
    }
    // Read Rnxobs Head
    this->m_readRnxHead();
}
void RnxobsFile::v_closeRnx()
{
    if (this->in.is_open())
        this->in.close();
    memset(tstore, 0, sizeof(tstore));
    // memset(tstore,0,sizeof(int) * MAXSAT * MAXFREQ);
}
void RnxobsFile::v_readEpoch(int &mjd, double &sod)
{
    Rnxobs::v_readEpoch(mjd, sod);
    int lfind, nprn, iflag, i, j, k, nline, ntemp;
    int iy, im, id, ih, imi, isat, isys, isys_header, isys_store, iobs;
    double sec, ds;
    streampos pos;
    char cline[1024], code[4];
    string line, varstr;
    double obt[MAXOBSTYP];
    if (!(this->in))
    {
        memset(obs, 0, sizeof(double) * MAXSAT * 2 * MAXFREQ);
        this->mjd = mjd;
        this->sod = sod;
        return;
    }
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    // Read Rnx
    lfind = false;
    while (!lfind && !in.eof())
    {
        pos = in.tellg();
        getline(in, line);
        if (line == "" || !len_trim(line.c_str()))
            continue;
        if (ver < 3.0)
        {
            nprn = atoi(line.substr(29, 3).c_str());
            iflag = atoi(line.substr(26, 3).c_str());
        }
        else
        {
            nprn = atoi(line.substr(32, 3).c_str());
            iflag = atoi(line.substr(29, 3).c_str());
        }
        if (nprn > MAXSAT)
        {
            LOGPRINT("nprn = %d,MAXSAT = %d,staname = %s,station's satellite number is bigger than MAXSAT,possible is the readEpoch iflag!",
                     nprn, MAXSAT, staname.c_str());
            exit(1);
        }
        // station moved
        if (iflag > 1)
        {
            for (i = 0; i < nprn; i++)
            { // nprn indicate the nline
                getline(in, line);
                if (strstr(line.c_str(), "ANTENNA: DELTA H/E/N"))
                {
                    cout << "***INFO(readEpoch):Changing the ENU of the station"
                         << staname << endl;
                    sscanf(line.c_str(), "%lf%lf%lf", &h, &e, &n);
                }
            }
            continue;
        }
        // check if the format is correct
        if (ver >= 3.0)
            sscanf(line.c_str(), "%*s%d%d%d%d%d%lf", &iy, &im, &id, &ih, &imi,
                   &sec);
        else
            sscanf(line.c_str(), "%d%d%d%d%d%lf", &iy, &im, &id, &ih, &imi,
                   &sec);
        yr2year(iy);
        if (iy < 2000 || iy > 3500)
        {
            LOGPRINT("iy = %d,staname = %s,reading time error,possible is the readEpoch iflag!", iy, staname.c_str());
            exit(1);
        }
        this->mjd = ymd2mjd(iy, im, id);
        this->sod = ih * 3600.0 + imi * 60 + sec; // NINT(ih * 3600.0 + imi * 60 + sec);
        if (ver < 3.0)
        {
            if (nprn % 12 != 0)
                nline = (int)(nprn / 12.0) + 1;
            else
                nline = nprn / 12;
            if (nprn == 0)
                nline = 1;
        }
        ds = timdif(mjd, sod, this->mjd, this->sod);
        if (ds < -MAXWND)
        {
            // the requested time is before the current time in the obsfile
            in.seekg(pos);
            this->mjd = mjd;
            this->sod = sod;
            return;
        }
        else if (ds > MAXWND)
        {
            // keep tracing until find the corresponding observation
            if (ver < 3.0)
            {
                for (i = 0; i < nline - 1; i++)
                    getline(in, line);
                if (nobstype[0] % 5 != 0)
                    ntemp = nprn * ((int)(nobstype[0] / 5) + 1);
                else
                    ntemp = nprn * (nobstype[0] / 5);
                for (i = 0; i < ntemp; i++)
                    getline(in, line);
            }
            else
            {
                for (i = 0; i < nprn; i++)
                    getline(in, line);
            }
        }
        else
        {
            lfind = true;
            mjd = this->mjd;
            sod = ih * 3600.0 + imi * 60.0 + sec;
            this->sod = sod;
        }
    }
    if (in.eof())
    {
        this->mjd = mjd;
        this->sod = sod;
        return;
    }
    if (ver < 3.0)
    {
        // get rinex 2.x version observation data
        in.seekg(pos);
        for (i = 0; i < nline; i++)
        {
            getline(in, line);
            for (j = 0; j < MIN(nprn - 12 * i, 12); j++)
            {
                cprn_ob[j + i * 12] = line.substr(32 + j * 3, 3);
                if (cprn_ob[j + i * 12][0] == ' ')
                    cprn_ob[j + i * 12][0] = 'G';
                if (cprn_ob[j + i * 12][1] == ' ')
                    cprn_ob[j + i * 12][1] = '0';
            }
        }
        if (nobstype[0] % 5 != 0)
            nline = (int)(nobstype[0] / 5.0) + 1;
        else
            nline = nobstype[0] / 5;
        for (i = 0; i < nprn; i++)
        {
            for (j = 0; j < nline; j++)
            {
                getline(in, line);
                strcpy(cline, line.c_str());
                fillobs(cline, MIN(nobstype[0] - j * 5, 5), 16, ver);
                varstr = string(cline);
                for (k = 0; k < MIN(nobstype[0] - j * 5, 5); k++)
                {
                    obt[k + j * 5] = 0.0;
                    obt[k + j * 5] = atof(varstr.substr(16 * k, 14).c_str());
                }
            }
            isat = pointer_string(dly->nprn, dly->cprn, this->cprn_ob[i]);
            if (isat != -1)
            {
                // isys = SYS.find_first_of(this->cprn_ob[i][0]);
                isys_header = index_string(SYS, this->cprn_ob[i][0]);
                isys = index_string(SYS, dly->prn_alias[isat][0]);

                for (j = 0; j < dly->nfreq[isys]; j++)
                {
                    code[0] = 'P';
                    code[1] = dly->freq[isys][j][1];
                    code[2] = '\0';
                    k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                    if (k != -1)
                    {
                        this->obs[isat][MAXFREQ + j] = obt[k];
                        code[0] = 'C';
                        code[1] = dly->freq[isys][j][1];
                        code[2] = 'P';
                        code[3] = '\0';
                        memcpy(this->fob[isat][MAXFREQ + j], code, sizeof(char) * 4);
                    }
                    if (this->obs[isat][MAXFREQ + j] == 0)
                    {
                        code[0] = 'C';
                        code[1] = dly->freq[isys][j][1];
                        code[2] = '\0';

                        k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                        if (k != -1)
                        {
                            this->obs[isat][MAXFREQ + j] = obt[k];
                            code[0] = 'C';
                            code[1] = dly->freq[isys][j][1];
                            code[2] = 'C';
                            code[3] = '\0';
                            memcpy(this->fob[isat][MAXFREQ + j], code, sizeof(char) * 4);
                        }
                    }
                    code[0] = 'L';
                    code[1] = dly->freq[isys][j][1];
                    code[2] = '\0';
                    k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                    if (k != -1)
                    {
                        this->obs[isat][j] = obt[k];
                        code[0] = 'L';
                        code[1] = dly->freq[isys][j][1];
                        code[2] = 'P';
                        code[3] = '\0';
                        memcpy(this->fob[isat][j], code, sizeof(char) * 4);
                    }
                    /////////////////////// dop observations//////////////
                    code[0] = 'D';
                    code[1] = dly->freq[isys][j][1];
                    code[2] = '\0';
                    k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                    if (k != -1)
                    {
                        this->dop[isat][j] = obt[k];
                    }
                }
                s_checkObs(isat, obs[isat], dop[isat], snr[isat]);
            }
        }
    }
    else
    {
        // get rinex 3.x version observation data
        for (i = 0; i < nprn; i++)
        {
            getline(in, line);
            // isys = SYS.find_first_of(line[0]);
            isys_header = index_string(SYS, line[0]);
            if (isys_header == -1)
                continue;
            if (line[1] == ' ')
                line[1] = '0';
            isat = pointer_string(dly->nprn, dly->cprn, line.substr(0, 3));
            if (isat == -1)
                continue;
            isys = index_string(SYS, dly->prn_alias[isat][0]);
            strcpy(cline, line.substr(3, line.size() - 3).c_str());
            fillobs(cline, nobstype[isys_header], 16, ver);
            varstr = line.substr(0, 3) + string(cline);
            for (j = 0; j < nobstype[isys_header]; j++)
            {
                obt[j] = atof(varstr.substr(3 + j * 16, 14).c_str());
            }
            isys_store = SYS[isys] == 'B' ? index_string(SYS, 'B') : isys_header; /// extra situation for BDS-2
            for (j = 0; j < dly->nfreq[isys]; j++)
            {
                if (!(this->tstore[isys_store][j]))
                {
                    for (iobs = 0; iobs < strlen(OBSTYPE[isys_store][j]); iobs++)
                    {
                        code[0] = 'C';
                        code[1] = dly->freq[isys][j][1];
                        code[2] = OBSTYPE[isys_store][j][iobs];
                        code[3] = '\0';
                        k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                        if (k != -1)
                        {
                            obs[isat][MAXFREQ + j] = obt[k];
                            memcpy(this->fob[isat][MAXFREQ + j], code, sizeof(char) * 4);
                        }
                        code[0] = 'L';
                        code[1] = dly->freq[isys][j][1];
                        code[2] = OBSTYPE[isys_store][j][iobs];
                        code[3] = '\0';
                        k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                        if (k != -1)
                        {
                            obs[isat][j] = obt[k];
                            memcpy(this->fob[isat][j], code, sizeof(char) * 4);
                        }

                        code[0] = 'S';
                        code[1] = dly->freq[isys][j][1];
                        code[2] = OBSTYPE[isys_store][j][iobs];
                        code[3] = '\0';
                        k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                        if (k != -1)
                        {
                            snr[isat][j] = obt[k];
                        }
                        code[0] = 'D';
                        code[1] = dly->freq[isys][j][1];
                        code[2] = OBSTYPE[isys_store][j][iobs];
                        code[3] = '\0';
                        k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                        if (k != -1)
                        {
                            dop[isat][j] = obt[k];
                        }
                        if (obs[isat][j] != 0 && obs[isat][MAXFREQ + j] != 0)
                            break;
                    }
                    if (iobs < strlen(OBSTYPE[isys_store][j]))
                    {
                        usetype[isys_store][j] = OBSTYPE[isys_store][j][iobs];
                        tstore[isys_store][j] = 1;
                        LOGPRINT_EX("[RNXOBS] %s %s %c %c selected!", staname.c_str(), dly->freq[isys_store][j], SYS[isys_store], usetype[isys_store][j]);
                    }
                }
                else
                {
                    code[0] = 'C';
                    code[1] = dly->freq[isys][j][1];
                    code[2] = this->usetype[isys_store][j];
                    // code[2] = this->usetype[isat][j];
                    code[3] = '\0';
                    k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                    if (k != -1)
                    {
                        obs[isat][MAXFREQ + j] = obt[k];
                        memcpy(this->fob[isat][MAXFREQ + j], code, sizeof(char) * 4);
                    }
                    code[0] = 'L';
                    code[1] = dly->freq[isys][j][1];
                    code[2] = this->usetype[isys_store][j];
                    // code[2] = this->usetype[isat][j];
                    code[3] = '\0';
                    k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                    if (k != -1)
                    {
                        obs[isat][j] = obt[k];
                        memcpy(this->fob[isat][j], code, sizeof(char) * 4);
                    }
                    code[0] = 'S';
                    code[1] = dly->freq[isys][j][1];
                    code[2] = this->usetype[isys_store][j];
                    code[3] = '\0';

                    k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                    if (k != -1)
                    {
                        snr[isat][j] = obt[k];
                    }

                    code[0] = 'D';
                    code[1] = dly->freq[isys][j][1];
                    code[2] = this->usetype[isys_store][j];
                    code[3] = '\0';

                    k = pointer_string(nobstype[isys_header], obstype[isys_header], string(code));
                    if (k != -1)
                    {
                        dop[isat][j] = obt[k];
                    }
                }
            }
            s_checkObs(isat, obs[isat], dop[isat], snr[isat]);
        }
    }
}
void RnxobsFile::m_readRnxHead()
{
    int isys, id, i, j, nline;
    double sec;
    string line, strflag;
    char cline[256], tmetag[256];
    this->nsys = 0;
    memset(this->nobstype, 0, sizeof(int) * MAXSYS);
    streampos pos;
    while (getline(this->in, line))
    {
        strflag = line.substr(60, line.size() - 60);
        const char *sflag = strflag.c_str();
        if (strstr(sflag, "END OF HEADER") != NULL)
            break;
        if (strstr(sflag, "COMMENT"))
        {
            pos = in.tellg();
            continue;
        }
        if (strstr(sflag, "RINEX VERSION"))
        {
            this->ver = atof(line.substr(0, 9).c_str());
            this->sys[0] = line[40];
            if (this->sys[0] == ' ')
                this->sys[0] = 'G';
            if (this->ver < 1.0 || this->ver > 4.0)
            {
                cout << "Read Rinex Version Error!Try To Read...\n";
                exit(1);
            }
        }
        if (strstr(sflag, "MARKER NAME"))
        {
            this->mark = line.substr(0, 4);
        }
        if (strstr(sflag, "REC #"))
        {
            this->recnum = line.substr(0, 20);
            this->rectype = line.substr(20, 20);
        }
        if (strstr(sflag, "ANT #"))
        {
            this->antnum = line.substr(0, 20);
            this->anttype = line.substr(20, 20);
        }
        if (strstr(sflag, "APPROX POSITION"))
        {
            sscanf(line.c_str(), "%lf%lf%lf", &x, &y, &z);
        }
        if (strstr(sflag, "ANTENNA: DELTA"))
        {
            sscanf(line.c_str(), "%lf%lf%lf", &h, &e, &n);
        }
        if (strstr(sflag, "WAVELENGTH FACT"))
        {
        }
        if (strstr(sflag, "SYS / # / OBS TYPES"))
        {
            // id = SYS.find_first_of(line[0]);
            id = index_string(SYS, line[0]);
            if (-1 == id)
            {
                pos = in.tellg();
                continue;
            }
            this->nobstype[id] = atoi(line.substr(3, 3).c_str());
            if (nobstype[id] % 13 != 0)
                nline = (int)(nobstype[id] / 13) + 1;
            else
            {
                if (nobstype[id] == 0)
                    nline = 1;
                else
                    nline = (int)(nobstype[id] / 13);
            }
            // maybe can use fseek
            in.seekg(pos);
            for (j = 0; j < nline; j++)
            {
                getline(in, line);
                for (i = 0; i < (MIN((nobstype[id] - 13 * j), 13)); i++)
                {
                    obstype[id][i + 13 * j] = line.substr(7 + i * 4, 3);
                }
            }
            // if (ver >= 3.02 && id == index_string(SYS, 'C') && ver < 3.04)
            // {
            // 	for (i = 0; i < nobstype[id]; i++)
            // 	{
            // 		if (strstr(obstype[id][i].c_str(), "C1") != NULL || strstr(obstype[id][i].c_str(), "L1") != NULL || strstr(obstype[id][i].c_str(), "S1") != NULL || strstr(obstype[id][i].c_str(), "D1") != NULL)
            // 			obstype[id][i][1] = '2';
            // 	}
            // }
        }
        if (strstr(sflag, "TYPES OF OBSERV"))
        {
            nobstype[0] = atoi(line.substr(0, 6).c_str());
            if (nobstype[0] == 0)
            {
                pos = in.tellg();
                continue;
            }
            if (nobstype[0] % 9 != 0)
                nline = (int)(nobstype[0] / 9) + 1;
            else
                nline = (int)(nobstype[0] / 9);
            in.seekg(pos);
            for (j = 0; j < nline; j++)
            {
                getline(in, line);
                for (i = 0; i < (MIN((nobstype[0] - 9 * j), 9)); i++)
                {
                    sscanf(line.substr(6 + 6 * i, 6).c_str(), "%s", cline);
                    obstype[0][i + 9 * j] = string(cline);
                }
            }
            for (i = 1; i < MAXSYS; i++)
            {
                nobstype[i] = nobstype[0];
                for (j = 0; j < nobstype[0]; j++)
                    obstype[i][j] = obstype[0][j];
            }
            // for whu's tracking stations,the C1 is C2,C2 is C7
            // id = SYS.find_first_of('C');
            id = index_string(SYS, 'C');
            if (-1 != id)
            {
                for (i = 0; i < nobstype[id]; i++)
                {
                    if ('2' == obstype[id][i][1])
                        obstype[id][i][1] = '7';
                    if ('1' == obstype[id][i][1])
                        obstype[id][i][1] = '2';
                }
            }
        }
        if (strstr(sflag, "INTERVAL"))
        {
            sscanf(line.c_str(), "%lf", &intv);
        }
        if (strstr(sflag, "TIME OF FIRST OBS"))
        {
            sscanf(line.c_str(), "%d%d%d%d%d%lf%s", t0, t0 + 1, t0 + 2, t0 + 3,
                   t0 + 4, &sec, tmetag);
            t0[5] = (int)sec;
        }
        if (strstr(sflag, "TIME OF LAST OBS"))
        {
            sscanf(line.c_str(), "%d%d%d%d%d%lf", t1, t1 + 1, t1 + 2, t1 + 3,
                   t1 + 4, &sec);
            t1[5] = (int)sec;
        }
        pos = in.tellg();
    }
    if (this->ver >= 3.0)
    {
        for (isys = 0; isys < MAXSYS; isys++)
        {
            if (this->nobstype[isys])
                (this->nsys)++;
        }
    }
}
