/*
 * SmoothAdapter.cpp
 *
 *  Created on: 2020/09/08 13:59:50
 *      Author: juntao, at wuhan university
 */
#include "include/Frame/Frame.h"
#include <algorithm>
using namespace std;
using namespace iono;
SmoothAdapter::SmoothAdapter()
{
    nmax = 100;
    ncur = 0;
    mean = 0.0;
    xsig = 999;
}
SmoothAdapter::SmoothAdapter(int max)
{
    if (max == 0)
    {
        cout << "max = 0,incorrect max value!" << endl;
        exit(1);
    }
    nmax = max;
    ncur = 0;
    mean = 0.0;
    xsig = 999;
}
void SmoothAdapter::m_smooth(double value, double &omean, double &oxsig, double inxsig)
{
    mean = (mean * ncur + value) / (ncur + 1);
    if (ncur < nmax)
    {
        if (ncur == 0)
        {
            xsig = inxsig;
        }
        else
        {
            xsig = sqrt((xsig * xsig * ncur + (value - mean) * (value - mean)) / (ncur + 1));
        }
        values.push_back(value);
        dvalues.push_back(value - mean);
        omean = mean;
        oxsig = xsig;
        ncur++;
    }
    else
    {
        double front = values.front();
        double frontdv = dvalues.front();
        mean = mean + (value - front) / ncur;
        xsig = sqrt(xsig * xsig + ((value - mean) * (value - mean) - frontdv * frontdv) / ncur);
        values.pop_front();
        dvalues.pop_front();

        values.push_back(value);
        dvalues.push_back(value - mean);

        omean = mean;
        oxsig = xsig;
    }
}
void SmoothAdapter::m_smooth(double value, double &omean, double &oxsig)
{
    if (ncur < nmax)
    {
        mean = (mean * ncur + value) / (ncur + 1);
        if (ncur > 0)
        {
            xsig = sqrt((xsig * xsig * (ncur - 1) + (value - mean) * (value - mean)) / ncur);
        }
        values.push_back(value);
        dvalues.push_back(value - mean);

        omean = mean;
        oxsig = xsig;
        ncur++;
    }
    else
    {
        double front = values.front();
        double frontdv = dvalues.front();
        mean = mean + (value - front) / ncur;
        xsig = sqrt(xsig * xsig + ((value - mean) * (value - mean) - frontdv * frontdv) / (ncur - 1));
        values.pop_front();
        dvalues.pop_front();

        values.push_back(value);
        dvalues.push_back(value - mean);

        omean = mean;
        oxsig = xsig;
    }
}

void SmoothAdapter::m_smooth(int tag, double value, double &omean, double &oxsig)
{
    if (ncur < nmax)
    {
        mean = (mean * ncur + value) / (ncur + 1);
        if (ncur > 0)
        {
            xsig = sqrt((xsig * xsig * (ncur - 1) + (value - mean) * (value - mean)) / ncur);
        }
        tags.push_back(tag);
        values.push_back(value);
        dvalues.push_back(value - mean);

        omean = mean;
        oxsig = xsig;
        ncur++;
    }
    else
    {
        double front = values.front();
        double frontdv = dvalues.front();
        mean = mean + (value - front) / ncur;
        xsig = sqrt(xsig * xsig + ((value - mean) * (value - mean) - frontdv * frontdv) / (ncur - 1));
        values.pop_front();
        dvalues.pop_front();
        tags.pop_front();

        values.push_back(value);
        dvalues.push_back(value - mean);
        tags.push_back(tag);

        omean = mean;
        oxsig = xsig;
    }
}
void SmoothAdapter::m_remove(function<bool(int)> fun)
{
    list<int>::iterator itr;
    list<double>::iterator vitr, ditr;
    for (itr = tags.begin(), vitr = values.begin(), ditr = dvalues.begin();
         itr != tags.end();)
    {
        if (fun(*itr))
        {
            itr = tags.erase(itr);
            vitr = values.erase(vitr);
            ditr = dvalues.erase(ditr);
            continue;
        }
        ++itr;
        ++vitr;
        ++ditr;
    }
    ncur = values.size();
}
void SmoothAdapter::m_reset()
{
    ncur = 0;
    mean = xsig = 0.0;
    values.clear();
    dvalues.clear();
    tags.clear();
}
void SmoothAdapter::m_setmax(int in)
{
    nmax = in;
}
