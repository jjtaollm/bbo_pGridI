#ifndef INCLUDE_FRAME_CONNECTION_IONOSPHERE_ATOMENCODE_H_
#define INCLUDE_FRAME_CONNECTION_IONOSPHERE_ATOMENCODE_H_

#include "include/Applications/Applications.h"

using namespace std;
namespace iono
{
    class SurfEncoder
    {
    public:
        static char *s_encode_slant_iono(std::map<std::string, polyCoefficient> &coef, const int isys, const int udint, const double mlat0, const double mlon0, const int n, const int m, gtime_t t0, int sync, int &ilen);
    };
}
#endif