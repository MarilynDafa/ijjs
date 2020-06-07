#ifdef _MSC_VER

#include <windows.h>
#include <time.h>
#include <stdio.h>

struct tm *localtime_r(long *clock,struct tm *res)
{
    _localtime32_s(res,clock);
    return(res);
}

int gettimeofday(struct timeval *tp,struct timezone *tz)
{
SYSTEMTIME st;

    if (tp!=NULL) {
	tp->tv_sec = _time32(NULL);
	GetLocalTime(&st);
	tp->tv_usec = 1000L * st.wMilliseconds;
    } /* if */

    return(0);
}

int fsync(FILE *fp) {
  return(fflush(fp));
}

#endif
