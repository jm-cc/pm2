

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
#include <stdlib.h>
#include <unistd.h>
#include "sigmund.h"
#include "parser.h"
#include "tracelib.h"
#include "string.h"
#include "graphlib.h"
#include "lwpthread.h"

#include "fut.h"

#define HSIZE 1000
#define VSIZE 1000

char *ps_output_name = NULL;

enum action {NONE, PRINT_PROCESS, PRINT_THREAD};

static u_64 begin_global = 0;
static u_64 end_global = 0;
static short int char_size = 6;
static int size_of_dec = 20;


float size_of_tick;

int h_coord_cpu(short int cpu)
{
  return (cpu*HSIZE/max_cpu());
}

float v_coord(u_64 time)
{
  return (float) VSIZE - (((unsigned) time - (float)begin_global)/((float)end_global-(float)begin_global))*(float)VSIZE;
}

void init_lwp_ps(FILE *f)
{
  fprintf(f, "%%!PS-Adobe-1.0 EPSF-1.0\n%%%%BoundingBox: -20 0 %d %d\n", HSIZE, VSIZE);
  fprintf(f, "(Helvetica) findfont %d scalefont setfont\n", char_size);
  fprintf(f, "/red {1.000 0.000 0.000 setrgbcolor} def\n");
  fprintf(f, "/green {0.000 1.000 0.000 setrgbcolor} def\n"); 
  fprintf(f, "/blue {1.000 0.000 1.000 setrgbcolor} def\n");
  fprintf(f, "/black {0.000 0.000 0.000 setrgbcolor} def\n");
  fprintf(f, "/cote 20 def\n");
  fprintf(f, "/rectangle {\n");
  fprintf(f, "/name exch def\n");
  fprintf(f, "/long exch def\n");
  fprintf(f, "newpath\n");
  fprintf(f, "moveto\n");
  fprintf(f, "green\n");
  fprintf(f, "cote 0 rlineto\n");
  fprintf(f, "0 long rlineto\n");
  fprintf(f, "cote neg 0 rlineto\n");
  fprintf(f, "closepath\n");
  fprintf(f, "1 long 2 div %f sub rmoveto\n", (float) char_size / (float) 2);
  fprintf(f, "name show\n");
  fprintf(f, "stroke\n");
  fprintf(f, "} def\n");
}

void draw_rectangle(u_64 begin, u_64 end, int pid, short int cpu, FILE *f)
{
  //  printf("%u %u\n", (unsigned) begin, (unsigned) end);
  fprintf(f, "%d %f %f (%d) rectangle\n", h_coord_cpu(cpu), v_coord(begin), v_coord(end) - v_coord(begin), pid);
}

void close_lwp_ps(FILE *f)
{
  int i;
  for (i = 0; i < max_cpu(); i++) {
    u_64 begin = get_begin_pid(pid_of_cpu(i));
    if (begin != -1)
      draw_rectangle(begin,end_global,pid_of_cpu(i),i,f);
  }
}

