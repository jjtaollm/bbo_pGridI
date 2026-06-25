/**
 * @ Author: Jun Tao
 * @ Create Time: 2024-07-28 21:34:30
 * @ Modified by: Jun Tao
 * @ Modified time: 2024-12-07 16:19:22
 * @ Description: Wuhan University, Contact: jtaowhu@whu.edu.cn
 */

#include "include/Frame/Frame.h"
#include "include/Applications/Applications.h"
#include "include/Frame/Config/Controller/Controller.h"
#include <cmath>
using namespace iono;
Controller *Controller::sInstance = NULL;
Controller::Controller()
{
    mInterp = NULL;
}
Controller::~Controller()
{
    if (mInterp)
        delete mInterp;
    for (auto &[_, v] : mPolyModel)
        delete v;
}

void Controller::m_setArgs(int argc, char *args[])
{
    this->m_argc = argc;
    memcpy(this->m_args, args, sizeof(char *) * argc);
}
void Controller::m_makeUpReaders()
{
    for (auto &kv : mDly.mSta)
    {
        UDAtomReader *reader = new SlantReader_post(kv.second.name);
        reader->m_setAdapter(&mionAdapter);
        mionReaders[kv.second.name] = reader;
    }
    /* set request Reader */
    if (!strstr(mDly.m_addargs, "-skipR"))
    {
        for (auto &kv : mDly.mPostReqs)
        {
            UDAtomReader *reader = new SlantReader_post(kv.first);
            reader->m_setAdapter(&mionAdapter);
            mionReaders[kv.first] = reader;
        }
    }
}
void Controller::m_makeUpdFilter()
{
    if (strstr(mDly.m_mode, "POLY"))
    {
        for (int isys = 0; isys < mDly.nsys; ++isys)
            mPolyModel[mDly.system[isys]] = new iono::polyModel(mDly.system[isys]);
    }

    if (strstr(mDly.m_mode, "INTERP"))
    {
        mInterp = new Interp_manager();
        if (mDly.lpost)
        {
            vector<Station> reqs;
            reqs.reserve(mDly.mPostReqs.size());
            for (const auto &[_, station] : mDly.mPostReqs)
                reqs.push_back(station);
            mInterp->_set_requests(reqs);
        }
    }
}

void Controller::m_initializeProcess(bool bPost)
{
    mDly.m_readJsonFile(m_argc, m_args);
    m_makeUpReaders();
    m_makeUpdFilter();
}

/// @brief TODO:
/// @param epoch
void Controller::m_splitEpochData(EpochData &epoch)
{
    for (const auto &[station, _] : mDly.mSta)
    {
        auto found = epoch.all.find(station);
        if (found != epoch.all.end())
            epoch.model[station] = found->second;
    }

    for (const auto &[station, observations] : epoch.all)
    {
        if (epoch.model.count(station) == 0)
            epoch.verify[station] = observations;
    }
}
Controller::EpochData Controller::m_readEpoch(bool reopenInputs)
{
    m_initInputs(mDly.mjd, mDly.sod, reopenInputs);
    for (auto &[_, reader] : mionReaders)
        reader->v_read(mDly.mjd, mDly.sod);

    EpochData epoch;
    epoch.all = mionAdapter.m_inquireIono(mDly.mjd, mDly.sod, MAXWND);
    m_splitEpochData(epoch);
    return epoch;
}

void Controller::m_logEpoch() const
{
    gtime_t current = utc2gpst(timeget());
    LOGPRINT("processing [%d %9.1lf]...%d (delay)", mDly.mjd, mDly.sod,
             current.time - mjd2time(mDly.mjd, mDly.sod).time)
}

Controller::SurfaceResult Controller::m_runSurface(EpochData &epoch)
{
    SurfaceResult result;
    if (!strstr(mDly.m_mode, "POLY") || mPolyModel.empty() || epoch.all.empty())
        return result;
    result.modeled = true;

    string outputFile;
    const time_t epochTime = mjd2time(mDly.mjd, mDly.sod).time;
    for (auto &[_, model] : mPolyModel)
    {
        model->v_run(mDly.mjd, mDly.sod, epoch.model);
        result.coefficients.insert(model->m_coef.begin(), model->m_coef.end());
        outputFile = model->m_inquire_outccf();

        if (mInterp && strstr(mDly.m_mode, "INTERP"))
        {
            t_stec_map residual = model->_get_residual(epochTime, model->m_coef, epoch.model);
            for (auto &[station, observations] : residual)
                result.residuals[station].insert(observations.begin(), observations.end());
        }
    }
    if (!outputFile.empty())
        polyModel::_output_coeff(mDly.mjd, mDly.sod, outputFile, result.coefficients);
    return result;
}

void Controller::m_runInterpolation(EpochData &epoch, SurfaceResult &surface)
{
    if (!strstr(mDly.m_mode, "INTERP") || !mInterp || epoch.model.empty())
        return;

    Log::s_tmeTag("INTERP");
    if (surface.hasModel())
    {
        mInterp->_set_surface_coefficient(surface.coefficients);
        mInterp->_interp(mDly.mjd, mDly.sod, surface.residuals, epoch.all);
    }
    else
    {
        mInterp->_interp(mDly.mjd, mDly.sod, epoch.model, epoch.all);
    }
    Log::s_tmeConsume("INTERP");
}

bool Controller::m_finishEpoch()
{
    if (mDly.lpost)
    {
        timinc(mDly.mjd, mDly.sod, mDly.dintv, &mDly.mjd, &mDly.sod);
        if ((mDly.mjd - mDly.mjd1) * 86400.0 + mDly.sod - mDly.sod1 >= 0.0)
            return false;
    }
    Log::s_tmeConsume("Epoch");
    return true;
}

void Controller::startup_GnssProcess(bool b_post)
{
    m_initializeProcess(b_post);
    if (mDly.lpost)
    {
        mDly.mjd = mDly.mjd0;
        mDly.sod = mDly.sod0;
    }
    if (strstr(mDly.m_mode, "DSTEC"))
    {
        bbo_dstec stec;
        stec.m_process();
        return;
    }
    int mjdSync = -1;
    while (true)
    {
        Log::s_tmeTag("Epoch");
        EpochData epoch = m_readEpoch(mjdSync != mDly.mjd);
        mjdSync = mDly.mjd;
        m_logEpoch();
        SurfaceResult surface = m_runSurface(epoch);
        m_runInterpolation(epoch, surface);

        if (!m_finishEpoch())
            break;
    }
}
/* initial requirements */
void Controller::m_initInputs(int mjd, double sod, bool b_reOpen)
{
    if (b_reOpen)
    {
        for (auto &kv : mionReaders)
            if (kv.second->v_isOpen())
                kv.second->v_close();
    }
    for (auto &kv : mionReaders)
    {
        if (!kv.second->v_isOpen())
            kv.second->v_open(toString(mjd) + ":" + toString(sod));
    }
}
