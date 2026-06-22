#ifndef CONNECTION_MYMQTTADAPTER_H_
#define CONNECTION_MYMQTTADAPTER_H_
#ifdef MQTT
#include </usr/local/include/MQTTClient.h>
#endif
#include <string>
namespace iono
{
    class MyMqttAdapter
    {
    public:
        MyMqttAdapter();
        ~MyMqttAdapter();
        bool m_connect(const char *);
        bool m_writeSync(const char *channel, const char *buff, int nbyte);
        int m_readSync(char *buff, int nsize);
        void m_addSubscribe(const char *);

    protected:
#ifdef MQTT
        MQTTClient client;
#endif
        std::string url;
        std::string curid;
        std::string topic;
        char *buff_remain;
        int nlen;
    };
}
#endif