//
// Created by juntao, at wuhan university   on 2020/11/2.
//

#ifndef BAMBOO_ORBITCLKREADER_H
#define BAMBOO_ORBITCLKREADER_H
#include <iostream>
#include <vector>
#include "OrbitClkAdapter.h"
namespace iono
{
    class OrbitClkReader
    {
    public:
        OrbitClkReader()
        {
        }
        OrbitClkReader(OrbitClkAdapter *in)
        {
            if (in)
                m_orbclkAdapters.push_back(in);
        }
        virtual void v_setOrbclkAdapter(OrbitClkAdapter *in)
        {
            if (in)
            {
                m_orbclkAdapters.clear();
                m_orbclkAdapters.push_back(in);
            }
        }
        virtual void v_addOrbclkAdapter(OrbitClkAdapter *in)
        {
            if (in)
            {
                m_orbclkAdapters.push_back(in);
            }
        }
        virtual void v_setOrbclkAdapter(vector<OrbitClkAdapter *> in)
        {
            m_orbclkAdapters = in;
        }
        virtual void v_resetOrbclkAdapter()
        {
            m_orbclkAdapters.clear();
        }
        virtual ~OrbitClkReader()
        {
            m_orbclkAdapters.clear();
        }
        virtual bool v_isOpen()
        {
            std::cout << "***(ERROR):Base Class is not implement!" << std::endl;
            return false;
        }
        virtual void v_openRnxEph(std::string)
        {
            std::cout << "***(ERROR):Base Class is not implement!" << std::endl;
        }
        virtual void v_closeRnxEph()
        {
        }

    protected:
        std::vector<OrbitClkAdapter *> m_orbclkAdapters;
    };
}
#endif // BAMBOO_ORBITCLKREADER_H
