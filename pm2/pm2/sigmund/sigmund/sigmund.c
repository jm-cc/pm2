
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <stdio.h>
#include "sigmund.h"
#include "parser.h"
#include <stdlib.h>
#include "time.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "temp.h"
#include "tracelib.h"

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

void print_trace(trace tr)
{
  int i, j = 0;
  i = tr.code & 0xff;
  i = (i - 12) / 4;
  printf("%s",(tr.type == USER)? "USER: " : "KERN: ");
  printf("%9u ",(unsigned) tr.clock);
  if (tr.type != USER)
    printf("%4u %1u ", tr.pid, tr.proc);
  else
    printf("%1u    %1u ", tr.proc, tr.thread);
  printf("%6x",tr.code);
  if (tr.type == USER) {
    printf("%40s", fut_code2name(tr.code >> 8));
  } else {
    if (tr.code >= FKT_UNSHIFTED_LIMIT_CODE)
      printf("%40s", fkt_code2name(tr.code >> 8));
    else {
      if (tr.code < FKT_SYS_CALL_LIMIT_CODE) {
	printf("\t\t\t\t\t  system call    %u", tr.code);
	printf("   %s", sys_calls[tr.code]);
      }
      else if (tr.code < FKT_TRAP_LIMIT_CODE) {
	i = tr.code - FKT_SYS_CALL_LIMIT_CODE;
	printf("\t\t\t\t\t  trap    %u", i);
	printf("   %s", traps[i]);
      }
      else {
	i = tr.code -  FKT_TRAP_LIMIT_CODE;
	printf("\t\t\t\t\t  IRQ    %u", i);
      }
      i = 0;
    }
  }
  while(i != 0) {
    printf("%14u  ",tr.args[j]);
    i--;
    j++;
  }
  printf("\n");
}

int main()
{
  trace tr;
  tracelib_init("prof_file");
  filter_add_time_slice(1630702193,1632249253);
  for(;;) {
    if (get_next_filtered_trace(&tr) == 1) break;
    print_trace(tr);
  }
  return 0;
}
