#include <cstring>
#include <unistd.h>
#include "include/Frame/Frame.h"
using namespace iono;
#define test_cond(t, _c)                                                     \
    if (_c)                                                                  \
    {                                                                        \
        LOGPRINT_EX("timeout for redis: %s %d", t->m_ip.c_str(), t->m_port); \
    }                                                                        \
    else                                                                     \
    {                                                                        \
    };
MyRedisAdapter::MyRedisAdapter()
{
#ifdef REDIS
    m_redis = NULL;
#endif
    buff_remain = NULL;
    nlen = 0;
    m_ip = "";
    m_port = 8001;
}
MyRedisAdapter::~MyRedisAdapter()
{
#ifdef REDIS
    if (m_redis)
        redisFree(m_redis);
    m_redis = NULL;
#endif
    if (buff_remain)
        free(buff_remain);
}
void MyRedisAdapter::m_setIpPort(const char *ip, int port)
{
    m_ip = ip;
    m_port = port;
}
void MyRedisAdapter::m_connectRedis()
{
#ifdef REDIS
    while (true)
    {
        if (m_redis)
            redisFree(m_redis);
        LOGPRINT_EX("ip = %s,port = %d, connect redis...", m_ip.c_str(), m_port);
        m_redis = redisConnect(m_ip.c_str(), m_port);
        if (!m_redis->err)
            break;
        usleep(5e6);
    }
    LOGPRINT_EX("ip = %s,port = %d, connect redis success", m_ip.c_str(), m_port);
#else
    LOGPRINT_EX("ip = %s,port = %d, connect redis success", m_ip.c_str(), m_port);
#endif
}
void MyRedisAdapter::m_initRedis()
{
    m_connectRedis();
    while (!m_updateSubscribe())
        m_connectRedis();
}
void MyRedisAdapter::m_disconRedis()
{
#ifdef REDIS
    if (m_redis)
        redisFree(m_redis);
    m_redis = NULL;
#endif
}
bool MyRedisAdapter::m_addSubscribe(string channel)
{
    bool isuccess = true;
#ifdef REDIS
    if (!bbo_contains_list(m_channels, channel))
        m_channels.push_back(channel);
    if (m_redis == NULL)
        return true;
    redisReply *reply = (redisReply *)redisCommand(m_redis, "subscribe %s", channel.c_str());
    if (reply == NULL || reply->type == REDIS_REPLY_ERROR)
    {
        isuccess = false;
        LOGPRINT_EX("sending command subscribe %s error", channel.c_str());
    }
    else
        LOGPRINT_EX("subscribe %s success", channel.c_str());
    if (reply)
        freeReplyObject(reply);
#else
    LOGPRINT_EX("subscribe %s with NULL implement", channel.c_str());
#endif
    return isuccess;
}
bool MyRedisAdapter::m_updateSubscribe()
{
    bool isuccess = true;
    for (auto &channel : m_channels)
        isuccess = isuccess && m_addSubscribe(channel);
    return isuccess;
}
bool MyRedisAdapter::m_writeSync(const char *channel, const char *buff, int nbyte, int expire, string sendtype)
{
    bool isuccess = true;
#ifdef REDIS
    redisReply *reply = NULL;
    if (m_redis == NULL)
        m_initRedis();
    if (expire == -1)
        reply = (redisReply *)redisCommand(m_redis, "%s %s %b", sendtype.c_str(), channel, buff, (size_t)nbyte);
    else
        reply = (redisReply *)redisCommand(m_redis, "%s %s %b EX %d", sendtype.c_str(), channel, buff, (size_t)nbyte, expire);
    if (reply == NULL)
    {
        isuccess = false;
        m_initRedis();
    }
    if (reply && reply->type == REDIS_REPLY_ERROR)
    {
        isuccess = false;
        LOGPRINT_EX("redis error for writeSync: %s", reply->str);
    }
    if (reply)
        freeReplyObject(reply);
#else
    LOGPRINT_EX("write redis with NULL implement");
#endif
    return isuccess;
}
int MyRedisAdapter::m_readSync(char *buff, int nbyte)
{
#ifdef REDIS
    int ngot = 0, nemp;
    if (nlen > 0)
    {
        ngot = (nbyte > nlen) ? nlen : nbyte;
        memcpy(buff, buff_remain, sizeof(char) * ngot);
        nlen = nlen - ngot;
        if (nlen > 0) /* there is still data remained */
        {
            char *buff_trunate = (char *)calloc(nlen, sizeof(char));
            memcpy(buff_trunate, buff_remain + ngot, sizeof(char) * nlen);
            free(buff_remain);
            buff_remain = buff_trunate;
            return ngot;
        }
        else
        {
            free(buff_remain);
            buff_remain = NULL;
        }
    }
    nemp = nbyte - ngot; /* remained length in buff */
    /* start to read from the redis */
    redisReply *reply = NULL;
    if (m_redis == NULL)
        m_initRedis();

    // add for timeout
    struct timeval tv = {15, 0};
    redisSetTimeout(m_redis, tv);
    int status = redisGetReply(m_redis, (void **)&reply);
    test_cond(this, status == REDIS_ERR && m_redis->err == REDIS_ERR_TIMEOUT);

    if (status == REDIS_ERR)
        m_initRedis(); /* something went wrong */
    if (status == REDIS_OK && reply)
    {
        /* here to parse the data */
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (int i = 0; i < reply->elements; ++i)
            {
                redisReply *childReply = reply->element[i];
                if (childReply->type == REDIS_REPLY_STRING && i == 2)
                {
                    int nrem = childReply->len - nemp; /* remain data */
                    if (nrem > 0)
                    {
                        /* means the data is more than the buff length */
                        memcpy(buff + ngot, childReply->str, sizeof(char) * nemp);
                        buff_remain = (char *)calloc(nrem, sizeof(char));
                        memcpy(buff_remain, childReply->str + nemp, sizeof(char) * nrem);
                        nlen = nrem;
                        ngot = ngot + nemp; // fill the buff
                    }
                    else
                    {
                        /* means the buff is enough to hold the data */
                        memcpy(buff + ngot, childReply->str, sizeof(char) * childReply->len);
                        ngot = ngot + childReply->len;
                    }
                }
            }
        }
    }
    if (reply)
    {
        freeReplyObject(reply);
    }
    return ngot;
