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

#endif
