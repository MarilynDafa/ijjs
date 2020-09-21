#pragma once
#include <time.h>
#include "../../headers/ijpre.h"
#if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)

typedef intptr_t ssize_t;
# define SSIZE_MAX INTPTR_MAX
# define _SSIZE_T_
# define _SSIZE_T_DEFINED
#endif
struct timezone {
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};
#include <WinSock2.h>


typedef unsigned char clockid_t;
void     InitTimeFunctions();
IJ_API int      gettimeofday(struct timeval* tv, struct timezone* tz);
IJ_API int clock_getres(clockid_t clock_id, struct timespec* res);
IJ_API int clock_gettime(clockid_t clock_id, struct timespec* tp);