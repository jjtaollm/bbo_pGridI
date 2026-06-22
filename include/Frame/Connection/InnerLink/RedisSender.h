#ifndef GNSSALGO_ALGO_ENCODER_REDISATOMSENDER_INCLUDE_H_
#define GNSSALGO_ALGO_ENCODER_REDISATOMSENDER_INCLUDE_H_
#include <queue>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <cstdlib>
#ifdef REDIS
#include <hiredis/hiredis.h>
#endif
#include "include/Frame/Connection/Net/MyRedisAdapter.h"
#include "include/Frame/Config/Const.h"
namespace iono
{
    class Package
    {
    public:
        Package()
        {
            ptime = 0;
            nbyte = 0;
            buff = NULL;
            expire = -1; /* not expired*/
            channel = "";
            type = "";
        }
        ~Package()
        {
            if (buff)
                free(buff);
        }
        Package(const Package &in)
        {
            ptime = in.ptime;
            nbyte = in.nbyte;
            channel = in.channel;
            type = in.type;
            expire = in.expire;
            buff = NULL;

            if (in.buff)
            {
                buff = (char *)calloc(in.nbyte, sizeof(char));
                memcpy(buff, in.buff, sizeof(char) * in.nbyte);
            }
        }
        Package &operator=(const Package &in)
        {
            ptime = in.ptime;
            nbyte = in.nbyte;
            channel = in.channel;
            type = in.type;
            expire = in.expire;
            buff = NULL;
            if (in.buff)
            {
                buff = (char *)calloc(in.nbyte, sizeof(char));
                memcpy(buff, in.buff, sizeof(char) * in.nbyte);
            }
            return *this;
        }
        string channel;
        string type;
        time_t ptime;
        char *buff;
        int nbyte;
        int expire;
    };
    class RedisSender
    {
    public:
        RedisSender();
        ~RedisSender();
        void m_setRedisConfigure(const char *ip, int port, const char *channel);
        void m_StartService(string tag = "");
        void m_addPackage(Package &, string);
        bool b_ison() { return is_on; }
        void m_alter_tag(string tag);
        string m_get_tag();

    protected:
        static void *s_pthSendMessage(void *);
        void m_loopSendMessage();
        string m_channel;
        string m_tag;
        bool is_on;
        bool b_recon;
        pthread_t pid_send;
        list<Package> m_packages;
        time_t m_last_check;
        std::mutex m_lock_send;
        std::shared_mutex m_lock_tag;
        std::condition_variable m_cond_send;
        MyRedisAdapter myredis;
    };
} // namespace bamboo
#endif