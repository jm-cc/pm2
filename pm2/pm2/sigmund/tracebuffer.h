#ifndef TRACEBUFFER
#define TRACEBUFFER

#include "sigmund.h"

#define TRACE_BUFFER_SIZE 1000
#define EMPTY_LIST (trace_list) NULL

typedef struct trace_item_st * trace_list;

struct trace_item_st {
  trace tr;
  trace_list next;
  trace_list prev;
};

typedef struct {
  trace_list first;
  trace_list last;
} trace_buffer;

void init_trace_buffer(char *futin, char *fktin);

int get_next_trace(trace *tr);

void close_trace_buffer(void);

#endif
