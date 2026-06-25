#ifndef INCLUDE_APPLICATION_ROTI_BBO_DSTEC_H_
#define INCLUDE_APPLICATION_ROTI_BBO_DSTEC_H_
#include "include/Frame/Frame.h"
namespace iono
{
    class bbo_dstec
    {
    public:
        bbo_dstec();
        ~bbo_dstec();
        void m_process();

    public:
        void m_initInputs(int mjd, double sod, int b_reset);
        void m_outputStec(int mjd, double sod, map<string, map<string, double>> &r2v);
        map<string, map<string, double>> m_computeRawStec();

    protected:
        map<string, Rnxobs *> s2rnx;
        OrbitClkAdapter *brd;
        OrbitClkReader *reader;
    };
}

#endif
