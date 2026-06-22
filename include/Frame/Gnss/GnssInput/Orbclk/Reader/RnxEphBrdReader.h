//
// Created by juntao, at wuhan university   on 2020/11/2.
//

#ifndef BAMBOO_RNXEPHBRDREADER_H
#define BAMBOO_RNXEPHBRDREADER_H
#include "include/Frame/Gnss/GnssInput/Orbclk/OrbitClkAdapter.h"
#include "include/Frame/Config/Const.h"
namespace iono
{
    class RnxEphBrdReader : public OrbitClkReader
    {
    public:
        RnxEphBrdReader();
        virtual ~RnxEphBrdReader();
        virtual void v_openRnxEph(std::string);
        virtual inline bool v_isOpen()
        {
            return this->isOpen;
        }
        virtual void v_closeRnxEph();
        // ionospheric correction parameters
        double ion[MAXSYS][2][4];

    protected:
        OrbitClkAdapter *orbclkAdapter;
        void m_readRnxnav(char mode);
        /// inner structure
        char curFile[1024];
        int mjd0, mjd1;
        double sod0, sod1;
        // header part
        double ver;
        // corrections to transform the system to UTC or other time systems
        double tim[MAXSYS][2][4];
        // number of leap second since 6-Jan-980
        int leap;

        //
        bool isOpen;
    };
} // namespace bamboo
#endif // BAMBOO_RNXEPHBRREADER_H
