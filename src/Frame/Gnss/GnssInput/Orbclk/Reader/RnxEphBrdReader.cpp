//
// Created by juntao, at wuhan university   on 2020/11/2.
//
#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace iono;
RnxEphBrdReader::RnxEphBrdReader()
{
    memset(curFile, 0, sizeof(curFile));
    mjd0 = mjd1 = leap = 0;
    sod0 = sod1 = ver = 0.0;
    memset(ion, 0, sizeof(double) * MAXSYS * 2 * 4);
    memset(tim, 0, sizeof(double) * MAXSYS * 2 * 4);
    isOpen = false;
}
RnxEphBrdReader::~RnxEphBrdReader()
{
    // the stl container will delete automaticly
    v_closeRnxEph();
}
void RnxEphBrdReader::v_openRnxEph(string ephcmd)
{
    int curmjd, iy, idoy;
    isOpen = true;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    double cursod, ep[6] = {0};
    curmjd = atoi(ephcmd.substr(0, index_string(ephcmd.c_str(), ':')).c_str());
    cursod = atof(ephcmd.substr(index_string(ephcmd.c_str(), ':') + 1).c_str());

    mjd0 = curmjd;
    sod0 = 0.0;
    mjd1 = curmjd + 1;
    sod1 = 0.0;
    time2epoch(mjd2time(curmjd, cursod), ep);

    idoy = mjd2doy(curmjd, &iy);
    sprintf(curFile, "%s%cbrdm%03d0.%02dp", dly->ephdir, FILEPATHSEP, idoy, iy - 2000);
    this->m_readRnxnav('M');
}
void RnxEphBrdReader::v_closeRnxEph()
{
    this->isOpen = false;
}
void RnxEphBrdReader::m_readRnxnav(char csys)
{
    // read rinex navigation file
    int i, k, isys, len, iy, im, id, ih, imin, mjd, wk_in;
    double dsec, dt1, dt0, d_aode, sod, data1, data3;
    char buf[2048], cline[1024], *ptr;
    list<GPSEPH>::iterator gpsItr;
    list<GLSEPH>::iterator glsItr;
    GPSEPH eph;
    GLSEPH ephr;
    ifstream in;
    string line;
    streampos pos;
    bool already;
    in.open(curFile, ios::in | ios::binary);
    if (!in.is_open())
    {
        cout << "***ERROR(m_readRnxnav):can't open file " << curFile << endl;
        exit(1);
    }
    while (getline(in, line))
    {
        if (strstr(line.c_str(), "END OF HEADER"))
            break;
        if (strstr(line.c_str(), "RINEX VERSION / TYPE") != NULL)
        {
            sscanf(line.c_str(), "%lf", &ver);
            if (ver > 3.0)
            {
                if (csys == 'M' && line[40] != 'M')
                {
                    printf(
                        "###WARNING(read_rnxnav):there only broadcast for %c!\n", line[40]);
                }
            }
        }
        else if (strstr(line.c_str(), "PGM / RUN BY / DATE") != NULL)
            continue;
        else if (strstr(line.c_str(), "COMMENT") != NULL)
            continue;
        else if (strstr(line.c_str(), "ION ALPHA") != NULL)
        {
        }
        else if (strstr(line.c_str(), "ION BETA") != NULL)
        {
        }
        // rinex 3.00 3.01 3.02
        else if (strstr(line.c_str(), "IONOSPHERIC CORR") != NULL)
        {
            if (!strncmp(line.c_str(), "GPS ", 4) || !strncmp(line.c_str(), "GPSA", 4))
            {
                i = index_string(SYS, 'G');
                k = 0;
            }
            else if (!strncmp(line.c_str(), "GPSB", 4))
            {
                i = index_string(SYS, 'G');
                k = 1;
            }
            else if (!strncmp(line.c_str(), "GAL ", 4))
            {
                i = index_string(SYS, 'E');
                k = 0;
            }
            else if (!strncmp(line.c_str(), "BDS ", 4) || !strncmp(line.c_str(), "BDSA", 4))
            {
                i = index_string(SYS, 'C');
                k = 0;
            }
            else if (!strncmp(line.c_str(), "BDSB", 4))
            {
                i = index_string(SYS, 'C');
                k = 1;
            }
            else if (!strncmp(line.c_str(), "QZS ", 4) || !strncmp(line.c_str(), "QZSA", 4))
            {
                i = index_string(SYS, 'J');
                k = 0;
            }
            else if (!strncmp(line.c_str(), "QZSB", 4))
            {
                i = index_string(SYS, 'J');
                k = 1;
            }
            else
                printf("###WARNING(read_rnxnav):unknown ionospheric corr!\n");
            strcpy(buf, line.c_str());
            ptr = buf;
            while (*ptr != '\0')
            {
                if (*ptr == 'D')
                    *ptr = 'e';
                ptr++;
            }
            sscanf(buf, "%*s%lf%lf%lf%lf", ion[i][k], ion[i][k] + 1,
                   ion[i][k] + 2, ion[i][k] + 3);
        }
        else if (strstr(line.c_str(), "DELTA-UTC: A0,A1,T,W") != NULL)
        {
            i = index_string(SYS, csys);
            if (i == -1)
            {
                printf(
                    "$$$MESSAGE(read_rnxnav):DELTA-UTC: A0,A1,T,W is only valid for single system!\n");
                continue;
            }
            strcpy(buf, line.c_str());
            ptr = buf;
            while (*ptr != '\0')
            {
                if (*ptr == 'D')
                    *ptr = 'e';
                ptr++;
            }
            sscanf(buf, "%lf%lf%lf%lf", tim[i][0], tim[i][0] + 1, tim[i][0] + 2,
                   tim[i][0] + 3);
        }
        else if (strstr(line.c_str(), "TIME SYSTEM CORR") != NULL)
        {
            if (!strncmp(line.c_str(), "GPUT", 4))
            {
                i = index_string(SYS, 'G');
                k = 0;
            }
            else if (!strncmp(line.c_str(), "GLUT", 4))
            {
                i = index_string(SYS, 'R');
                k = 0;
            }
            else if (!strncmp(line.c_str(), "GAUT", 4))
            {
                i = index_string(SYS, 'E');
                k = 0;
            }
            else if (!strncmp(line.c_str(), "BDUT", 4))
            {
                i = index_string(SYS, 'C');
                k = 0;
            }
            else if (!strncmp(line.c_str(), "SBUT", 4))
            {
                i = index_string(SYS, 'S');
                k = 0;
            }
            else if (!strncmp(line.c_str(), "QZUT", 4))
            {
                i = index_string(SYS, 'J');
                k = 0;
            }
            else if (!strncmp(line.c_str(), "GPGA", 4))
            {
                i = index_string(SYS, 'G');
                k = 1;
            }
            else if (!strncmp(line.c_str(), "GLGP", 4))
            {
                i = index_string(SYS, 'R');
                k = 1;
            }
            else if (!strncmp(line.c_str(), "QZGP", 4))
            {
                i = index_string(SYS, 'J');
                k = 1;
            }
            else
            {
                printf(
                    "$$$MESSAGE(read_rnxnav):unknown unknown TIME SYSTEM CORR!\n");
                continue;
            }
            strcpy(buf, line.c_str());
            ptr = buf;
            while (*ptr != '\0')
            {
                if (*ptr == 'D')
                    *ptr = 'e';
                ptr++;
            }
            sscanf(buf, "%*5c%lf%lf%lf%lf", tim[i][k], tim[i][k] + 1,
                   tim[i][k] + 2, tim[i][k] + 3);
        }
        else if (strstr(line.c_str(), "LEAP SECONDS") != NULL)
            sscanf(line.c_str(), "%d", &leap);
    }
    while (!in.eof())
    {
        // acquire the ephemeris
        pos = in.tellg();
        getline(in, line);
        if (!len_trim(line.c_str()))
            continue;
        in.seekg(pos, ios::beg);
        if (ver < 3.0)
        {
            buf[0] = csys;
        }
        else
            buf[0] = line[0];
        switch (buf[0])
        {
        case 'G':
        case 'E':
        case 'C':
        case 'J':
            isys = index_string(SYS, buf[0]);
            len = 0;
            memset(buf, 0, sizeof(char) * 2048);
            for (i = 0; i < 8; i++)
            {
                getline(in, line);
                strcpy(cline, line.c_str());
                if (i != 7)
                {
                    filleph(cline, ver);
                }
                if (ver < 3)
                {
                    if (buf[0] != csys)
                        buf[0] = csys;
                    strncpy(buf + 1 + len, cline, strlen(cline));
                }
                else
                    strncpy(buf + len, cline, strlen(cline));
                len += strlen(cline);
            }
            ptr = buf;
            while (*ptr != '\0')
            {
                if (*ptr == '\n' || *ptr == '\r')
                    *ptr = ' ';

                if (*ptr == 'D')
                    *ptr = 'e';
                ptr++;
            }
            if (ver < 3.0)
            {
                buf[0] = csys;
                if (buf[1] == ' ')
                    buf[1] = '0';
            }
            if (buf[0] == 'C')
            {
                sscanf(buf,
                       "%s%d%d%d%d%d%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",
                       eph.cprn, &iy, &im, &id, &ih, &imin, &dsec, &eph.a0,
                       &eph.a1, &eph.a2, &d_aode, &eph.crs, &eph.dn, &eph.m0,
                       &eph.cuc, &eph.e, &eph.cus, &eph.roota, &eph.toe, &eph.cic,
                       &eph.omega0, &eph.cis, &eph.i0, &eph.crc, &eph.omega,
                       &eph.omegadot, &eph.idot, &eph.resvd0, &eph.week,
                       &eph.resvd1, &eph.accu, &eph.hlth, &eph.tgd, &eph.aodc,
                       &data1, &eph.iodc, &data3, &eph.signal_idx);
                eph.signal_idx = (int)eph.signal_idx == 0 ? 1 : (int)eph.signal_idx;
                if (eph.signal_idx > 6 && eph.signal_idx <= 13)
                {
                    eph.delta_A = eph.roota;
                    eph.A_DOT = eph.resvd0;
                    eph.delta_n_dot = eph.resvd1;
                }
                //				    if(eph.signal_idx != 7) continue; // for b2b test
            }
            else
            {
                sscanf(buf,
                       "%s%d%d%d%d%d%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",
                       eph.cprn, &iy, &im, &id, &ih, &imin, &dsec, &eph.a0,
                       &eph.a1, &eph.a2, &d_aode, &eph.crs, &eph.dn, &eph.m0,
                       &eph.cuc, &eph.e, &eph.cus, &eph.roota, &eph.toe, &eph.cic,
                       &eph.omega0, &eph.cis, &eph.i0, &eph.crc, &eph.omega,
                       &eph.omegadot, &eph.idot, &eph.resvd0, &eph.week,
                       &eph.resvd1, &eph.accu, &eph.hlth, &eph.tgd, &eph.aodc);
            }
            ///////////////////////////////////////////////////////////////////////////////////////////////
            if (eph.hlth > 0 || isys == -1)
                continue;
            yr2year(iy);
            if (iy < 2000)
                continue;
            // time of clock
            eph.aode = static_cast<int>(d_aode);
            eph.mjd = ymd2mjd(iy, im, id); // TIME IN BDS TIME AND SHOULD NOT CHANGE IT INTO GPST BECAUSE THERE ARE OTHER TIME TAG IN THE EPHEMERIS
            eph.sod = ih * 3600.0 + imin * 60.0 + dsec;
            // adapter to bds
            if (eph.cprn[0] == 'C')
            {
                // the week in broadcast file generated by WHU is GPS week,
                // but that in IGS meraged file is BDS week
                mjd2wksow(eph.mjd, eph.sod, &k, &dt1);
                if (eph.week != 0.0)
                {
                    wksow2mjd(eph.week, eph.toe, &mjd, &sod);
                    mjd2wksow(mjd, sod, &wk_in, &eph.toe);
                    eph.week = wk_in;
                    if (k != (int)eph.week)
                    {
                        eph.week = eph.week + 1356.0;
                    }
                }
                else
                {
                    eph.week = 0;
                    for (int i = -1; i < 2; i++)
                    {
                        wksow2mjd(k + i, eph.toe, &mjd, &sod);
                        if (fabs(timdif(mjd, sod, eph.mjd, eph.sod)) < 86400.0 * 7 / 2)
                        {
                            eph.week = k + i;
                            break;
                        }
                    }
                    if (eph.week == 0)
                    {
                        memset(eph.cprn, 0, sizeof(eph.cprn));
                    }
                    else
                    {
                        wksow2mjd(eph.week, eph.toe, &mjd, &sod);
                        mjd2wksow(mjd, sod, &wk_in, &eph.toe);
                        eph.week = wk_in;
                    }
                }
                eph.tgd1 = eph.aodc;
            }
            // check time
            dt0 = 0.0;
            dt1 = 0.0;
            if (mjd0 != 0)
            {
                dt0 = eph.mjd + eph.sod / 86400.0 - mjd0;
            }
            if (mjd1 != 0)
            {
                dt1 = eph.mjd + eph.sod / 86400.0 - mjd1;
            }
            if (dt0 < -1.0 / 24.0 || dt1 > 1.0 / 24.0)
                continue;
            for (auto &v : m_orbclkAdapters)
                v->v_inputEph(&eph, NULL);
            break;
        case 'R':
            isys = index_string(SYS, buf[0]);
            len = 0;
            memset(buf, 0, sizeof(char) * 2048);
            for (i = 0; i < 4; i++)
            {
                getline(in, line);
                strcpy(cline, line.c_str());
                if (ver < 3)
                {
                    if (buf[0] != csys)
                        buf[0] = csys;
                    strncpy(buf + 1 + len, cline, strlen(cline));
                }
                else
                    strncpy(buf + len, cline, strlen(cline));
                len += strlen(cline);
            }
            ptr = buf;
            while (*ptr != '\0')
            {
                if (*ptr == '\n' || *ptr == '\r')
                {
                    *ptr = ' ';
                }
                if (*ptr == 'D')
                {
                    *ptr = 'e';
                }
                ptr++;
            }
            if (ver < 3.0)
            {
                buf[0] = csys;
                if (buf[1] == ' ')
                    buf[1] = '0';
            }
            sscanf(buf,
                   "%s%d%d%d%d%d%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",
                   ephr.cprn, &iy, &im, &id, &ih, &imin, &dsec, &ephr.tau,
                   &ephr.gamma, &ephr.tk, &ephr.pos[0], &ephr.vel[0],
                   &ephr.acc[0], &ephr.health, &ephr.pos[1], &ephr.vel[1],
                   &ephr.acc[1], &ephr.frenum, &ephr.pos[2], &ephr.vel[2],
                   &ephr.acc[2], &ephr.age);

            if (ephr.health > 0.0)
                continue;
            yr2year(iy);
            if (iy < 2000)
                continue;
            ephr.mjd = ymd2mjd(iy, im, id);
            ephr.sod = ih * 3600.0 + imin * 60.0 + dsec;
            brdtime(ephr.cprn, &ephr.mjd, &ephr.sod); // CHANGE IT INTO GPST TIME BECAUSE THE THE BASE TIME CAN CHANGE AND SHOULD NOT
            // UPDATE IT IN A WHILE LOOP
            // check time
            dt0 = 0.0;
            dt1 = 0.0;
            if (mjd0 != 0)
            {
                dt0 = ephr.mjd + ephr.sod / 86400.0 - mjd0;
            }
            if (mjd1 != 0)
            {
                dt1 = ephr.mjd + ephr.sod / 86400.0 - mjd1;
            }
            if (dt0 < -1.0 / 24.0 || dt1 > 1.0 / 24.0)
                continue;
            for (auto &v : m_orbclkAdapters)
                v->v_inputEph(NULL, &ephr);
            break;
        default:
            getline(in, line);
            break;
        }
    }
}
