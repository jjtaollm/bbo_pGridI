/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-22 14:53:53
 * @ Modified by: Jun Tao
 * @ Modified time: 2024-08-23 19:44:33
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */

#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace iono;

SlantReader_rt::SlantReader_rt(string cmd) : UDAtomReader(cmd)
{
    shouldStop = false;
    len = nbyte = 0;
    payload = NULL;
    memset(lenbuf, 0, sizeof(lenbuf));
}
SlantReader_rt::~SlantReader_rt()
{
    shouldStop = true;
    cv.notify_one();
    if (t_pthRecv.joinable())
        t_pthRecv.join();
}
void SlantReader_rt::v_open(string cmd)
{
    isOpen = true;
    t_pthRecv = thread(&SlantReader_rt::s_channelRead, this, mtag);
}
void SlantReader_rt::v_read(int mjd, double sod)
{
    /* empty implement */
}
void SlantReader_rt::v_close()
{
    /* empty implement */
}
void SlantReader_rt::s_channelRead(string url)
{
    int nread, i;
    char buff[1024];
    char command[2048];
    char addr[256] = {0}, port_str[256] = {0};
    decodetcppath(url.c_str(), addr, port_str, NULL, NULL, NULL, NULL);

    // 找到最后一个 '/'
    size_t pos = url.find_last_of('/');

    // 提取 '/' 后面的部分（不包括 '/'）
    std::string mnt = (pos == std::string::npos) ? url : url.substr(pos + 1);

    myRedis.m_setIpPort(addr, atoi(port_str));
    while (!shouldStop)
    {
        time_t now = time(NULL);
        list<string> stacache;
        {
            std::unique_lock<std::mutex> lk(mtx);
            stacache = mcaclist;
        }
        auto start = std::chrono::steady_clock::now();
        map<string, string> msgcache;
        for (auto &sta : stacache)
        {
            sprintf(command, "GET %s:ion:%s", mnt.c_str(), sta.c_str());
            string s = myRedis.m_redisGet(command);
            if (s == "")
                continue;
            if (msgcache.count(sta) == 0 || msgcache[sta] != s)
            {
                for (auto &c : s)
                    m_inputData(c); /* 如果是正常的STEC，就会被丢到adapter中 */
                msgcache[sta] = s;  /* 避免重复丢入 */
            }
        }
        // 计算下一个整秒点
        auto next_second = start + std::chrono::seconds(1);
        std::this_thread::sleep_until(next_second);

        if (now % 5 == 0)
        {
            LOGPRINT_EX("Current Redis Station Count: %d ", stacache.size());
        }
    }
}

int SlantReader_rt::m_inputData(unsigned char data)
{
    if (nbyte == 0)
    {
        if (data != ATOMPREAMB_UD)
            return 0;
    }
    if (nbyte < 5)
    {
        lenbuf[nbyte++] = data;
        if (nbyte == 5)
        {
            if (payload != NULL)
            {
                free(payload);
                payload = NULL;
            }
            len = getbitu((unsigned char *)lenbuf, 8, 32) + 5; /* length without parity */
            payload = (char *)calloc(len + 4, sizeof(char));
            memcpy(payload, lenbuf, sizeof(char) * 5);
        }
        return 0;
    }
    payload[nbyte++] = data;
    if (nbyte < len + 4)
        return 0;
    nbyte = 0;
    if (rtk_crc24q((unsigned char *)payload, len) !=
        getbitu((unsigned char *)payload, len * 8, 32))
    {
        LOGPRINT("Decode parity error:len = %d", len);
        return 0;
    }
    m_decode();
    return 1;
}
void SlantReader_rt::m_decode()
{
    char s_base[8] = {0}, s_rover[8] = {0}, s_cprn[4] = {0};
    time_t tt;
    double ddion, ddtrp, sod, geod_r[3], geod_b[3], span;
    int is_bds23cont_stable;
    int i = 5 * 8, isat, nsat, iset, iprn, nfix, mjd, week, elev, azim, type, zwd, n1, ifix, virtual_rv, virtual_bs;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();

    type = getbitu((unsigned char *)payload, i, 8);
    i += 8;
    memcpy(s_rover, payload + i / 8, sizeof(char) * 4);
    i += 32;
    /* add whether it is virtual station */
    virtual_rv = getbitu((unsigned char *)payload, i, 8);
    i += 8;
    memcpy(s_base, payload + i / 8, sizeof(char) * 4);
    i += 32;
    /* add whether it is virtual station */
    virtual_bs = getbitu((unsigned char *)payload, i, 8);
    i += 8;

    for (int is = 0; is < 2; ++is) /* getting xrov coordinate here */
    {
        geod_r[is] = 1.0 * getbits((unsigned char *)payload, i, 32) / 1e7;
        i += 32;
    }
    geod_r[2] = 1.0 * getbits((unsigned char *)payload, i, 32) / 100.0;
    i += 32;

    for (int is = 0; is < 2; ++is) /* getting xbas coordinate here */
    {
        geod_b[is] = 1.0 * getbits((unsigned char *)payload, i, 32) / 1e7;
        i += 32;
    }
    geod_b[2] = 1.0 * getbits((unsigned char *)payload, i, 32) / 100.0;

    i += 32;

    mjd = getbitu((unsigned char *)payload, i, 16);
    i += 16;
    sod = getbitu((unsigned char *)payload, i, 24);
    i += 24;

    zwd = getbits((unsigned char *)payload, i, 32);
    i += 32;

    zwd = getbits((unsigned char *)payload, i, 32);
    i += 32;

    nsat = getbitu((unsigned char *)payload, i, 16);
    i += 16;

    is_bds23cont_stable = getbitu((unsigned char *)payload, i, 8);
    i += 8;

    map<int, StecC> sat2s;
    for (isat = 0; isat < nsat; ++isat)
    {
        memcpy(s_cprn, payload + i / 8, sizeof(char) * 3);
        i += 24;
        ddion = getbits((unsigned char *)payload, i, 24);
        i += 24;
        ddtrp = getbits((unsigned char *)payload, i, 24);
        i += 24;

        ifix = getbits((unsigned char *)payload, i, 16);
        i += 16;

        elev = getbits((unsigned char *)payload, i, 16);
        i += 16;
        azim = getbits((unsigned char *)payload, i, 16);
        i += 16;
        n1 = getbits((unsigned char *)payload, i, 24);
        i += 24;
        span = getbits((unsigned char *)payload, i, 24);
        i += 24;

        if (-1 == (iprn = pointer_string(dly->nprn, dly->cprn, s_cprn)))
            continue;

        StecC stec;
        stec.isat = pointer_string(dly->nprn, dly->cprn, s_cprn);
        strcpy(stec.name, s_rover);
        stec.ionva = ddion / 10000.0;
        stec.elev = elev / 1000.0 * RAD2DEG;
        stec.azim = azim / 1000.0 * RAD2DEG;
        stec.t_r = mjd2time(mjd, sod);
        if (stec.elev > 30)
            stec.R = 1.0;
        else
            stec.R = sin(stec.elev * DEG2RAD) * 2;

        blhxyz(geod_r, 0.0, 0.0, stec.x);
        sat2s[stec.isat] = stec;
    }
    if (!sat2s.empty())
        m_adapter->m_inputIono(sat2s);
    /* update station list */
}
void SlantReader_rt::m_updateSit(list<string> &vlist)
{
    std::lock_guard<std::mutex> lk(mtx);
    mcaclist = vlist;
}