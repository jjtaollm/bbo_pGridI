#ifndef INCLUDE_IONOSPHERE_READER_SLANTREADER_RT_H_
#define INCLUDE_IONOSPHERE_READER_SLANTREADER_RT_H_
#include "include/Frame/Gnss/GnssInput/Atomsphere/UDAtomReader.h"
#include "include/Frame/Connection/Net/MyRedisAdapter.h"
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;
namespace iono
{
    class SlantReader_rt : public UDAtomReader
    {
    public:
        SlantReader_rt(string s);
        virtual ~SlantReader_rt();

        virtual void v_open(string cmd);          /* open data */
        virtual void v_close();                   /* close data */
        virtual void v_read(int mjd, double sod); /* read a epoch */
        void s_channelRead(string url);
        void m_updateSit(list<string> &);
        int m_inputData(unsigned char data);
        void m_decode();

    protected:
        thread t_pthRecv;
        bool shouldStop;
        std::mutex mtx;
        std::condition_variable cv;
        list<string> mcaclist;
        int nbyte;
        int len;
        char *payload;
        char lenbuf[64];
        MyRedisAdapter myRedis;
    };
}
#endif
