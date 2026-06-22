#ifndef INCLUDE_FRAME_CONNECTION_IONOSPHERE_SLANTREADER_POST_H_
#define INCLUDE_FRAME_CONNECTION_IONOSPHERE_SLANTREADER_POST_H_
#include "include/Frame/Gnss/GnssInput/Atomsphere/UDAtomReader.h"
namespace iono
{
    class SlantReader_post : public UDAtomReader
    {
    public:
        SlantReader_post(string s) : UDAtomReader(s) { reFp = NULL, c_lastpos = -1; }
        virtual ~SlantReader_post();

        virtual void v_open(string cmd);          /* open data */
        virtual void v_close();                   /* close data */
        virtual void v_read(int mjd, double sod); /* read a epoch */

    protected:
        FILE *reFp;
        int c_lastpos;
    };
}
#endif