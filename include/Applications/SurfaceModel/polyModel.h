#ifndef INCLUDE_APPLICATIONS_SURFACEMODEL_POLYMODEL_H_
#define INCLUDE_APPLICATIONS_SURFACEMODEL_POLYMODEL_H_
#include "include/Frame/Frame.h"
#include <map>
#include <cmath>
#include <vector>
#include <Eigen/Dense>
using namespace std;
namespace iono
{
#define redundancySite 7
    class polyCoefficient
    {
    public:
        polyCoefficient()
        {
            isat = -1;
            t0 = 0;
            lon0 = lat0 = 0.0;
            fit_rms = 0.0;
            esig = 0.0;
            fit_rms_all = 0.0;
            esig_all = 0.0;
            n = m = 0;
            nobs = 0;
            ndof = 0;
            nobs_all = 0;
            ndof_all = 0;
        }
        time_t t0;
        double lon0;
        double lat0;
        double esig;
        double esig_all;
        double fit_rms;
        double fit_rms_all;
        int n;
        int m;
        int nobs;
        int ndof;
        int nobs_all;
        int ndof_all;
        int isat;
        vector<double> coef;
        /// @brief return model value
        /// @param lat0 in degrees
        /// @param lon0 in degrees
        /// @return
        double m_getModelV(time_t time, double lat, double lon)
        {
            double dif_lat = (lat - lat0);
            double dif_lon = (lon - lon0) + ((time - t0) / 3600.0 * 15);

            double v = 0.0;
            for (int i = 0; i < (n + 1); ++i)
            {
                for (int j = 0; j < (m + 1); ++j)
                {
                    double cc = pow(dif_lat, i) * pow(dif_lon, j);
                    v = v + cc * coef[i * (m + 1) + j];
                }
            }
            return v;
        }
    };

    class polypm
    {
    public:
        polypm(int isat, char csys, string s, int tp, int i, int j);
        ~polypm();
        char csys;
        int isat;
        string sname;
        int tp;
        int i;
        int j;
        int iobs;
        double xest;
        double xini;
        double xcor;
    };
    class StecC; // 前向声明
    class polyModel
    {
    public:
        polyModel(int n, int m, double lon0, double lat0, char csys);
        using t_key2v = map<int, StecC>;
        polyModel(char csys);
        virtual ~polyModel();
        string m_inquire_outccf()
        {
            string f = KvContainer::getv(m_tag_coef_fil);
            if (f != "" && Logtrace::s_defaultlogger.m_lexist(f))
                return f;
            return "";
        }

        virtual void v_run(int mjd, double sod, map<string, t_key2v> &data);
        /* to update residuals */
        map<string, map<int, StecC>> _get_residual(time_t time, map<string, polyCoefficient> &coef, map<string, map<int, StecC>> &data);
        /* calculate per-satellite and whole-model residual statistics */
        void _assign_fit_statistics(map<string, polyCoefficient> &coef);
        /* get coefficient */
        inline map<string, polyCoefficient> m_getCoef() { return m_coef; }
        /* set span */
        inline void m_setSpan(double t) { span = t; }

    public:
        /* configures */
        void _union_data(time_t t, map<string, t_key2v> &data);
        void _read_DCB(int mjd, double sod, bool breset);
        int _get_estimated_list(map<time_t, map<string, t_key2v>> &, int *sat2o);
        void _update_estimated_conf(time_t t0, map<time_t, map<string, t_key2v>> &);
        int _idx_pm(int isat, char csys, string ssta, int tp, int i, int j);

    public:
        /* estimated functions */
        vector<double> _update_coeff(double dif_lat, double dif_lon, StecC &s, bool bunify);
        void _init_DCB_pam();
        void _add_DCB_prior(double *AtA, int lda);
        void _estimated(map<time_t, map<string, map<int, StecC>>> &data, int nobs, int *sat2no);
        int _residual_check(map<time_t, map<string, t_key2v>> &, double *r, int (*nb2idx)[4], int nb);
        int _form_A_matrix(map<time_t, map<string, t_key2v>> &data, double *A, int ldaA, double *L, int *sat2o, int (*nb2idx)[4], bool bunify = true);
        int m_form_norm_and_solve(int nb, double *A, int lda, double *L, int ldaL, double *, int);
        void _add_single_constraint(double *AtA, int lda, char);
        bool _add_center_constraint(double *AtA, int lda, char csys, bool);

        bool _b_gotDCBCorrection(string name, char csys);
        void _update_s2o(map<time_t, map<string, t_key2v>> &data, int *sat2o);

    public:
        /* output functions */
        void _output(int mjd, double sod, map<string, map<int, StecC>> &data);
        static void _output_coeff(int mjd, double sod, string f, std::map<std::string, polyCoefficient> &coef);
        void _output_residual(int mjd, double sod, string f, map<string, map<int, StecC>> &res);

        map<string, polyCoefficient> _get_estimated_coeff();

    public:
        double mlat0;
        double mlon0;
        double mesig;
        int mN = 2;
        int mM = 2;
        int mjdsync = -1;
        gtime_t mt0 = {0};
        double span = 15;
        vector<polypm> m_Pm;
        vector<string> sta_list;
        vector<string> sat_list;
        DCBAdapter dcbAdapter;
        DCBReader reader;
        map<string, polyCoefficient> m_coef;
        map<time_t, map<string, t_key2v>> m_data;
        char m_csys;
        string m_tag_res_fil;
        string m_tag_coef_fil;
    };
}
#endif