void print_process()
{
  FILE **F_proc;
  FILE *F_out;
  int i;
  char s[200];
  char num[3];
  trace tr;
  int eof = 0;
  float last_trace[NB_PROC];
  printf("%d cpu\n", max_cpu());
  for(i = 0; i < max_cpu(); i++) 
    last_trace[i] = (float) VSIZE;
  F_proc = (FILE **) malloc (max_cpu() * sizeof(FILE *));
  if ((F_out = fopen(ps_output_name,"w")) == NULL) {
    fprintf(stderr,"Couldn't open file %s\n", ps_output_name);
    exit(1);
  }
  for(i = 0; i < max_cpu(); i++) {
    strcpy(s, "/tmp/proc_proc");
    sprintf(num,"%d",i);
    strcat(s, num);
    if ((F_proc[i] = fopen(s,"w")) == NULL) {
      fprintf(stderr,"Couldn't open file %s\n", s);
      exit(1);
    }
    fprintf(F_proc[i], "newpath\n");
    fprintf(F_proc[i], "black\n");
    fprintf(F_proc[i], "%d %d moveto\n", h_coord_cpu(i) + 10, VSIZE);
  }
  init_lwp_ps(F_out);
  while ((eof == 0) || (eof == 3)){
    eof = get_next_loose_filtered_trace(&tr);
    if ((tr.type == KERNEL) && (tr.code >> 8 == FKT_SWITCH_TO_CODE)) {
      int new_thread;
      u_64 begin = get_begin_pid(tr.pid);
      draw_rectangle(begin, tr.clock, tr.pid, tr.cpu, F_out);
      if (tr.thread != -1) {
	if (get_thread_disactivated(tr.thread) == FALSE) {
	  fprintf(F_proc[tr.cpu], "0 %f rlineto\n", v_coord(tr.clock) - last_trace[tr.cpu]);
	}
      }
      fprintf(F_proc[tr.cpu], "stroke %% %d %d\n", tr.pid, tr.args[0]);
      new_thread = thread_of_lwp(tr.args[0]);
      if (new_thread != -1) {
	fprintf(F_proc[tr.cpu], "%d %f moveto\n", 
		h_coord_cpu(tr.cpu) + 10 + get_thread_dec(new_thread),
		v_coord(tr.clock));
	fprintf(F_proc[tr.cpu], "%s\n", (get_last_type_thread(new_thread) == USER)? "black" : "red");
      }
      last_trace[tr.cpu] = v_coord(tr.clock);
      continue;
    }
    if ((tr.type == USER) && (tr.code >> 8 == FUT_SWITCH_TO_CODE)) {
      if (get_thread_disactivated(tr.thread) == FALSE) {
	fprintf(F_proc[tr.cpu], "0 %f rlineto\n", v_coord(tr.clock) - last_trace[tr.cpu]);
      }
      fprintf(F_proc[tr.cpu], "stroke\nblue\n%d %f moveto\n(%d) show\nstroke\n",
	      h_coord_cpu(tr.cpu) - 20, v_coord(tr.clock), tr.args[1]);
      
      fprintf(F_proc[tr.cpu], "%d %f moveto\n",
	      h_coord_cpu(tr.cpu) + 10 + get_thread_dec(tr.args[1]) * size_of_dec,
	      v_coord(tr.clock));
      fprintf(F_proc[tr.cpu], "%s\n", (get_last_type_thread(tr.thread) == USER)? "black" : "red");
      last_trace[tr.cpu] = v_coord(tr.clock);
      continue;
    }
    if ((tr.type == USER) && (tr.code >> 8 == FUT_NEW_LWP_CODE)) {
      /*short int cpu_t;
            if (get_thread_disactivated(tr.thread) == FALSE) {
	fprintf(F_proc[tr.cpu], "0 %f rlineto\n", v_coord(tr.clock) - last_trace[tr.cpu]);
	} */

      /*      if (
      cpu_t = 
      fprintf(F_proc[tr.cpu], "%d %f moveto\n",
	      h_coord_cpu(tr.cpu) + 10 + get_thread_dec(tr.args[1]) * size_of_dec,
	      v_coord(tr.clock));
      fprintf(F_proc[tr.cpu], "%s\n", (get_last_type_thread(tr.thread) == USER)? "black" : "red");
      last_trace[tr.cpu] = v_coord(tr.clock);*/
      continue;
    }
    if ((tr.type == USER) && (tr.code >> 8 == FUT_THREAD_BIRTH_CODE)) {
      continue;
    }
    if (get_thread_disactivated(tr.thread) != FALSE) {
      set_thread_disactivated(tr.thread, FALSE);
      last_trace[tr.cpu] = v_coord(tr.clock);
      fprintf(F_proc[tr.cpu], "stroke\n%d %f moveto\n",
	      h_coord_cpu(tr.cpu) + 10 + get_thread_dec(tr.thread) * size_of_dec,
	      last_trace[tr.cpu]);
      fprintf(F_proc[tr.cpu], "%s\n", (get_last_type_thread(tr.thread) == USER)? "black" : "red");
    } else fprintf(F_proc[tr.cpu], "0 %f rlineto\n", v_coord(tr.clock) - last_trace[tr.cpu]);
    if (tr.type == KERNEL) {
      if (get_last_type_thread(tr.thread) == USER) {
	fprintf(F_proc[tr.cpu], "stroke\nred\n");
	fprintf(F_proc[tr.cpu], "%d %f moveto\n",h_coord_cpu(tr.cpu) + 10 +get_thread_dec(tr.thread) * size_of_dec,
		v_coord(tr.clock));
	set_last_type_thread(tr.thread, KERNEL);
      }
    } else {
      if (get_last_type_thread(tr.thread) == KERNEL) {
	fprintf(F_proc[tr.cpu], "stroke\nblack\n");
	fprintf(F_proc[tr.cpu], "%d %f moveto\n",h_coord_cpu(tr.cpu) + 10+get_thread_dec(tr.thread) * size_of_dec,
		v_coord(tr.clock));
	set_last_type_thread(tr.thread, USER);
      }
      if (tr.code < 0x40000) {
	fprintf(F_proc[tr.cpu], "%d 0 rlineto\n", size_of_dec);
	fprintf(F_proc[tr.cpu], "%d 0 rlineto\n", -size_of_dec);
      } else {
	if ((tr.code >> 8) >= 0x8000) {
	  fprintf(F_proc[tr.cpu], "%d 0 rlineto\n", size_of_dec);
	  fprintf(F_proc[tr.cpu], "%d 0 rlineto\n", -size_of_dec);
	} else {
	  if (((tr.code >> 8) & 0x100) == 0) 
	    fprintf(F_proc[tr.cpu], "%d 0 rlineto\n", size_of_dec);
	  else 
	    fprintf(F_proc[tr.cpu], "%d 0 rlineto\n", -size_of_dec);
	}
      }
    }
    last_trace[tr.cpu] = v_coord(tr.clock);
  }

  for(i = 0; i < max_cpu(); i++) {
    char c;
    rewind(F_proc[i]);
    while (fread(&c, sizeof(char), 1, F_proc[i]) == 1)
      fwrite(&c, sizeof(char), 1, F_out);
    fprintf(F_out, "stroke\n");
    strcpy(s, "/tmp/proc_proc");
    sprintf(num,"%d",i);
    strcat(s, num);
    unlink(s);
  }
  close_lwp_ps(F_out);
  fclose(F_out);
}

