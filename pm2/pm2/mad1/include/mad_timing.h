
#ifndef MAD_TIMING_EST_DEF
#define MAD_TIMING_EST_DEF

#ifdef MAD_TIMING

#include "tbx.h"

#define TIMING_EVENT(name)   TIME(name)

#else

#define TIMING_EVENT(name)   /* nothing */

#endif

#endif
