#ifndef __PM2_FXT_TOOLS_H__
#define __PM2_FXT_TOOLS_H__
#include "fxt/fxt.h"
#undef DEFAULT_TRACE_FILE
#ifndef FUT
#define DEFAULT_TRACE_FILE "trace_file"
#else
#define DEFAULT_TRACE_FILE "fut_trace_file"
#endif
extern void dumptime(time_t *the_time, clock_t *the_jiffies);
extern fxt_t fut;
#endif /* __PM2_FXT_TOOLS_H__ */
