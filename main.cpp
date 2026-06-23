#include "include/Frame/Config/Controller/Controller.h"
using namespace iono;
static bool s_check_lpost(int nargc, char *args[])
{
    for (int iargc = 1; iargc < nargc; ++iargc)
    {
        if (strstr(args[iargc], "-post"))
            return true;
    }
    return false;
}
int main(int nargc, char *args[])
{
    Controller::s_getInstance()->m_setArgs(nargc, args);
    Controller::s_getInstance()->startup_GnssProcess(s_check_lpost(nargc, args));
}