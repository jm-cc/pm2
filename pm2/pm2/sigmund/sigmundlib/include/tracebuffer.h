#ifndef TRACEBUFFER
#define TRACEBUFFER

#include "sigmund.h"

void init_trace_file(char *supertrace);

void init();

int get_next_trace(trace *tr);

void close_trace_file(void);

extern unsigned nb_cpu;
extern u_64 begin_str;
extern u_64 end_str;
extern double cpu_cycles;

#endif
