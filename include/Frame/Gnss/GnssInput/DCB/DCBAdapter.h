#ifndef INCLUDE_APPLICATIONS_GNSS_GNSSINPUT_DCB_DCBADAPTER_H
#define INCLUDE_APPLICATIONS_GNSS_GNSSINPUT_DCB_DCBADAPTER_H
#include <string>
#include <map>
using namespace std;
namespace iono
{
    class DCBAdapter
    {
    public:
        void v_input(string sname, char csys, double v, double sig)
        {
            if (s2dcb.find(sname) == s2dcb.end())
                s2dcb[sname] = map<char, double>();
            s2dcb[sname][csys] = v;

            if (s2dcb_sig.find(sname) == s2dcb_sig.end())
                s2dcb_sig[sname] = map<char, double>();
            s2dcb_sig[sname][csys] = sig;
        }
        map<string, map<char, double>> s2dcb;
        map<string, map<char, double>> s2dcb_sig;
    };
}
#endif