#include "tracebuffer.h"
#include "tracelib.h"

int tracelib_init(char *supertrace)
{
  init_trace_file(supertrace);
  init_filter();
}

int get_next_filtered_trace(trace *tr)
{
  int i = 0;
  while (i == 0) {
    i = get_next_trace(tr);
    if (is_valid(tr) == TRUE) break;
  }
  return i;
}

void tracelib_close()
{
  close_filter();
  close_trace_file();
}
