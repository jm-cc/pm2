
#ifndef TRACE_IS_DEF
#define TRACE_IS_DEF

#include <stdio.h>

#ifdef USE_TRACE

#define TRACE(fmt, arg...) \
  fprintf(stderr, fmt, ##arg)

#else

#define TRACE(fmt, arg...) \
  (void)0

#endif // end ifdef USE_TRACE

#endif
