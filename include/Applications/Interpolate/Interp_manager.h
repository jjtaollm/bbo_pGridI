#ifndef INCLUDE_APPLICATION_INTERPOLATE_INTERP_MANAGER_H
#define INCLUDE_APPLICATION_INTERPOLATE_INTERP_MANAGER_H
#include "include/Applications/Interpolate/Interp_kriging_smp.h"
#include <mutex>
namespace iono
{
    class Interp_manager : public Callable
    {
    public:
        Interp_manager();
        ~Interp_manager();
        void _interp(int mjd, double sod, bool b_useresidual, map<string, map<int, StecC>> &data, map<string, map<int, StecC>> &real);
        inline void _set_surface_coefficient(map<string, polyCoefficient> &in) { m_coef = in; }

        void _alterConf(map<string, string> payload);

        map<string, pair<double, double>> _get_gridv(int mjd, double sod, int *refsat, map<int, t_Sunit> &sat2v, Station &grid);
        void _set_requests(vector<Station> &in);
        void _setOutputUrl(string url) { m_url_send = url; }
        void _setOutputModuleId(string id) { m_moduleid = id; }

        void _kriging_update_xy(map<string, vector<double>> &s2coor, double lon0, map<string, map<int, StecC>> &data, Station &);
        map<string, map<int, StecC>> _get_residual(map<std::string, polyCoefficient> &coef, map<string, map<int, StecC>> &data);

    protected:
        void _output(int mjd, double sod, map<string, t_gridV> &grid_v, vector<Station> &grids, map<string, map<int, StecC>> &);
        void _output_grids(int mjd, double sod, string f, map<string, t_gridV> &grid_v, vector<Station> &grids, map<string, map<int, StecC>> &data);
        void _output_redis(int mjd, double sod, map<string, t_gridV> &grid_v, vector<Station> &grids);

    public:
        std::mutex mtx;
        string m_url_send, m_moduleid;
        vector<Station> mReqs;
        map<string, polyCoefficient> m_coef;
        ThreadPool *_pool;
        RedisSender m_sender;
        map<string, Interp_kriging_smp *> rq2krig;
        map<time_t, map<int, map<string, double>>> m_r_win_grids; /* residual windows, time_t,isat,site,sig */
        map<time_t, map<int, map<string, double>>> m_r_win_coef;  /* residual windows, time_t,isat,site,sig */
    };
}
#endif