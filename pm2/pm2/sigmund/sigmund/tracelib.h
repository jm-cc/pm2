#ifndef TRACELIB_H
#define TRACELIB_H

#include "filter.h"
#include "tracebuffer.h"

#define TRUE 1
#define FALSE 0

extern filter options;

extern char *fut_code2name(int code);

extern int name2code(char *name, mode *type, int *a);

extern char *fkt_code2name(int code);

extern void tracelib_init(char *supertrace);

extern int get_next_filtered_trace(trace *tr);

extern void tracelib_close();

#endif
