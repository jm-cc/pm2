#ifndef TRACELIB_H
#define TRACELIB_H

#include "filter.h"
#include "tracebuffer.h"
#include "fkt.h"

#define TRUE 1
#define FALSE 0

extern filter options;

extern char *fut_code2name(int code);

extern int name2code(char *name, mode *type, int *code);

extern int sys2code(char *name, int *code);

extern int trap2code(char *name, int *code);

extern char *fkt_code2name(int code);

extern void tracelib_init(char *supertrace);

int get_next_loose_filtered_trace(trace *tr);

extern int get_next_filtered_trace(trace *tr);

extern void tracelib_close();

extern u_64 get_active_time();

extern u_64 get_idle_time();

extern int get_active_slices();

extern int get_idle_slices();

extern int get_function_time(int *code, mode *type, int *thread, u_64 *begin, u_64 *end, u_64 *time);

extern int is_a_valid_proc(int proc);

extern int is_a_valid_cpu(int logic);

extern int max_cpu();

extern u_64 get_begin_lwp(int lwp);

#endif
