/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-22 14:53:50
 * @ Modified by: Jun Tao
 * @ Modified time: 2024-08-23 18:46:07
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */

#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
#include <cmath>
#include <string>
using namespace std;
using namespace iono;

SlantReader_post::~SlantReader_post()
{
    v_close();
}
void SlantReader_post::v_close()
{
    if (reFp)
        fclose(reFp);
    reFp = NULL;
    isOpen = false;
}
void SlantReader_post::v_open(string ioncmd)
{
    int curmjd, iy, idoy;
    char filname[1024] = {0};
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    curmjd = atoi(ioncmd.substr(0, index_string(ioncmd.c_str(), ':')).c_str());

    idoy = mjd2doy(curmjd, &iy);
    sprintf(filname, "%s/slantion_%04d%03d_%s", dly->externaldir, iy, idoy, mtag.c_str());
    if (reFp != NULL)
        fclose(reFp);
    if (!(reFp = fopen(filname, "rb")))
    {
        LOGPRINT("file = %s,can't open ion file to read!", filname);
    }
    this->isOpen = true;
    LOGPRINT("reading %s ...", filname);
}
void SlantReader_post::v_read(int mjd, double sod)
{
    /*********** get the corresponding ion delay from the ion-est file ****************************/
    char line[1024] = {0}, c_cprn[8] = {0};
    int i_mjd, isat, ifix, AMB_FIX = 5;
    double d_sod, elev, slant_est, azim, sig, ipp_lat, ipp_lon;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    if (reFp)
    {
        if (c_lastpos != -1)
        {
            /* rewind the file here */
            fseek(reFp, c_lastpos - ftell(reFp), SEEK_CUR);
        }
        while (!feof(reFp))
        {
            c_lastpos = ftell(reFp); /* index of the first epoch */
            fgets(line, 1024, reFp);
            if (strncmp(line, "  ION", 5))
                continue;
            sscanf(line, "%*s%*s%*s%d%lf", &i_mjd, &d_sod);
            if ((mjd - i_mjd) * 86400.0 + sod - d_sod > MAXWND)
                continue;
            /* rewind to this line */
            fseek(reFp, c_lastpos - ftell(reFp), SEEK_CUR);
            break;
        }
        while (!feof(reFp))
        {
            fgets(line, 1024, reFp);
            if (strncmp(line, "  ION", 5))
                continue;
            sscanf(line, "%*s%*s%s%d%lf%lf%lf%lf%lf%lf%lf%d", c_cprn, &i_mjd, &d_sod, &elev, &azim, &ipp_lat, &ipp_lon, &slant_est, &sig, &ifix);
            if (fabs((mjd - i_mjd) * 86400.0 + sod - d_sod) > MAXWND)
                break;
            isat = pointer_string(dly->nprn, dly->cprn, c_cprn);
            if (isat != -1 && ifix == AMB_FIX)
            {
                StecC stec;
                stec.isat = isat;
                strcpy(stec.name, mtag.c_str());
                stec.ionva = slant_est;
                stec.elev = elev;
                stec.azim = azim;
                stec.t_r = mjd2time(i_mjd, d_sod);
                // 添加权重
                if (stec.elev > 30)
                    stec.R = 1.0;
                else
                    stec.R = sin(stec.elev * DEG2RAD) * 2;
                if (dly->mSta.count(stec.name))
                    memcpy(stec.x, dly->mSta[stec.name].x, sizeof(double) * 3);
                if (dly->mPostReqs.count(stec.name))
                    memcpy(stec.x, dly->mPostReqs[stec.name].x, sizeof(double) * 3);
                m_adapter->m_inputIono(stec);
            }
        }
    }
}
