/* copyleft */

#include "timer.h"

#ifdef WIN32
int gettimeofday (struct timeval *tp, void *tz)
{
   struct _timeb timebuffer;
   _ftime(&timebuffer);
   tp->tv_sec = timebuffer.time;
   tp->tv_usec = timebuffer.millitm * 1000;
   return 0;
}
#endif

void chrono_init(Chrono* c)
{
   c->started = false;
}

void chrono_reset(Chrono* c)
{
   gettimeofday(&c->tv1, NULL);
   c->tv2 = c->tv1;
   c->back = 1;
}

void chrono_start(Chrono* c)
{
   struct timeval tv;
   if (c->back != 1) {
      c->tv1.tv_sec = 2 * c->tv2.tv_sec - c->tv1.tv_sec;
      c->tv1.tv_usec = 2 * c->tv2.tv_usec - c->tv1.tv_usec;
      c->back = 1;
   }
   gettimeofday(&tv, NULL);
   c->tv1.tv_sec += tv.tv_sec - c->tv2.tv_sec;
   c->tv1.tv_usec += tv.tv_usec - c->tv2.tv_usec;
   c->started = true;
}

float chrono_stop(Chrono* c)
{
   chrono_reset(c);
   c->started = false;
   return chrono_read(c);
}

void chrono_pause(Chrono* c)
{
   if (c->started)
      gettimeofday(&c->tv2, NULL);
   c->started = false;
}

void chrono_set(Chrono* c, int sec)
{
   c->tv1.tv_sec = c->tv2.tv_sec - sec;
   c->tv1.tv_usec = c->tv2.tv_usec;
}

void chrono_back(Chrono* c)
{
   struct timeval tv;
   if (c->back != -1) {
      c->tv1.tv_sec = 2 * c->tv2.tv_sec - c->tv1.tv_sec;
      c->tv1.tv_usec = 2 * c->tv2.tv_usec - c->tv1.tv_usec;
      c->back = -1;
   }
   gettimeofday(&tv, NULL);
   c->tv1.tv_sec += tv.tv_sec - c->tv2.tv_sec;
   c->tv1.tv_usec += tv.tv_usec - c->tv2.tv_usec;
   c->started = true;
}

float chrono_read(Chrono* c)
{
   if (c->started)
      gettimeofday(&c->tv2, NULL);
   return c->back * (c->tv2.tv_sec - c->tv1.tv_sec + (c->tv2.tv_usec * 1e-6f - c->tv1.tv_usec * 1e-6f));
}
