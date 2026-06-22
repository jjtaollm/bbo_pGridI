#ifndef INCLUDE_APPLICATIONS_GNSS_GNSSINPUT_DCB_DCBREADER_H
#define INCLUDE_APPLICATIONS_GNSS_GNSSINPUT_DCB_DCBREADER_H
#include "DCBAdapter.h"
namespace iono
{
    class DCBReader
    {
    public:
        DCBReader();
        ~DCBReader();

        void m_read_DCB(string f);
        void m_setAdapter(DCBAdapter *in)
        {
            adapter = in;
        }

    protected:
        DCBAdapter *adapter;
    };
}
#endif