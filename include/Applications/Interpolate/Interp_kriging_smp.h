#ifndef INCLUDE_APPLICATION_INTERPLOATE_Interp_kriging_smp_H_
#define INCLUDE_APPLICATION_INTERPLOATE_Interp_kriging_smp_H_
#include "include/Frame/Frame.h"
#include <set>
#include <mutex>
using namespace std;
namespace iono
{
    class t_VariogramM
    {
    public:
        bool b_isok = 0;
        double nugget = 0.0; // NUGGET
        double sill = 0.0;   // PARTIAL SILL; total sill is nugget+sill
        double range = 0.0;  // RANGE (Max distance to consider v(a)=SILL)
        char TYPE = 'S';     // Type of variogram to use, S sepheric, G gaussian,E exponential, L linear
    };

    class t_SampPt
    {
    public:
        int sta_index = -1;
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    class t_Sunit /* interpolate results */
    {
    public:
        int isat = -1;
        double v = 0.0;
        double vsig = 0.0;  /* cross-validation error */
        double vsign = 0.0; /* normal error */
        int nobs = 0;
        double aveDist = 999; /* average distance for intepolate d*/
    };

    class t_gridV
    {
    public:
        void m_reset()
        {
            for (auto &ref : refsat)
                ref = -1;
            gridv.clear();
        }
        int refsat[MAXSYS];
        map<int, t_Sunit> gridv; /*  isat, v */
    };

    class Interp_kriging_smp
    {
    public:
        Interp_kriging_smp(); /* interpolate functions */
        ~Interp_kriging_smp();
        void m_set_pool(ThreadPool *pin) { _pool = pin; }

        map<string, t_gridV> _interp_kriging(int mjd, double sod, int *, vector<Station> grids, map<string, map<int, StecC>> &data);
        inline map<int, map<int, double>> &_get_interp_cv_residual() { return m_residual; } /* residuals */

    protected:
        void _update_window(time_t t, int *refsat, map<int, map<int, double>> &s2idxv, map<time_t, map<int, map<int, double>>> &s2idxv_list);
        void _interp_r_one(int isat, int *refsat, vector<string> &snames, map<string, map<int, StecC>> &data_sd);
        void _interp_r_all(time_t t, int *refsat, vector<string> &snames, std::set<int> &sates, map<string, map<int, StecC>> &data_sd);
        bool _kriging_set_ref(map<std::string, map<int, StecC>> &data, int *ref);                              /* get reference satellite */
        map<std::string, map<int, StecC>> _kriging_form_sd(map<std::string, map<int, StecC>> &data, int *ref); /* form single-difference data */
        t_VariogramM _kriging_est_hyperpam(vector<t_SampPt> &pts, const char type);
        map<string, t_Sunit> _kriging_interp_onesat(time_t, int, vector<Station> &grids, vector<t_SampPt> &pts, t_VariogramM &m);
        vector<t_SampPt> _prepare_samples(vector<string> &snames, vector<int> index, map<std::string, map<int, StecC>> &data, int isat);
        t_VariogramM _kriging_est_hyperpam_spherical(vector<t_SampPt> &pts);
        map<int, double> _kriging_get_gridresidual(vector<t_SampPt> &pts, t_VariogramM &m);

    protected:
        void _process(time_t, int *, vector<string> &snames, std::set<int> &sates, vector<Station> &grids, map<string, map<int, StecC>> &data_sd);
        void _kriging_single_p(time_t, int isat,                       /* processed satellites */
                               vector<string> &snames,                 /* snames for the data */
                               vector<int> &sindex,                    /* station index, that contains */
                               vector<Station> &reqs,                  /* processed requests */
                               map<string, map<int, StecC>> &data_sd); /* data input */

    public:
        ThreadPool *_pool;
        std::mutex _mtx;
        map<string, t_gridV> mGridv;
        map<int, map<int, double>> m_residual;              /* isat -> sta -> v */
        map<time_t, map<int, map<int, double>>> m_r_window; /* residual windows */
    };
}

#endif
