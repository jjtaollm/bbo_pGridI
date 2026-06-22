//
// Created by juntao, at wuhan university   on 2020/11/2.
//

#ifndef BAMBOO_LOGTRACE_H
#define BAMBOO_LOGTRACE_H
#include "include/Frame/Config/Const.h"
#include "Log.h"
#include <mutex>
namespace iono
{
    class Logtrace
    {
    public:
        /// functions
        static std::string m_logRunTime();
        static void m_logTrace(int level, const char *fmt_msg, ...);
        static void m_logtagTrace(const char *tag, const char *fmt_msg, ...);
        static void m_logNoFormatTrace(const char *fmt_msg, ...);
        static void m_logStat(bool, const char *fmt_msg, ...);
        static void m_logStatInfo(const char *fmt_msg, ...);
        static void m_logMatrix(double *mat, int lda, int row, int col, int p, int q, bool ball, const char *premsg);
        static void m_logPrint(const char *fmt_msg, ...);
        static void m_logError(const char *fmt_msg, ...);

    public:
        /// variables
        static time_t s_st;
        static Log s_defaultlogger;
        static mutex q_m;
    };
} // namespace bamboo
#endif // BAMBOO_LOGTRACE_H
