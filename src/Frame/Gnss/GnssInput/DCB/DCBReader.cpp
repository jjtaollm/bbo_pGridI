/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-22 17:51:06
 * @ Modified by: Jun Tao
 * @ Modified time: 2024-09-22 09:52:03
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */

#include "include/Frame/Frame.h"
#include <algorithm>
using namespace std;
using namespace iono;
DCBReader::DCBReader()
{
    adapter = NULL;
}
DCBReader::~DCBReader()
{
}
void DCBReader::m_read_DCB(string filname)
{
    char line[1024] = {0}, sitn[16] = {0}, csys[16] = {0};
    double dv = 0.0, dsig;
    FILE *fp = fopen(filname.c_str(), "r");
    if (fp == NULL)
    {
        LOGPRINT("file = %s, can't open DCB file to read!", filname.c_str());
        return;
    }
    while (!feof(fp))
    {
        fgets(line, 1024, fp);
        sscanf(line, "%s%s%lf%lf", sitn, csys, &dv, &dsig);
        transform(sitn, sitn + 16, sitn, ::toupper);
        if (adapter)
            adapter->v_input(sitn, csys[0], dv, dsig);
    }
    fclose(fp);
}