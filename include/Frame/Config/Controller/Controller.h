#ifndef INCLUDE_FRAME_CONTROLLER_H_
#define INCLUDE_FRAME_CONTROLLER_H_
#include <string>
#include <cstdlib>
#include "include/Frame/Frame.h"
#include "include/Applications/Applications.h"
using namespace std;
namespace iono
{
    class Controller
    {
        using t_stec_map = map<string, map<int, StecC>>;
        struct EpochData
        {
            t_stec_map all;
            t_stec_map model;
            t_stec_map verify;
        };
        struct SurfaceResult
        {
            map<string, polyCoefficient> coefficients;
            t_stec_map residuals;
            bool modeled = false;

            bool hasModel() const { return modeled; }
        };

    public:
        Controller();
        ~Controller();
        void m_setArgs(int argc, char *args[]);           /* set up args */
        void startup_GnssProcess(bool);                   /* main entry  */
        inline Deploy *m_getConfigure() { return &mDly; } /* get current configure */
        void m_initInputs(int mjd, double sod, bool bReopen);
        void m_makeUpReaders();
        void m_makeUpdFilter();

    public:
        static Controller *s_getInstance()
        {
            if (sInstance == NULL)
                sInstance = new Controller();
            return sInstance;
        }

    public:
        int m_argc = 0;     /* number of args */
        char *m_args[1024]; /* args */
        Deploy mDly;
        UDAtomAdapter mionAdapter;
        map<string, UDAtomReader *> mionReaders; /* in post mode, each station has a reader, * in realtime, each connection has a reader */
        map<char, polyModel *> mPolyModel;
        Interp_manager *mInterp;

    private:
        void m_initializeProcess(bool bPost);
        EpochData m_readEpoch(bool reopenInputs);
        void m_splitEpochData(EpochData &epoch);
        void m_logEpoch() const;
        SurfaceResult m_runSurface(EpochData &epoch);
        void m_runInterpolation(EpochData &epoch, SurfaceResult &surface);
        bool m_finishEpoch();

        static Controller *sInstance;
    };
}
#endif
