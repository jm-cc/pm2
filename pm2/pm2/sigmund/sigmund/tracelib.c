#include "tracebuffer.h"
#include "filter.h"
#include "tracelib.h"
#include "temp.h"

char *fut_code2name(int code)
{
  int i = 0;
  while(1) {
    if (code_table[i].code == 0) return NULL;
    if (code_table[i].code == code) return code_table[i].name;
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
      *a = code_table[i].code;
      return 0;
    }
    i++;
  }
  i = 0;
  while(1) {
    if (fkt_code_table[i].code == 0) return -1;
    if (!strcmp(fkt_code_table[i].name,name)) {
      *type = KERNEL;
      *a = fkt_code_table[i].code;
      return 0;
    }
    i++;
  }
  return -1;
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

void tracelib_close()
{
  close_filter();
  close_trace_file();
}
