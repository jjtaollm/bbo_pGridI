#include <iostream>
#include <thread>
#include <random>
#include <algorithm>
#include <unistd.h>
#include "include/Frame/Frame.h"
using namespace iono;
using namespace std;
MyMqttAdapter::MyMqttAdapter()
{
#ifdef MQTT
    client = NULL;
#endif
    url = "";
    buff_remain = NULL;
    nlen = 0;
    topic = "";
}
MyMqttAdapter::~MyMqttAdapter()
{
#ifdef MQTT
    if (client)
        MQTTClient_destroy(&client);
#endif
    if (buff_remain)
        free(buff_remain);
}

void connectionLost(void *context, char *cause)
{
#ifdef MQTT
    MQTTClient *cli = (MQTTClient *)context;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 30;
    conn_opts.cleansession = 0;
    MQTTClient_setCallbacks(cli, NULL, connectionLost, NULL, NULL);
    while (MQTTClient_connect(cli, &conn_opts) != MQTTCLIENT_SUCCESS)
    {
        LOGPRINT_EX("Connection lost! Cause: %s,trying reconnect MQTT server...", cause);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    LOGPRINT_EX("reconnect to MQTT success!");
#endif
}

bool MyMqttAdapter::m_connect(const char *path)
{
#ifdef MQTT
    auto get_rd_id = [](void) -> int
    {
        std::random_device rd;

        // 使用 Mersenne Twister 引擎
        std::mt19937 gen(rd());

        // 定义分布：生成范围在 1 到 100 的随机整数
        std::uniform_int_distribution<> dis(1, 1000);

        // 生成随机数
        return dis(gen);
    };

    int ir;
    LOGPRINT("connect to %s", path);
    pthread_t thread_id = pthread_self();
    pid_t process_id = getpid();
    this->url = path;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    curid = "bamboo+" + toString(process_id) + "_" + toString(thread_id) + "_" + toString(get_rd_id());
    MQTTClient_create(&client, path, curid.c_str(),
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 30;
    conn_opts.cleansession = 1;
    MQTTClient_setCallbacks(client, NULL, connectionLost, NULL, NULL);

    while ((ir = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        LOGPRINT_EX("trying connect MQTT server: %s curid: %s code: %d", path, curid.c_str(), ir);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    LOGPRINT_EX("connect to MQTT %s id: %s success!", path, curid.c_str());

    return true;
#else
    return false;
#endif
}
bool MyMqttAdapter::m_writeSync(const char *channel, const char *buff, int mbyte)
{
#ifdef MQTT
    int ir;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    pubmsg.payload = (void *)buff;
    pubmsg.payloadlen = mbyte;
    pubmsg.qos = 0;
    pubmsg.retained = 0;
    if (!MQTTClient_isConnected(client))
    {
        MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
        conn_opts.keepAliveInterval = 30;
        conn_opts.cleansession = 0;

        while ((ir = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
        {
            LOGPRINT_EX("trying reconnect MQTT server %s:%s/%s code:%d...", url.c_str(), curid.c_str(), channel, ir);
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        LOGPRINT_EX("reconnect to MQTT success!");
    }
    return MQTTClient_publishMessage(client, channel, &pubmsg, &token);
#else
    return false;
#endif
}
void MyMqttAdapter::m_addSubscribe(const char *channel)
{
#ifdef MQTT
    if (client)
    {
        this->topic = channel;
        MQTTClient_subscribe(client, channel, 0);
        LOGPRINT_EX("MQTT SUBSCRIBE CHANNEL: %s", channel);
    }
#endif
}

int MyMqttAdapter::m_readSync(char *buff, int nbyte)
{
#ifdef MQTT
    MQTTClient_message *message = NULL;
    int ngot = 0, nemp, ir;
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
    while (!message)
    {
        int topicLen = 0;
        char *topicName = NULL;
        int rc = MQTTClient_receive(client, &topicName, &topicLen, &message, 15000);

        if (!MQTTClient_isConnected(client))
        {
            MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
            conn_opts.keepAliveInterval = 30;
            conn_opts.cleansession = 0;
            while ((ir = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
            {
                LOGPRINT_EX("trying reconnect MQTT server %s:%s/%s code:%d...", url.c_str(), curid.c_str(), topic.c_str(), ir);
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
            LOGPRINT_EX("reconnect to MQTT success!");
            if (topic != "")
                m_addSubscribe(topic.c_str());
        }
        if (rc == MQTTCLIENT_SUCCESS && message)
        {
            // fwrite(message->payload, sizeof(char), message->payloadlen, stdout);
            // LOGPRINT(" ");
            int nrem = message->payloadlen - nemp; /* remain data */
            if (nrem > 0)
            {
                /* means the data is more than the buff length */
                memcpy(buff + ngot, message->payload, sizeof(char) * nemp);
                buff_remain = (char *)calloc(nrem, sizeof(char));
                memcpy(buff_remain, (char *)message->payload + nemp, sizeof(char) * nrem);
                nlen = nrem;
                ngot = ngot + nemp; // fill the buff
            }
            else
            {
                /* means the buff is enough to hold the data */
                memcpy(buff + ngot, message->payload, sizeof(char) * message->payloadlen);
                ngot = ngot + message->payloadlen;
            }
            if (topicName)
                MQTTClient_free(topicName);
        }
    }
    MQTTClient_freeMessage(&message);
    return ngot;
#else
    return 0;
#endif
}