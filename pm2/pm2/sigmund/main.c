
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
#include "tracebuffer.h"
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
  if (tr.type == USER)
    printf("%u ", tr.thread);
  printf("%9u ",(unsigned) tr.clock);
  printf("%4u %1u ",tr.pid, tr.proc);
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
    printf("%9x  ",tr.args[j]);
    i--;
    j++;
  }
  printf("\n");
}

int main()
{
  trace tr;
  int i;
  init_trace_buffer("prof_file_single", "trace_file");
  i = get_next_trace(&tr);
  print_trace(tr);
  while(i == 0) {
    i = get_next_trace(&tr);
    print_trace(tr);
  }
  return 0;
}

