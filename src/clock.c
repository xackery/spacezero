#include "clock.h"

double Clock(int id, int action) {
  /*
     ten clocks
  */
  static struct Clock clock[10];
  struct timeval now;

  if (id > 9)
    return (0);
  if (id < 0)
    return (0);

  switch (action) {
    case CL_CLEAR:
      clock[id].time = 0;
      break;
    case CL_START:
      gettimeofday(&clock[id].starttime, NULL);
      break;
    case CL_STOP:
      gettimeofday(&now, NULL);
      decr_timeval(&now, &clock[id].starttime);
      clock[id].time += (double)(now.tv_sec + now.tv_usec / 1.0e6);
      break;
    case CL_READ:
      return (clock[id].time);
      break;
    default:
      return (0);
      break;
  }
  return (0);
}
