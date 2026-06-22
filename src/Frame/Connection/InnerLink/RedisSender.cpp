#include "include/Frame/Frame.h"
using namespace iono;
RedisSender::RedisSender()
{
    m_last_check = 0;
    pid_send = 0;
    is_on = false;
    m_tag = "";
    b_recon = false;
}
RedisSender::~RedisSender()
{
    is_on = false;
    m_cond_send.notify_all();
    if (pid_send != 0)
        pthread_join(pid_send, NULL);
}
void RedisSender::m_StartService(string tag)
{
    m_tag = tag;
    is_on = true;
#ifdef _WIN32
    LOGPRINT("Function is not implemented in windows!");
#else
    /// bind a port first
    if (0 != pthread_create(&pid_send, NULL, &s_pthSendMessage, this))
    {
        cout << "***ERROR(s_VRSMain):cant create thread to request atomsphere!"
             << endl;
        exit(1);
    }
#endif
}
void RedisSender::m_setRedisConfigure(const char *ip, int port, const char *channel)
{
    m_channel = channel;
    myredis.m_setIpPort(ip, port);
}
void *RedisSender::s_pthSendMessage(void *args)
{
    RedisSender *handler = (RedisSender *)args;
    handler->m_loopSendMessage();
    return NULL;
}
/* should use thread pool */
void RedisSender::m_loopSendMessage()
{
    while (is_on)
    {
        Package pack;
        {
            std::unique_lock<std::mutex> lk(m_lock_send);
            while (m_packages.empty() && is_on)
                m_cond_send.wait(lk);
            if (!m_packages.empty())
            {
                pack = m_packages.front();
                m_packages.pop_front();
            }
        }
        if (pack.buff != NULL)
        {
            {
                std::shared_lock<std::shared_mutex> lk(m_lock_tag);
                if (b_recon)
                {
                    char addr[256] = {0}, port_str[256] = {0}, mnt[256] = {0};
                    decodetcppath(m_tag.c_str(), addr, port_str, NULL, NULL, mnt, NULL);
                    myredis.m_setIpPort(addr, atoi(port_str));
                    if (strlen(mnt))
                        m_channel = mnt;
                    b_recon = false;
                }
            }

            if (pack.channel == "")
                myredis.m_writeSync(m_channel.c_str(), pack.buff, pack.nbyte, pack.expire, pack.type);
            else
                myredis.m_writeSync(pack.channel.c_str(), pack.buff, pack.nbyte, pack.expire, pack.type);
        }
    }
}
string RedisSender::m_get_tag()
{
    std::shared_lock<std::shared_mutex> lk(m_lock_tag);
    return m_tag;
}
void RedisSender::m_alter_tag(string s)
{
    std::unique_lock<std::shared_mutex> lk(m_lock_tag);
    m_tag = s;
    b_recon = true;
}
void RedisSender::m_addPackage(Package &pack, string tag)
{
    char str[256] = {0};
    list<Package>::iterator itr;
    char fmt[1024];
    {
        std::unique_lock<std::mutex> lk(m_lock_send);
        m_packages.push_back(pack);
        /// clear the over
        for (itr = m_packages.begin(); itr != m_packages.end();)
        {
            if ((pack.ptime - (*itr).ptime) > 120)
            { /// overthan 2 minutes
                itr = m_packages.erase(itr);
                continue;
            }
            ++itr;
        }
        time_t tt = time(NULL);
        if (tt - m_last_check > 120)
        {
            m_last_check = tt;
            if (m_packages.size() > 0)
            {
                Package &front = m_packages.front();
                Package &back = m_packages.back();
                LOGPRINT_EX("%s data remains: %d(size),%s(front) %s(back)", tag.c_str(), m_packages.size(),
                            time2str(front.ptime, str, 1),
                            time2str(back.ptime, str, 1));
            }
            else
            {
                LOGPRINT_EX("%s data remains: %d(size)", tag.c_str(), m_packages.size());
            }
        }
    }
    m_cond_send.notify_all();
}