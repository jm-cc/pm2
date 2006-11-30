/* copyleft */

#ifndef PFA_TIMER_H
#define PFA_TIMER_H 1

#include <time.h>

/* putain de fichu c89 */
#ifndef __USE_ISOC99
#ifndef true
typedef unsigned char bool;
#define true 1
#define false 0
#endif
#else
#include <stdbool.h>
#endif

#ifndef WIN32
#include <sys/time.h>
#endif

#ifdef WIN32
#include <sys/timeb.h>
int gettimeofday (struct timeval *tp, void *tz);
#endif

typedef struct Chrono_tag
{
      struct timeval tv1;
      struct timeval tv2;
      bool started;
      int back;
} Chrono;

void chrono_init(Chrono* c);
void chrono_reset(Chrono* c);
void chrono_start(Chrono* c);
void chrono_pause(Chrono* c);
void chrono_back(Chrono* c);
void chrono_set(Chrono* c, int sec);
float chrono_stop(Chrono* c);   /* retourne le nombre de secondes depuis le start et arrete le décompte */
float chrono_read(Chrono* c);   /* retourne le nombre de secondes depuis le start */

#endif
