#ifndef INCLUDE_FRAME_CONNECTION_IONOSPHERE_UDAtomReader_H_
#define INCLUDE_FRAME_CONNECTION_IONOSPHERE_UDAtomReader_H_
#include <string>
using namespace std;
namespace iono
{
    class UDAtomAdapter;
    class UDAtomReader
    {
    public:
        UDAtomReader(string tag)
        {
            isOpen = false;
            mcmd = "";
            mtag = tag;
            m_adapter = NULL;
        }
        virtual ~UDAtomReader() {}

        virtual void v_open(string cmd) {}          /* open the data */
        virtual void v_close() {}                   /* close the files */
        virtual bool v_isOpen() { return isOpen; }  /* check if is open */
        virtual void v_read(int mjd, double sod) {} /* reading the data */

        void m_setAdapter(UDAtomAdapter *in) { this->m_adapter = in; }
        void m_setCmd(string cmd) { this->mcmd = cmd; }
        string m_getcmd() { return mcmd; }

    protected:
        bool isOpen;
        string mcmd;
        string mtag;
        UDAtomAdapter *m_adapter;
    };
}

#endif