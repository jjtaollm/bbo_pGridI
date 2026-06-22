//
// Created by juntao, at wuhan university   on 2020/11/2.
//
#include <cstdarg>
#include <cstring>
#include <time.h>
#include "include/Frame/Frame.h"
#include "include/Frame/Config/Controller/Controller.h"
using namespace iono;
Log Logtrace::s_defaultlogger;
time_t Logtrace::s_st = time(NULL);
std::mutex Logtrace::q_m;
void Logtrace::m_logPrint(const char *fmt_msg, ...)
{
    char log_msg_buff[8192], tme[256];
    va_list args;
    va_start(args, fmt_msg);
    vsprintf(log_msg_buff, fmt_msg, args);
    va_end(args);
    printf("[%s LOCAL]-%s", runlocaltime(tme), log_msg_buff);
}
void Logtrace::m_logError(const char *fmt_msg, ...)
{
    std::unique_lock<std::mutex> ql(q_m);
    char log_msg_buff[40960] = {0};
    va_list args;
    va_start(args, fmt_msg);
    vsprintf(log_msg_buff, fmt_msg, args);
    va_end(args);
    perror(log_msg_buff);
}
void Logtrace::m_logTrace(int level, const char *fmt_msg, ...)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    char log_msg_buff[4096], tme[256];
    if (level < dly->tracelevel || !dly->ltrace)
        return;
    va_list args;
    va_start(args, fmt_msg);
    vsprintf(log_msg_buff, fmt_msg, args);
    va_end(args);
    s_defaultlogger.m_wtMsg("@%s [%s LOCAL]-%s", dly->tracefile, runlocaltime(tme), log_msg_buff);
}
void Logtrace::m_logtagTrace(const char *tag, const char *fmt_msg, ...)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    char log_msg_buff[4096], tme[256];
    va_list args;
    va_start(args, fmt_msg);
    vsprintf(log_msg_buff, fmt_msg, args);
    va_end(args);
    s_defaultlogger.m_wtMsg("@%s [%s LOCAL]-[%s]-%s", dly->statfile, runlocaltime(tme), tag, log_msg_buff);
}
void Logtrace::m_logNoFormatTrace(const char *fmt_msg, ...)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    char log_msg_buff[4096];
    va_list args;
    va_start(args, fmt_msg);
    vsprintf(log_msg_buff, fmt_msg, args);
    va_end(args);
    s_defaultlogger.m_wtMsg("@%s %s", dly->tracefile, log_msg_buff);
}
void Logtrace::m_logStat(bool bnewline, const char *fmt_msg, ...)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    char log_msg_buff[4096];
    va_list args;
    va_start(args, fmt_msg);
    vsprintf(log_msg_buff, fmt_msg, args);
    va_end(args);
    if (bnewline)
        s_defaultlogger.m_wtMsg("@%s %s\n", dly->statfile, log_msg_buff);
    else
        s_defaultlogger.m_wtMsg("@%s %s", dly->statfile, log_msg_buff);
}
string Logtrace::m_logRunTime()
{
    char svrMsg[2048] = {0};
    int i, span[4], difsecond, div[] = {86400, 3600, 60, 1};
    time_t now;
    time(&now);
    difsecond = now - s_st;
    for (i = 0; i < 4; i++)
    {
        span[i] = difsecond / div[i];
        difsecond -= span[i] * div[i];
    }
    sprintf(svrMsg, "%d(d)-%02d(h)-%02d(m)-%02d(s)", span[0], span[1], span[2], span[3]);
    return string(svrMsg);
}
void Logtrace::m_logStatInfo(const char *fmt_msg, ...)
{
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    char log_msg_buff[4096] = {0}, tme[256] = {0};
    va_list args;
    va_start(args, fmt_msg);
    vsprintf(log_msg_buff, fmt_msg, args);
    va_end(args);
    s_defaultlogger.m_wtMsg("@%s [%s LOCAL] %s", dly->statfile, runlocaltime(tme), log_msg_buff);
}
void Logtrace::m_logMatrix(double *mat, int lda, int row, int col, int p, int q, bool ball, const char *premsg)
{
    int i, j;
    Deploy *dly = Controller::s_getInstance()->m_getConfigure();
    s_defaultlogger.m_wtMsg("@%s %s\n", dly->statfile, premsg);
    if (row < 0 || col < 0 || lda < 0)
    {
        printf("lda = %d,row = %d,col = %d,wrong input arguments for the row and col", lda, row, col);
        return;
    }
    int *lv = (int *)calloc(sizeof(int), col);
    for (i = 0; i < col; i++)
        lv[i] = 1;
    if (!ball)
    {
        memset(lv, 0, sizeof(int) * col);
        for (j = 0; j < col; j++)
            for (i = 0; i < row; i++)
                if (mat[i + j * lda] != 0)
                {
                    lv[j] = 1;
                    break;
                }
    }
    for (i = 0; i < row; i++)
    {
        for (j = 0; j < col; j++)
        {
            if (lv[j])
                s_defaultlogger.m_wtMsg("@%s %*.*lf ", dly->statfile, p, q, mat[i + j * lda]);
        }
        s_defaultlogger.m_wtMsg("@%s \n", dly->statfile);
    }
    free(lv);
}