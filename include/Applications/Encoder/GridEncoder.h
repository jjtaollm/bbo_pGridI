#ifndef INCLUDE_FRAME_CONNECTION_IONOSPHERE_GRIDENCODE_H_
#define INCLUDE_FRAME_CONNECTION_IONOSPHERE_GRIDENCODE_H_

#include "include/Applications/Applications.h"

using namespace std;
namespace iono
{
    class GridEncoder
    {
    public:
        static char *s_encode_intern_send(char *buff_in, const int blen, const int msgid, int &ilen);
        static char *s_encode_slant_iono(map<string, pair<double, double>> &m_v, const int isys, const int iod, const int udint, const double mlat0, const double mlon0, const double mheight0, gtime_t t0, int sync, int &ilen);
    };
}
#endif