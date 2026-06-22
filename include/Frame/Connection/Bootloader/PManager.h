#ifndef INCLUDE_FRAME_CONNECTION_BOOTLOADER_PMANAGER_H_
#define INCLUDE_FRAME_CONNECTION_BOOTLOADER_PMANAGER_H_
#include "include/Frame/Config/Controller/Station.h"
#include "include/Frame/Connection/InnerLink/Callable.h"
#include <vector>
#include <thread>
#include <mutex>
#include <set>
using namespace std;
namespace iono
{
    class PConfig /* process config*/
    {
    public:
        PConfig()
        {
            bok = false;
            i_ephcmd = "";
            redis_exchange = "";
            redis_prefix_atm_i = "";
            redis_prefix_atm_o = "";
            o_debugcmd = "";
            moduleid = "";
        }
        vector<Station> reqlist;   /* station list */
        string i_ephcmd;           /* real-time atomsphere connection */
        string i_obscmd;           /* debug information */
        string redis_exchange;     /* real-time grid output cmd */
        string redis_prefix_atm_i; /* real-time grid output cmd */
        string redis_prefix_atm_o; /* real-time debug or display information output cmd */
        string o_debugcmd;         /* debug information */
        set<string> m_baselist;
        map<string, Station> m_baseinfo;
        string moduleid;
        bool bok;
    };
    class PManager : public Callable
    {
    public:
        PManager();
        ~PManager();

    public:
        void s_openUrl(string s);

        void s_urlRead(string url);
        PConfig s_parseJson(string jconf);
        void s_alterConf(map<string, string> payload);
        void s_outputConf(string tag, string s, PConfig *info);
        bool s_updateInput(PConfig &t_now, PConfig &t_o);
        void s_inputJsonConf(string c_v);

    protected:
        string m_jsonConf;
        PConfig m_config;
        std::thread t_pthJson;
        std::mutex jlock;
        bool b_updateConf;
    };
}
#endif