#ifndef CONNECTION_NET_MYREDISADAPTER_H_
#define CONNECTION_NET_MYREDISADAPTER_H_
#include <string>
#include <list>
#include "include/Frame/Config/Const.h"
#ifdef REDIS
#include <hiredis/hiredis.h>
#endif
namespace iono
{
    class MyRedisAdapter
    {
    public:
        MyRedisAdapter();
        ~MyRedisAdapter();
        void m_initRedis();
        void m_disconRedis();
        bool m_addSubscribe(std::string);
        void m_setIpPort(const char *ip, int port);
        int m_readSync(char *buff, int nbyte);
        string m_readPack();
        bool m_writeSync(const char *channel, const char *buff, int nbyte, int, string sendtype = "PUBLISH");
        bool m_redisSet(string command);
        string m_redisGet(string command);

    protected:
        void m_connectRedis();
        bool m_updateSubscribe();
        std::string m_ip;
        std::list<std::string> m_channels;
        int m_port;
        char *buff_remain;
        int nlen;

#ifdef REDIS
        redisContext *m_redis;
#endif
    };
} // namespace bamboo
#endif