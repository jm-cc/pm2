
#ifndef PM2_TIMING_EST_DEF
#define PM2_TIMING_EST_DEF

#ifdef PM2_TIMING

#include "tbx.h"

#define TIMING_EVENT(name)   TIME(name)

#else

#define TIMING_EVENT(name)   /* nothing */

#endif

#endif
