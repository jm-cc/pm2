#include "tracebuffer.h"
#include "filter.h"
#include "tracelib.h"
#include "temp.h"
#include "fkt.h"
#include "fut_code.h"

struct time_st {
  u_64 active_time;
  int active_slices;
  u_64 idle_time;
  int idle_slices;
  u_64 last_time;
  int old_active;
  int first;
};

struct time_st timing;

char *fut_code2name(int code)
{
  int i = 0;
  while(1) {
    if (code_table[i].code == 0) return NULL;
    if (code_table[i].code == code) return code_table[i].name;
    i++;
  }
}

char *fkt_code2name(int code)
{
  int i = 0;
  while(1) {
    if (fkt_code_table[i].code == 0) return NULL;
    if (fkt_code_table[i].code == code) return fkt_code_table[i].name;
    i++;
  }
}

int name2code(char *name, mode *type, int *a)
{
  int i = 0;
  while(1) {
    if (code_table[i].code == 0) break;
    if (!strcmp(code_table[i].name,name)) {
      *type = USER;
      *a = code_table[i].code << 8;
      return 0;
    }
    i++;
  }
  i = 0;
  while(1) {
    if (fkt_code_table[i].code == 0) return -1;
    if (!strcmp(fkt_code_table[i].name,name)) {
      *type = KERNEL;
      *a = fkt_code_table[i].code << 8;
      return 0;
    }
    i++;
  }
  return -1;
}

int sys2code(char *name, int *code)
{
  int i;
  for(i = 0; i < NSYS_CALLS; i++)
    if (!strcmp(sys_calls[i], name)) break;
  if (i == NSYS_CALLS) return -1;
  *code = i;
  return 0;
}

int trap2code(char *name, int *code)
{
  int i;
  for(i = 0; i < NTRAPS; i++)
    if (!strcmp(traps[i], name)) break;
  if (i == NTRAPS) return -1;
  *code = i + FKT_SYS_CALL_LIMIT_CODE;
  return 0;
}

void tracelib_init(char *supertrace)
{
  init_trace_file(supertrace);
}

int is_valid_calc(trace *tr, int eof)
{
  int v = is_valid(tr);
  if (timing.first == TRUE) {
    if (options.active != 0) {
      timing.first = FALSE;
      timing.last_time = tr->clock;
      timing.old_active = TRUE;
    }
  } else {
    if ((timing.old_active == TRUE) && (options.active == 0)) {
      timing.active_time += tr->clock - timing.last_time;
      timing.active_slices++;
      timing.last_time = tr->clock;
      timing.old_active = FALSE;
    } else if ((timing.old_active == FALSE) && (options.active != 0)) {
      timing.idle_time += tr->clock - timing.last_time;
      timing.idle_slices++;
      timing.last_time = tr->clock;
      timing.old_active = TRUE;
    }
  }
  if ((eof != 0) && (timing.old_active == TRUE)) {
    timing.active_time += tr->clock - timing.last_time;
    timing.active_slices++;
  }
  return v;
}

// O if not eof, 1 if eof and not valid 2 if eof and valid
int get_next_filtered_trace(trace *tr)
{
  int eof = 0;
  while (eof == 0) {
    eof = get_next_trace(tr);
    if (is_valid_calc(tr, eof) == TRUE) {
      if (eof != 0) eof = 2;
      break;
    }
  }
  return eof;
}

int get_next_loose_filtered_trace(trace *tr)
{
  int i = 0;
  while (i == 0) {
    i = get_next_trace(tr);
    if (tr->type == KERNEL) {
      if (tr->code >> 8 == FKT_SWITCH_TO_CODE) break;
    } else if (tr->code >> 8 == FUT_SWITCH_TO_CODE) break;
    if (is_valid_calc(tr, i) == TRUE) break;
  }
  return i;
}

void tracelib_close()
{
  close_filter();
  close_trace_file();
}

u_64 get_active_time()
{
  return timing.active_time;
}

u_64 get_idle_time()
{
  return timing.idle_time;
}

int get_active_slices()
{
  return timing.active_slices;
}

int get_idle_slices()
{
  return timing.idle_slices;
}

