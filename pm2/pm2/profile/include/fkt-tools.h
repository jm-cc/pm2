#ifndef __FKT_TOOLS_H__
#define __FKT_TOOLS_H__
#include <sys/types.h>
#include <sys/times.h>

#define EPRINTF(fmt,...)	do {\
	fprintf(stderr, "=== " fmt, ## __VA_ARGS__); \
	printf("=== " fmt, ## __VA_ARGS__); \
	fflush(stdout); \
} while (0)

#ifndef FUT
#define DEFAULT_TRACE_FILE "trace_file"
#else
#define DEFAULT_TRACE_FILE "fut_trace_file"
#endif
extern void dumptime(time_t *the_time, clock_t *the_jiffies);
#endif /* __FKT_TOOLS_H__ */