#else
    LOGPRINT_EX("read redis with NULL implement");
    usleep(1E7); /* sleep 10s */
    return 0;
#endif
}
string MyRedisAdapter::m_readPack()
{
#ifdef REDIS
    string str_r = "";
    /* start to read from the redis */
    redisReply *reply = NULL;
    if (m_redis == NULL)
        m_initRedis();

    struct timeval tv = {15, 0};
    redisSetTimeout(m_redis, tv);
    int status = redisGetReply(m_redis, (void **)&reply);
    test_cond(this, status == REDIS_ERR && m_redis->err == REDIS_ERR_TIMEOUT);

    if (status == REDIS_ERR)
        m_initRedis(); /* something went wrong */
    if (status == REDIS_OK && reply)
    {
        /* here to parse the data */
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (int i = 0; i < reply->elements; ++i)
            {
                redisReply *childReply = reply->element[i];
                if (childReply->type == REDIS_REPLY_STRING && i == 2)
                {
                    str_r.assign(childReply->str, childReply->len);
                }
            }
        }
    }
    if (reply)
    {
        freeReplyObject(reply);
    }
    return str_r;
#else
    LOGPRINT_EX("read redis with NULL implement");
    usleep(1E7); /* sleep 10s */
    return 0;
#endif
}
/// @brief Redis set function
/// @param key
/// @return
string MyRedisAdapter::m_redisGet(string command)
{
#ifdef REDIS
    if (!m_redis)
        m_initRedis();

    struct timeval tv = {2, 0};
    redisSetTimeout(m_redis, tv);
    redisReply *reply = (redisReply *)redisCommand(m_redis, command.c_str());
    if (!reply)
    {
        LOGPRINT_EX("Redis command failed:%s", command.c_str());
        m_initRedis(); // Retry connection on failure
        return "";
    }
    if (reply->type == REDIS_REPLY_ERROR || reply->type == REDIS_REPLY_NIL)
    {
        // LOGPRINT_EX("Redis ERROR or NIL  %s", command.c_str());
        freeReplyObject(reply);
        return "";
    }
    if (reply->type == REDIS_REPLY_STRING && reply->str)
    {
        string v = std::string(reply->str, reply->len);
        freeReplyObject(reply);
        return v;
    }
    freeReplyObject(reply);
    return "";
#else
    return "";
#endif
}

bool MyRedisAdapter::m_redisSet(string command)
{
#ifdef REDIS
    if (!m_redis)
        m_initRedis();

    struct timeval tv = {2, 0};
    redisSetTimeout(m_redis, tv);
    redisReply *reply = (redisReply *)redisCommand(m_redis, command.c_str());
    if (!reply)
    {
        LOGPRINT("Redis command failed: %s ", command.c_str());
        m_initRedis(); // Retry connection on failure
        return false;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        LOGPRINT_EX("Redis error: %s", reply->str);
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
#else
    return false;
#endif
}
