#include "tracebuffer.h"
#include "filter.h"
#include "tracelib.h"
#include "temp.h"
#include "fkt.h"
#include "fut_code.h"

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

int get_next_filtered_trace(trace *tr)
{
  int i = 0;
  while (i == 0) {
    i = get_next_trace(tr);
    if (is_valid(tr) == TRUE) break;
  }
  return i;
}

int get_next_loose_filtered_trace(trace *tr)
{
  int i = 0;
  while (i == 0) {
    i = get_next_trace(tr);
    if (tr->type == KERNEL) {
      if (tr->code >> 8 == FKT_SWITCH_TO_CODE) break;
    } else if (tr->code >> 8 == FUT_SWITCH_TO_CODE) break;
    if (is_valid(tr) == TRUE) break;
  }
  return i;
}

void tracelib_close()
{
  close_filter();
  close_trace_file();
}
