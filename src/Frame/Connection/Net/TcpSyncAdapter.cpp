/*
 * TcpSyncAdapter.cpp
 *
 *  Created on: 2020/01/20 10:07:14
 *      Author: juntao
 */
#include "include/Frame/Frame.h"
#ifndef _WIN32
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
using namespace std;
using namespace iono;
// #define DEBUG_POST

TcpSyncAdapter::TcpSyncAdapter()
{
    sockid = -1;
    port = -1;
}
TcpSyncAdapter::~TcpSyncAdapter()
{
    m_close();
}
bool TcpSyncAdapter::m_connect(const char *path)
{
#ifndef DEBUG_POST
    /* decode tcp/ntrip path */
    char addr[256] = {0}, port_str[256] = {0};
    decodetcppath(path, addr, port_str, NULL, NULL, NULL, NULL);
    if (!(sockid = m_openTcp(addr, atoi(port_str))))
        return false;
#endif
    return true;
}
int TcpSyncAdapter::m_setsock(int sock)
{
    struct timeval tv = {10, 0};
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) == -1)
    {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return 0;
    }
    return 1;
}
int TcpSyncAdapter::m_openTcp(string ip_in, int port_in)
{
    struct hostent *hp;
    struct sockaddr_in server;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (!m_setsock(sock))
    {
        return 0;
    }
    this->ip = ip_in;
    this->port = port_in;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    // #ifdef _WIN32
    if (!(hp = gethostbyname(ip.c_str())))
    {
        LOGPRINT("ip = %s,port = %d,opening stream socket error", ip.c_str(), port);
        close(sock);
    }
    memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
    // #else
    // 	inet_aton(ip.c_str(), &server.sin_addr);
    // #endif
    if (connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0)
    {
        LOGPRINT("ip = %s,port = %d,connecting stream socket error", ip.c_str(), port);
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return 0;
    }
    else
    {
        LOGPRINT("Connect success");
        return sock;
    }
}
void TcpSyncAdapter::m_close()
{
#ifdef _WIN32
    closesocket(sock);
#else
    close(sockid);
#endif
}
int TcpSyncAdapter::m_readSync(char *buff, int len)
{
#ifndef DEBUG_POST
    int nread, timeout = 60;
    time_t in = time(NULL);
    memset(buff, 0, sizeof(char) * len);
    while (1)
    {
        nread = recv(sockid, buff, len, 0);
        if (nread > 0)
            break;
        else if (nread <= 0)
        {
            if (nread < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
            {
                if (time(NULL) - in < timeout)
                {
                    continue;
                }
            }
#ifdef _WIN32
            closesocket(sockid);
#else
            close(sockid);
#endif
            LOGPRINT("connection lost,reconnecting...");
            while (!(sockid = m_openTcp(ip, port)))
            {
                sleepms(3000);
            }
            time(&in);
            continue;
        }
    }
    return nread;
#else
    int nread;
    const char *pf = "/data/jtao/work/data/rinex/2021/187/HB011870.21b";
    static FILE *fp = fopen(pf, "rb");
    if (NULL == fp)
    {
        printf("file = %s,can't open file to read!", pf);
        exit(1);
    }
    nread = fread(buff, sizeof(char), len, fp);
    return nread;
#endif
}
