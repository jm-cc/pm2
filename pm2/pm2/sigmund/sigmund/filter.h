#ifndef FILTER_H
#define FILTER_H

#include "sigmund.h"
#include <stdlib.h>

#define THREAD_LIST_NULL (thread_list) NULL
#define PROC_LIST_NULL (proc_list) NULL
#define CPU_LIST_NULL (cpu_list) NULL
#define EVENT_LIST_NULL (event_list) NULL
#define TIME_SLICE_LIST_NULL (time_slice_list) NULL
#define EVNUM_SLICE_LIST_NULL (evnum_slice_list) NULL
#define GENERAL_SLICE_LIST_NULL (general_slice_list) NULL

typedef struct {
  u_64 begin;
  u_64 end;
} time_slice;

typedef struct {
  unsigned int begin;
  unsigned int end;
} evnum_slice;

typedef struct {
  mode begin_type;
  int begin;
  int begin_param_active;
  char begin_param;
  mode end_type;
  int end;
  char end_param_active;
  int end_param;
} general_slice;


typedef struct thread_list_st {
  int thread;
  struct thread_list_st *next;
} * thread_list;

typedef struct proc_list_st {
  int proc;
  struct proc_list_st *next;
} * proc_list;

typedef struct cpu_list_st {
  short int cpu;
  struct cpu_list_st *next;
} * cpu_list;

typedef struct event_list_st {
  mode type;
  int code;
  struct event_list_st *next;
} * event_list;

typedef struct time_slice_list_st {
  time_slice t;
  struct time_slice_list_st *next;
} * time_slice_list;

typedef struct evnum_slice_list_st {
  evnum_slice t;
  struct evnum_slice_list_st *next;
} * evnum_slice_list;

typedef struct general_slice_list_st {
  general_slice t;
  struct general_slice_list_st *next;
} * general_slice_list;

typedef struct {
  thread_list thread;
  proc_list proc;
  cpu_list cpu;
  event_list event;
  time_slice_list time;
  evnum_slice_list evnum_slice;
  general_slice_list gen_slice;
  int active;               
} filter;

extern void init_filter();

extern void close_filter();

extern void filter_add_thread(int thread);

extern void filter_add_proc(int proc);

extern void filter_add_cpu(short int cpu);

extern void filter_add_event(mode type, int code);

extern void filter_add_time_slice(u_64 begin, u_64 end);

extern void filter_add_evnum_slice(unsigned int begin, unsigned int end);

extern void filter_add_gen_slice(mode begin_type, int begin, 
				 char begin_param_active, int begin_param, 
				 mode end_type, int end, 
				 char end_param_active, int end_param);

int is_valid(trace *tr);

#endif