void print_thread()
{

}

void error_usage()
{
  fprintf(stderr,"Usage: sigmund [options] [filters] action\n");
  fprintf(stderr,"  action: --print-process or --print-thread\n");

  exit(1);
}

int main(int argc, char **argv)
{
  int argCount;
  int nb_time = 0;
  char *trace_file_name = NULL;
  enum action ac;
  ac = NONE;
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
    } else if (!strcmp(*argv, "-o")) {
      if (argc <= 1) error_usage();
      if (ps_output_name != NULL) error_usage();
      ps_output_name = *(argv + 1);
      argCount = 2;
    } else if (!strcmp(*argv, "--process")) {
      if (argc <= 1) error_usage();
      filter_add_proc(atoi(*(argv + 1)));
      argCount = 2;
    } else if (!strcmp(*argv, "--logic")) {
      if (argc <= 1) error_usage();
      filter_add_logic(atoi(*(argv + 1)));
      argCount = 2;
    } else if (!strcmp(*argv, "--cpu")) {
      if (argc <= 1) error_usage();
      filter_add_cpu(atoi(*(argv + 1)));
      argCount = 2;
    } else if (!strcmp(*argv, "--time-slice")) {
      if (argc <= 2) error_usage();
      filter_add_time_slice(atoi(*(argv + 1)), atoi(*(argv + 2)));
      if (nb_time == 0) {
	begin_global = atoi(*(argv + 1));
	end_global = atoi(*(argv + 2));
	nb_time = 1;
      }
      argCount = 3;
    } else if (!strcmp(*argv, "--function")) {
      int begin;
      mode begin_type;
      int end;
      mode end_type;
      char s[400];
      if (argc <= 1) error_usage();
      strcpy(s,*(argv+1));
      strcat(s,"_entry");
      if (name2code(s, &begin_type, &begin) != 0) error_usage();
      strcpy(s,*(argv+1));
      strcat(s,"_exit");
      if (name2code(s, &end_type, &end) != 0) error_usage();
      filter_add_function(begin_type, begin, FALSE, 0, end_type, end, FALSE, 0);
      argCount = 2;
    } else if (!strcmp(*argv, "--event")) {
      int a;
      mode type;
      if (argc <= 1) error_usage();
      if (name2code(*(argv+1), &type, &a) != 0) error_usage();
      filter_add_event(type, a);
      argCount = 2;
    } else if (!strcmp(*argv, "--sys-call")) {
      int a;
      if (argc <= 1) error_usage();
      if (sys2code(*(argv+1), &a) != 0) error_usage();
      filter_add_event(KERNEL, a);
      argCount = 2;
    } else if (!strcmp(*argv, "--event-slice")) {
      if (argc <= 2) error_usage();
      filter_add_evnum_slice(atoi(*(argv + 1)), atoi(*(argv + 2)));
      argCount = 3;
    } else if (!strcmp(*argv, "--begin")) {
      int begin_param_active = 0;
      int end_param_active = 0;
      mode begin_type = 0;
      mode end_type = 0;
      int begin_param = 0;
      int end_param = 0;
      int begin = 0;
      int end = 0;
      if (argc <= 3) error_usage();
      if (strcmp(*(argv+2),"--end")) error_usage();
      if (strchr(*(argv+1),':') == NULL) {
	begin_param_active = FALSE;
	if (name2code(*(argv+1), &begin_type, &begin) != 0) error_usage();
      }
      else {
	char *s;
	begin_param_active = TRUE;
	s = strtok(*(argv+1),":");
	if (name2code(s, &begin_type, &begin) != 0) error_usage();
	s = strtok(NULL,":");
	begin_param = atoi(s);
      }
      if (strchr(*(argv+3),':') == NULL) {
	end_param_active = FALSE;
	if (name2code(*(argv+3), &end_type, &end) != 0) error_usage();
      }
      else {
	char *s;
	end_param_active = TRUE;
	s = strtok(*(argv+3),":");
	if (name2code(s, &end_type, &end) != 0) error_usage();
	s = strtok(NULL, ":");
	end_param = atoi(s);
      }
      filter_add_gen_slice(begin_type, begin, begin_param_active, begin_param,
			   end_type, end, end_param_active, end_param);
      argCount = 4;
    } else if (!strcmp(*argv, "--print-process")) {
      if (ac != NONE) error_usage();
      ac = PRINT_PROCESS;
    } else if (!strcmp(*argv, "--print-thread")) {
      if (ac != NONE) error_usage();
      ac = PRINT_THREAD;
    } else error_usage();
  }
  tracelib_init(trace_file_name);
  if (nb_time == 0) {
    begin_global = get_begin_str();
    end_global = get_end_str();
  }
  if (ps_output_name == NULL) ps_output_name = "sigmundps_output.eps";
  switch(ac) {
  case NONE : {
    error_usage();
    break;
  }
  case PRINT_PROCESS : {
    print_process();
    break;
  }
  case PRINT_THREAD : {
    print_thread();
    break;
  }
  default: {
    fprintf(stderr,"Please report bug to cmenier@ens-lyon.fr\n");
    exit(1);
  }
  }
  return 0;
}
