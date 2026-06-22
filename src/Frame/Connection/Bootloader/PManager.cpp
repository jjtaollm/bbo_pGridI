/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-24 18:33:28
 * @ Modified by: Jun Tao
 * @ Modified time: 2024-08-10 11:03:43
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */

#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
#include <cmath>
#include <fstream>
using namespace iono;
using namespace std;
PManager::PManager() : Callable(this)
{
    b_updateConf = false;
    PManager::m_register("COMMAND_UPDATE", std::bind(&PManager::s_alterConf, this, std::placeholders::_1));
}
PManager::~PManager()
{
}
void PManager::s_alterConf(map<string, string> payload)
{
    if (b_updateConf)
    {
        std::unique_lock<std::mutex> lock(jlock);
        b_updateConf = false;
        /* step 1, check configures */
        PConfig t_cnow = s_parseJson(m_jsonConf);
        if (!t_cnow.bok)
            return;
        /* step 2, alter configures, if any changed */
        if (s_updateInput(t_cnow, m_config))
        { // 更新请求信息

            /* update current configures */
            m_config = t_cnow;

            /* output the configures */
            s_outputConf("", m_jsonConf, &t_cnow);
        }
    }
}
bool PManager::s_updateInput(PConfig &t_n, PConfig &t_o)
{
    bool b_conf_update = false;
    Controller *ctl = Controller::s_getInstance();
    if (t_o.redis_exchange != t_n.redis_exchange || t_o.redis_prefix_atm_i != t_n.redis_prefix_atm_i)
    {
        for (auto &kv : ctl->mionReaders)
            delete kv.second;
        /* initial new */
        string cmd = t_n.redis_exchange + "/" + t_n.redis_prefix_atm_i;
        UDAtomReader *reader = new SlantReader_rt(cmd);
        reader->m_setAdapter(&(Controller::s_getInstance()->mionAdapter));
        ctl->mionReaders[cmd] = reader;
        b_conf_update = true;
    }

    if (t_o.o_debugcmd != t_n.o_debugcmd)
    {
        if (t_o.o_debugcmd != "")
        {
            /* delete the older connection */
            if (ctl->mdebugSd)
                delete ctl->mdebugSd;
            ctl->mdebugSd = NULL;
        }
        /* initial new */
        char addr[256] = {0}, port_str[256] = {0}, mnt[256] = {0};
        decodetcppath(t_n.o_debugcmd.c_str(), addr, port_str, NULL, NULL, mnt, NULL);
        ctl->mdebugSd = new RedisSender;
        ctl->mdebugSd->m_setRedisConfigure(addr, atoi(port_str), mnt);
        ctl->mdebugSd->m_StartService();
        b_conf_update = true;
    }

    if (t_o.redis_exchange != t_n.redis_exchange || t_o.redis_prefix_atm_o != t_n.redis_prefix_atm_o)
    {
        string cmd = t_n.redis_exchange + "/" + t_n.redis_prefix_atm_o;
        if (ctl->mInterp)
            ctl->mInterp->_setOutputUrl(cmd);
    }

    if (b_conf_update)
    {
        /* update readers */
        list<string> stalist;
        for (auto &v : t_n.m_baselist)
            stalist.push_back(v);
        map<string, UDAtomReader *> &sr = Controller::s_getInstance()->mionReaders;
        for (auto &kv : sr)
        {
            ((SlantReader_rt *)kv.second)->m_updateSit(stalist);
        }
    }
    /* update request */
    ctl->mInterp->_set_requests(t_n.reqlist);
    ctl->mInterp->_setOutputModuleId(t_n.moduleid);
    /* update Station */
    ctl->mDly.m_updateSit(t_n.m_baseinfo);
    return b_conf_update;
}
void PManager::s_outputConf(string tag, string s, PConfig *info)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    char filename[1024] = {0};
    gtime_t now;
    double ep[6] = {0};
    now.time = time(NULL);
    time2epoch(now, ep);
    {
        sprintf(filename, "%s/iono_config.%04d-%02d-%02d_%02d:%02d:%02d_%s", dly->tracedir, (int)ep[0], (int)ep[1], (int)ep[2], (int)ep[3], (int)ep[4], (int)ep[5], tag.c_str());
        Logtrace::s_defaultlogger.m_openLog(filename);
        int nsz = s.size();
        for (int i = 0; i < ceil(1.0 * nsz / 512); ++i)
        {
            Logtrace::s_defaultlogger.m_wtMsg("@%s %s", filename, s.substr(i * 512, i * 512 > nsz ? nsz - (i - 1) * 512 : 512).c_str());
        }
        Logtrace::s_defaultlogger.m_closeLog(filename);
    }
    if (info)
    {
        sprintf(filename, "%s/iono_config.%04d-%02d-%02d_%02d:%02d:%02d_%s", dly->tracedir, (int)ep[0], (int)ep[1], (int)ep[2], (int)ep[3], (int)ep[4], (int)ep[5], tag.c_str());
        Logtrace::s_defaultlogger.m_openLog(filename);

        Logtrace::s_defaultlogger.m_wtMsg("@%s %-20s = %-40s\n", filename, "i_ephcmd", info->i_ephcmd.c_str());
        Logtrace::s_defaultlogger.m_wtMsg("@%s %-20s = %-40s\n", filename, "redis_exchange", info->redis_exchange.c_str());
        Logtrace::s_defaultlogger.m_wtMsg("@%s %-20s = %-40s\n", filename, "redis_prefix_atm_i", info->redis_prefix_atm_i.c_str());
        Logtrace::s_defaultlogger.m_wtMsg("@%s %-20s = %-40s\n", filename, "redis_prefix_atm_o", info->redis_prefix_atm_o.c_str());
        Logtrace::s_defaultlogger.m_wtMsg("@%s %-20s = %-40s\n", filename, "o_debugcmd", info->o_debugcmd.c_str());

        int i = 0;
        Logtrace::s_defaultlogger.m_wtMsg("@%s \n\n\n\n\n%-20s:%20d\n", filename, "REQLIST", info->reqlist.size());
        for (auto &kv : info->reqlist)
        {
            Logtrace::s_defaultlogger.m_wtMsg("@%s %05d %s %13.3lf %13.3lf %13.3lf %13.3lf %13.3lf %13.3lf\n", filename, ++i, kv.name.c_str(), kv.x[0],
                                              kv.x[1], kv.x[2], kv.geod[0] * R2D, kv.geod[1] * R2D, kv.geod[2]);
        }
        Logtrace::s_defaultlogger.m_closeLog(filename);
    }
}
PConfig PManager::s_parseJson(string jconf)
{
    /* parse json configures */
    PConfig info;
    Json::CharReaderBuilder rbuilder;
    rbuilder["collectComments"] = false;
    Json::Value root;
    JSONCPP_STRING errs;
    istringstream ssin(jconf);
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    bool parse_ok = parseFromStream(rbuilder, ssin, &root, &errs);
    if (!parse_ok)
    {
        LOGPRINT_EX("ERROR, can't parse json configures!");
        return info;
    }
    /* i/o command */
    try
    {
        info.moduleid = root["grid"]["moduleid"].asCString();
        info.i_ephcmd = root["grid"]["i_ephcmd"].asCString();
        info.o_debugcmd = root["grid"]["o_debugcmd"].asCString();
        info.redis_exchange = root["grid"]["redis_exchange"].asCString();
        info.redis_prefix_atm_i = root["grid"]["redis_prefix_atm_i"].asCString();
        info.redis_prefix_atm_o = root["grid"]["redis_prefix_atm_o"].asCString();
    }
    catch (...)
    {
        LOGPRINT_EX("ERROR, parse IO command !");
        return info;
    }
    /* request ids */
    string str;
    set<string> staset;
    int nsz = root["reqlist"].size();
    for (int i = 0; i < nsz; ++i)
    {
        Station sta;
        sta.name = root["reqlist"][i]["reqid"].asCString();
        str = root["reqlist"][i]["stalist"].asCString();
        /* split base station */
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, ','))
        { // 以逗号为分隔符
            staset.insert(item);
        }
        /* split request position */
        str = root["reqlist"][i]["blh"].asCString();
        sscanf(str.c_str(), "%lf%lf%lf", sta.geod, sta.geod + 1, sta.geod + 2);
        sta.geod[0] = sta.geod[0] * D2R;
        sta.geod[1] = sta.geod[1] * D2R;
        blhxyz(sta.geod, 0.0, 0.0, sta.x);
        info.reqlist.push_back(sta);
    }
    info.m_baselist = staset;
    /* station */
    try
    {
        char value[256] = {0};
        string tag = "stalist";
        int nsz = root[tag].size();
        for (int i = 0; i < nsz; ++i)
        {
            Station sta;
            /* parse body */
            sta.name = root[tag][i]["name"].asCString();
            excludeAnnoValue(value, root[tag][i]["xyzenu"].asCString());
            sscanf(value, "%lf%lf%lf", sta.x, sta.x + 1, sta.x + 2);
            ecef2pos(sta.x, sta.geod);

            info.m_baseinfo[sta.name] = sta;
        }
    }
    catch (...)
    {
        LOGPRINT_EX("ERROR, parse stalist configures error !");
        return info;
    }
    /* final step, valid the configures */
    info.bok = true;
    return info;
}
void PManager::s_openUrl(string url)
{
    /* open an thread to read url */
    t_pthJson = thread(&PManager::s_urlRead, this, url); /* open thread to receive url */
}
void PManager::s_urlRead(string url)
{
    int ldebug = false;
    if (ldebug)
    {
        string line, s;
        ifstream ifs("/usr/dev/vdb1/zj/iono_bbo/configures/rt_config.json", ios::in);
        if (ifs.is_open())
        {
            s = "";
            while (getline(ifs, line))
            {
                s = s + line;
            }
            s_inputJsonConf(s);
            ifs.close();
        }
    }
    else
    {
        /* open an redis to read an url */
        char addr[256] = {0}, port_str[256] = {0}, mnt[256] = {0};
        MyRedisAdapter m_redis;
        decodetcppath(url.c_str(), addr, port_str, NULL, NULL, mnt, NULL);

        m_redis.m_setIpPort(addr, atoi(port_str));
        m_redis.m_addSubscribe(mnt);
        while (true)
        {
            /* read from the redis and */
            string s = m_redis.m_readPack();
            if (s != "")
            {
                // printf("%s", s.c_str());
                std::unique_lock<std::mutex> lock(jlock);
                /* update json here */
                s_inputJsonConf(s);
            }
            sleepms(1000);
        }
    }
}

void PManager::s_inputJsonConf(string c_v)
{
    /* outter JSON configure inputter,received from redis */
    if (c_v != m_jsonConf)
    {
        m_jsonConf = c_v;
        b_updateConf = true;
    }
}