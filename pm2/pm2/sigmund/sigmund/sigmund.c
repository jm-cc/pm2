
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
#include "tracelib.h"
#include "fkt.h"

extern char **traps;
extern char **sys_calls;

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

void error_usage()
{
  fprintf(stderr,"Usage: sigmund [options] [filters] action\n");
  exit(1);
}

int main(int argc, char **argv)
{
  trace tr;
  int argCount;
  char *trace_file_name = NULL;
  init();
  init_filter();
  for(argc--, argv++; argc > 0; argc -= argCount, argv +=argCount) {
    argCount = 1;
    if (!strcmp(*argv, "--trace-file")) {
      if (argc <= 1) error_usage();
      trace_file_name = *(argv + 1);
      argCount = 2;
    } else if (!strcmp(*argv, "--thread")) {
      if (argc <= 1) error_usage();
      filter_add_thread(atoi(*(argv + 1)));
      argCount = 2;
    } else if (!strcmp(*argv, "--process")) {
      if (argc <= 1) error_usage();
      filter_add_proc(atoi(*(argv + 1)));
      argCount = 2;
    } else if (!strcmp(*argv, "--cpu")) {
      if (argc <= 1) error_usage();
      filter_add_cpu(atoi(*(argv + 1)));
      argCount = 2;
    } else if (!strcmp(*argv, "--slice")) {
      if (argc <= 2) error_usage();
      filter_add_time_slice(atoi(*(argv + 1)), atoi(*(argv + 2)));
      argCount = 3;
    } else if (!strcmp(*argv, "--function")) {
      fprintf(stderr, "--function: unsupported yet\n");
      exit(1);
    } else if (!strcmp(*argv, "--event")) {
      int a;
      mode type;
      if (argc <= 1) error_usage();
      name2code(*(argv+1), &type, &a);
      filter_add_event(type, a);
      argCount = 2;
    } else if (!strcmp(*argv, "--event-slice")) {
      if (argc <= 2) error_usage();
      filter_add_evnum_slice(atoi(*(argv + 1)), atoi(*(argv + 2)));
      argCount = 3;
    } else if (!strcmp(*argv, "--begin")) {
      if (argc <= 3) error_usage();
      fprintf(stderr, "--begin --end: unsupported yet\n");
      exit(1);
      argCount = 4;
    } else error_usage();
  }
  tracelib_init(trace_file_name);
  for(;;) {
    if (get_next_filtered_trace(&tr) == 1) break;
    print_trace(tr);
  }
  return 0;
}
