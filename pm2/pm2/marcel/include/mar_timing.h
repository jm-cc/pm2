
#ifndef MAR_TIMING_EST_DEF
#define MAR_TIMING_EST_DEF

#ifdef MAR_TIMING

#include "tbx.h"

#define TIMING_EVENT(name) TIME(name)

#else

#define TIMING_EVENT(name) /* nothing */

#endif

#endif
