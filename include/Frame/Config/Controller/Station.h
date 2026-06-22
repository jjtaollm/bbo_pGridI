#ifndef INCLUDE_FRAME_CONFIG_STATION_H_
#define INCLUDE_FRAME_CONFIG_STATION_H_
#include <string>
#include <cstring>
using namespace std;
namespace iono
{
    class Station
    {
    public:
        Station()
        {
            name = "";
            memset(x, 0, sizeof(x));
            memset(geod, 0, sizeof(geod));
            memset(gausx, 0, sizeof(gausx));
        }
        string name;     /* station name */
        double x[3];     /* xyz in ECEF*/
        double geod[3];  /* lat/lon height */
        double gausx[3]; /* gausx */
    };
};
#endif