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
  unsigned code;
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
  unsigned code;
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
  u_64 last_up_lwp;
  int logic;
  int thread;
  short int cpu;
  int active;
  int active_thread;
  struct lwp_thread_list_st *next;
} * lwp_thread_list;

typedef struct {
  thread_list thread;             // List of wanted threads
  int active_thread;              // Number of thread active at a certain time
  proc_list proc;                 // List of wanted processus
  logic_list logic;               // List of wanted logic lwp
  int active_proc;                // Number of active process at a certain time
  cpu_list cpu;                   // List of wanted cpu
  event_list event;               // List of wanted events
  time_slice_list time;           // List of wanted time slices
  int active_time;                // Indicates if we are in an active time zone
  evnum_slice_list evnum_slice;   // List of wanted evnum slices
  int active_evnum_slice;         // Indicates if we are in an active evnum zone
  general_slice_list function;    // List of wanted function
  thread_fun_list thread_fun;     /* List of thread activated by the option
				     function */
  function_time_list function_begin;   /* List to record the beginning time of
					  each function call */
  function_time_list function_time;    /* List to record the timing of each
					  function */
  int active_thread_fun;          /* Number of active thread activated by
				     function */
  general_slice_list gen_slice;   // List of wanted general zone
  int active_gen_slice;           // Number of active zones at a certain time
  int active;                     // Indicates if we are in active zone or not
} filter;

extern void init_filter();

extern void close_filter();

extern void filter_add_thread(int thread);

extern void filter_add_proc(int proc);

extern int is_in_proc_list(int proc);

extern void filter_add_logic(int logic);

extern int is_in_logic_list(int logic);

extern void filter_add_cpu(short int cpu);

extern void filter_add_event(mode type, unsigned code);

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

extern int is_valid(trace *tr);

extern int filter_pid_of_cpu(int i);

#endif
