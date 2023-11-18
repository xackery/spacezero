#ifndef _CLOCK_
#define _CLOCK_

#include <stdio.h>
#include <sys/time.h>

#define CL_CLEAR 0
#define CL_START 1
#define CL_STOP 2
#define CL_SUM 3
#define CL_READ 4

#define decr_timeval(t1, t2)                                   \
  do {                                                         \
    if ((t1)->tv_usec < (t2)->tv_usec) {                       \
      (t1)->tv_sec -= (t2)->tv_sec + 1;                        \
      (t1)->tv_usec = (t1)->tv_usec + 1000000 - (t2)->tv_usec; \
    } else {                                                   \
      (t1)->tv_sec -= (t2)->tv_sec;                            \
      (t1)->tv_usec -= (t2)->tv_usec;                          \
    }                                                          \
  } while (0)

struct Clock {
  double time;
  struct timeval starttime;
};

double Clock(int id, int action);

#endif
