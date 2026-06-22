/*
 * SmoothAdapter.h
 *
 *  Created on: 2020/09/08 13:32:19
 *      Author: juntao, at wuhan university
 */

#ifndef INCLUDE_QC_SMOOTHADAPTER_H_
#define INCLUDE_QC_SMOOTHADAPTER_H_
#include <iostream>
#include <list>
#include <vector>
#include <cmath>
#include <algorithm>
#include <functional>
using namespace std;
namespace iono
{
	class SmoothAdapter
	{
	public:
		SmoothAdapter();
		SmoothAdapter(int max);
		void m_smooth(double value, double &omean, double &oxsig, double inxsig);
		void m_smooth(double value, double &omean, double &oxsig);
		void m_smooth(int tag, double value, double &omean, double &oxsig);
		void m_reset();
		void m_setmax(int in);
		void m_remove(function<bool(int)> fun);
		inline double m_getmean() { return mean; }
		inline double m_getxsig() { return xsig; }
		inline int m_getcount() { return ncur; }

	private:
		double mean, xsig;
		int ncur, nmax;
		list<int> tags;
		list<double> values;
		list<double> dvalues;
	};
}
#endif /* INCLUDE_QC_SMOOTHADAPTER_H_ */
