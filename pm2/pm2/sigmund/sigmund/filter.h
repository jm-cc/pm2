#ifndef FILTER_H
#define FILTER_H

#include "sigmund.h"
#include <stdlib.h>

#define THREAD_LIST_NULL (thread_list) NULL
#define PROC_LIST_NULL (proc_list) NULL
#define LOGIC_LIST_NULL (logic_list) NULL
#define CPU_LIST_NULL (cpu_list) NULL
#define EVENT_LIST_NULL (event_list) NULL
#define TIME_SLICE_LIST_NULL (time_slice_list) NULL
#define EVNUM_SLICE_LIST_NULL (evnum_slice_list) NULL
#define GENERAL_SLICE_LIST_NULL (general_slice_list) NULL
#define LWP_THREAD_LIST_NULL (lwp_thread_list) NULL
#define THREAD_FUN_LIST_NULL (thread_fun_list) NULL
#define FUNCTION_TIME_LIST_NULL (function_time_list) NULL

typedef struct thread_list_st {
  int thread;
  struct thread_list_st *next;
} * thread_list;

typedef struct proc_list_st {
  int proc;
  struct proc_list_st *next;
} * proc_list;

typedef struct logic_list_st {
  int logic;
  struct logic_list_st *next;
} * logic_list;

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
  u_64 begin;
  u_64 end;
  struct time_slice_list_st *next;
} * time_slice_list;

typedef struct evnum_slice_list_st {
  unsigned int begin;
  unsigned int end;
  struct evnum_slice_list_st *next;
} * evnum_slice_list;

typedef struct function_time_list_st {
  mode type;
  int code;
  int thread;
  u_64 begin;
  u_64 end;
  u_64 time;
  struct function_time_list_st *next;
} * function_time_list;

typedef struct general_slice_list_st {
  mode begin_type;
  int begin;
  int begin_param_active;
  char begin_param;
  mode end_type;
  int end;
  char end_param_active;
  int end_param;
  struct general_slice_list_st *next;
} * general_slice_list;

typedef struct thread_fun_list_st {
  int thread;
  int number;
  struct thread_fun_list_st *next;
} * thread_fun_list;


typedef struct lwp_thread_list_st {
  int lwp;
  int logic;
  int thread;
  int active;
  int active_thread;
  struct lwp_thread_list_st *next;
} * lwp_thread_list;

typedef struct {
  thread_list thread;
  int active_thread;
  proc_list proc;
  logic_list logic;
  int active_proc;
  cpu_list cpu;
  lwp_thread_list lwp_thread;
  event_list event;
  time_slice_list time;
  int active_time;
  evnum_slice_list evnum_slice;
  int active_evnum_slice;
  general_slice_list function;
  thread_fun_list thread_fun;
  function_time_list function_begin;
  function_time_list function_time;
  int active_thread_fun;
  general_slice_list gen_slice;
  int active_gen_slice;
  int active;
} filter;

extern void init_filter();

extern void close_filter();

extern void filter_add_thread(int thread);

extern void filter_add_proc(int proc);

extern void filter_add_logic(int logic);

extern void filter_add_cpu(short int cpu);

extern void filter_add_event(mode type, int code);

extern void filter_add_time_slice(u_64 begin, u_64 end);

extern void filter_add_evnum_slice(unsigned int begin, unsigned int end);

extern void filter_add_gen_slice(mode begin_type, int begin, 
				 char begin_param_active, int begin_param, 
				 mode end_type, int end, 
				 char end_param_active, int end_param);

extern void filter_add_function(mode begin_type, int begin, 
				 char begin_param_active, int begin_param, 
				 mode end_type, int end, 
				 char end_param_active, int end_param);

extern int filter_get_function_time(struct function_time_list_st *fct_time);

int is_valid(trace *tr);

#endif
