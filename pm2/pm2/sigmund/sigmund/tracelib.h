#ifndef TRACELIB_H
#define TRACELIB_H

#include "filter.h"
#include "tracebuffer.h"

#define TRUE 1
#define FALSE 0

extern filter options;

extern int tracelib_init(char *supertrace);

extern int get_next_filtered_trace(trace *tr);

extern void tracelib_close();

#endif
