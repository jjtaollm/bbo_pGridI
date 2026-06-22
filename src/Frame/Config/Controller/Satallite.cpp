/*
 * Satallite.cpp
 *
 *  Created on: 2018/8/26
 *      Author: juntao, at wuhan university
 */
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include "include/Frame/Frame.h"
using namespace std;
using namespace iono;
Satellite::Satellite()
{
    ifreq = 0;
    mass = 0.0;
    memset(xyz, 0, sizeof(xyz));
    memset(offs, 0, sizeof(offs));
    memset(satpos, 0, sizeof(satpos));
    xclk = 0.0;
}
Satellite::~Satellite() {}
