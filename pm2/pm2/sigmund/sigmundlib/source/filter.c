#include "filter.h"
#include "tracelib.h"
#include "assert.h"
#include <stdlib.h>
#include <stdio.h>
#include "fut.h"
#include "lwpthread.h"
#include "graphlib.h"

filter options;

int table[NB_PROC];

/* compare 2 codes of events. returns true if codes are equal */
static int codecmp(mode type1, int code1, mode type2, int code2)
{
  if (type1 != type2) return FALSE;
  if (type1 == USER) {
    return (code1 == code2 );
  }
  if (code2 >= FKT_UNSHIFTED_LIMIT_CODE)
    return (code1 == code2 );
  else return (code1 == code2);
}


/* the names of the funcyion are explicit enough */
void init_filter() 
{
  int i;
  for(i=0; i < NB_PROC; i++) table[i] = -1;
  options.thread = THREAD_LIST_NULL;
  options.proc = PROC_LIST_NULL;
  options.logic = LOGIC_LIST_NULL;
  options.cpu = CPU_LIST_NULL;
  options.event = EVENT_LIST_NULL;
  options.time = TIME_SLICE_LIST_NULL;
  options.evnum_slice = EVNUM_SLICE_LIST_NULL;
  options.gen_slice = GENERAL_SLICE_LIST_NULL;
  options.function = GENERAL_SLICE_LIST_NULL;
  options.thread_fun = THREAD_FUN_LIST_NULL;
  options.active_thread = 1;
  options.active_time = 1;
  options.active_evnum_slice = 1;
  options.active_gen_slice = 1;
  options.active_thread_fun = 1;
  options.active = 1;
  options.function_time = FUNCTION_TIME_LIST_NULL;
  options.function_begin = FUNCTION_TIME_LIST_NULL;
}

void close_filter()
{
  // Il faudrait tout de même nettoyer la mémoire un peu.
}

void filter_add_thread(int thread)
{
  thread_list tmp;
  tmp = (thread_list) malloc(sizeof(struct thread_list_st));
  assert(tmp != NULL);
  tmp->next = options.thread;
  tmp->thread = thread;
  options.thread = tmp;
  options.active_thread = 0;
}

static int is_in_thread_list(int thread)
{
  if (options.thread != THREAD_LIST_NULL) {
    thread_list temp;
    temp = options.thread;
    while (temp != THREAD_LIST_NULL) {
      if (temp->thread == thread) break;
      temp = temp->next;
    }
    if (temp == THREAD_LIST_NULL) return FALSE;
  }
  return TRUE;
}

void filter_add_proc(int proc)
{
  proc_list tmp;
  tmp = (proc_list) malloc(sizeof(struct proc_list_st));
  assert(tmp != NULL);
  tmp->next = options.proc;
  tmp->proc = proc;
  options.proc = tmp;
}

int is_in_proc_list(int proc)
{
  if (options.proc != PROC_LIST_NULL) {
    proc_list temp;
    temp = options.proc;
    while (temp != PROC_LIST_NULL) {
      if (temp->proc == proc) break;
      temp = temp->next;
    }
    if (temp == PROC_LIST_NULL) return FALSE;
  }
  return TRUE;
}

void filter_add_logic(int logic)
{
  logic_list tmp;
  tmp = (logic_list) malloc(sizeof(struct logic_list_st));
  assert(tmp != NULL);
  tmp->next = options.logic;
  tmp->logic = logic;
  options.logic = tmp;
}

int is_in_logic_list(int logic)
{
  if (options.logic != LOGIC_LIST_NULL) {
    logic_list temp;
    temp = options.logic;
    while (temp != LOGIC_LIST_NULL) {
      if (temp->logic == logic) break;
      temp = temp->next;
    }
    if (temp == LOGIC_LIST_NULL) return FALSE;
  }
  return TRUE;
}



void filter_add_cpu(short int cpu)
{
  cpu_list tmp;
  tmp = (cpu_list) malloc(sizeof(struct cpu_list_st));
  assert(tmp != NULL);
  tmp->next = options.cpu;
  tmp->cpu = cpu;
  options.cpu = tmp;
}

static int is_in_cpu_list(short int cpu)
{
  if (options.cpu != CPU_LIST_NULL) {
    cpu_list temp;
    temp = options.cpu;
    while (temp != CPU_LIST_NULL) {
      if (temp->cpu == cpu) break;
      temp = temp->next;
    }
    if (temp == CPU_LIST_NULL) return FALSE;
  }
  return TRUE;
}

void filter_add_event(mode type, unsigned code)
{
  event_list tmp;
  tmp = (event_list) malloc(sizeof(struct event_list_st));
  assert(tmp != NULL);
  tmp->next = options.event;
  tmp->code = code;
  tmp->type = type;
  options.event = tmp;
}

static int is_in_event_list(trace *tr)
{
  if (options.event != EVENT_LIST_NULL) {
    event_list temp;
    temp = options.event;
    while (temp != EVENT_LIST_NULL) {
      if (codecmp(temp->type, temp->code, tr->type, tr->code)) break;
      temp = temp->next;
    }
    if (temp == EVENT_LIST_NULL) return FALSE;
  }
  return TRUE;
}

void filter_add_time_slice(u_64 begin, u_64 end)
{
  time_slice_list tmp;
  tmp = (time_slice_list) malloc(sizeof(struct time_slice_list_st));
  assert(tmp != NULL);
  tmp->next = options.time;
  tmp->begin = begin;
  tmp->end = end;
  options.time = tmp;
  options.active_time = 0;
}

static int is_in_time_list(trace *tr)
{
  if (options.time != TIME_SLICE_LIST_NULL) {
    time_slice_list temp;
    temp = options.time;
    while (temp != TIME_SLICE_LIST_NULL) {
      if (((unsigned) temp->begin <= (unsigned) tr->clock) &&
	  ((unsigned) temp->end >= (unsigned) tr->clock))
	break;
      temp = temp->next;
    }
    if (temp == TIME_SLICE_LIST_NULL) {
      options.active_time = 0;
      return FALSE;
    } else options.active_time = 1;
  }
  return TRUE;
}

void filter_add_evnum_slice(unsigned int begin, unsigned int end)
{
  evnum_slice_list tmp;
  tmp = (evnum_slice_list) malloc(sizeof(struct evnum_slice_list_st));
  assert(tmp != NULL);
  tmp->next = options.evnum_slice;
  tmp->begin = begin;
  tmp->end = end;
  options.evnum_slice = tmp;
  options.active_evnum_slice = 0;
}

static int is_in_evnum_list(trace *tr)
{
  if (options.evnum_slice != EVNUM_SLICE_LIST_NULL) {
    evnum_slice_list temp;
    temp = options.evnum_slice;
    while (temp != EVNUM_SLICE_LIST_NULL) {
      if (((unsigned) temp->begin <= (unsigned) tr->number) &&
	  ((unsigned) temp->end >= (unsigned) tr->number))
	break;
      temp = temp->next;
    }
    if (temp == EVNUM_SLICE_LIST_NULL) {
      options.active_evnum_slice = 0;
      return FALSE;
    } else options.active_evnum_slice = 1;
  }
  return TRUE;
}

void filter_add_function(mode begin_type, int begin, char begin_param_active, 
			  int begin_param, mode end_type, int end, 
			  char end_param_active, int end_param)
{
  general_slice_list tmp;
  tmp = (general_slice_list) malloc(sizeof(struct general_slice_list_st));
  assert(tmp != NULL);
  tmp->next = options.function;
  tmp->begin_type = begin_type;
  tmp->begin = begin;
  tmp->end_type = end_type;
  tmp->end = end;
  tmp->begin_param_active = begin_param_active;
  tmp->end_param_active = end_param_active;
  tmp->begin_param = begin_param;
  tmp->end_param = end_param;
  options.function = tmp;
  options.active_thread_fun = 0;
}

static void begin_function(trace *tr)
{
  function_time_list tmp;
  tmp = (function_time_list) malloc(sizeof(struct function_time_list_st));
  assert(tmp != NULL);
  tmp->next = options.function_begin;
  tmp->begin = tr->clock;
  tmp->code = tr->code;
  tmp->type = tr->type;
  tmp->thread = tr->thread;
  options.function_begin = tmp;
}

static void add_function_time(unsigned code, mode type, int thread, u_64 time, u_64 begin, u_64 end)
{
  function_time_list tmp;
  tmp = (function_time_list) malloc(sizeof(struct function_time_list_st));
  assert(tmp != NULL);
  tmp->next = options.function_time;
  tmp->time = time;
  tmp->thread = thread;
  tmp->begin = begin;
  tmp->end = end;
  tmp->code = code;
  tmp->type = type;
  options.function_time = tmp;
}

static void end_function(unsigned begin_code, mode begin_type, trace *tr)
{
  function_time_list tmp;
  function_time_list prev;
  prev = options.function_begin;
  if (prev == FUNCTION_TIME_LIST_NULL) {
    // Error with the bracketts(?), very bad problem, OUPS
    fprintf(stderr, "Erreur de parenthésage\n");
    return;   // Bad: return
  }
  if ((codecmp(prev->type, prev->code, begin_type, begin_code)) && \
      (tr->thread == prev->thread)) {
    options.function_begin = prev->next;
    add_function_time(begin_code, begin_type, tr->thread, tr->clock - prev->begin, prev->begin, tr->clock);
    free(prev);
    return;
  }
  tmp = prev->next;
  while (tmp != FUNCTION_TIME_LIST_NULL) {
    if ((codecmp(begin_type, begin_code, tmp->type, tmp->code)) && \
	(tr->thread == tmp->thread)){
      prev->next = tmp->next;
      add_function_time(begin_code, begin_type, tr->thread, tr->clock - tmp->begin, tmp->begin, tr->clock);
      free(tmp);
      return;
    }
    prev = tmp;
    tmp = tmp->next;
  }
   // Error with the bracketts(?), very bad problem, OUPS
  fprintf(stderr, "Erreur de parenthésage\n");
  return;    // Bad: return
}

int filter_get_function_time(struct function_time_list_st *fct_time)
{
  function_time_list tmp;
  function_time_list prev;
  tmp = options.function_time;
  if (tmp == FUNCTION_TIME_LIST_NULL) return -1;
  prev = tmp;
  while (tmp->next != FUNCTION_TIME_LIST_NULL) {
    prev = tmp;
    tmp = tmp->next;
  }
  if(prev->next != FUNCTION_TIME_LIST_NULL)
    prev->next = FUNCTION_TIME_LIST_NULL;
  else options.function_time = FUNCTION_TIME_LIST_NULL;
  *fct_time = *tmp;
  free(tmp);
  return 0;
}

static void filter_add_thread_fun(int thread)
{
  thread_fun_list tmp;
  tmp = options.thread_fun;
  while (tmp != THREAD_FUN_LIST_NULL) {
    if (tmp->thread == thread) break;
    tmp = tmp->next;
  }
  if (tmp == THREAD_FUN_LIST_NULL) {
    options.active_thread_fun++;
    tmp = (thread_fun_list) malloc(sizeof(struct thread_fun_list_st));
    assert(tmp != NULL);
    tmp->next = options.thread_fun;
    tmp->thread = thread;
    tmp->number = 1;
    options.thread_fun = tmp;
  } else tmp->number++;
}

static void filter_del_thread_fun(int thread)
{
  thread_fun_list tmp;
  thread_fun_list prev;
  if (options.thread_fun == THREAD_FUN_LIST_NULL) return; // Error
  tmp = options.thread_fun->next;
  if (options.thread_fun->thread == thread) {
    options.thread_fun->number--;
    if (options.thread_fun->number == 0){
      set_thread_disactivated(thread, TRUE);
      options.active_thread_fun--;
      free(options.thread_fun);
      options.thread_fun = tmp;
    }
    return;
  }
  prev = options.thread_fun;
  while (tmp != THREAD_FUN_LIST_NULL) {
    if (tmp->thread == thread) {
      tmp->number--;
      if (tmp->number == 0) {
	set_thread_disactivated(thread, TRUE);
	prev->next = tmp->next;
	free(tmp);
      }
      return;
    }
  }
  return; // Error
}

static int is_in_thread_fun_list(int thread)
{
  thread_fun_list temp;
  temp = options.thread_fun;
  while (temp != THREAD_FUN_LIST_NULL) {
    if (temp->thread == thread) break;
    temp = temp->next;
  }
  if (options.function != GENERAL_SLICE_LIST_NULL) {
    if (temp == THREAD_FUN_LIST_NULL) return FALSE;
  }
  return TRUE;
}

void filter_add_gen_slice(mode begin_type, int begin, char begin_param_active, 
			  int begin_param, mode end_type, int end, 
			  char end_param_active, int end_param)
{
  general_slice_list tmp;
  tmp = (general_slice_list) malloc(sizeof(struct general_slice_list_st));
  assert(tmp != NULL);
  tmp->next = options.gen_slice;
  tmp->begin_type = begin_type;
  tmp->begin = begin;
  tmp->end_type = end_type;
  tmp->end = end;
  tmp->begin_param_active = begin_param_active;
  tmp->end_param_active = end_param_active;
  tmp->begin_param = begin_param;
  tmp->end_param = end_param;
  options.gen_slice = tmp;
  options.active_gen_slice = 0;
}

static void search_begin_gen_slice_list(trace *tr)
{
  if (options.gen_slice != GENERAL_SLICE_LIST_NULL) {
    general_slice_list temp;
    temp = options.gen_slice;
    while (temp != GENERAL_SLICE_LIST_NULL) {
      if (codecmp(temp->begin_type, temp->begin, tr->type, tr->code)) {
	if (temp->begin_param_active == FALSE)
	  options.active_gen_slice++;
	else
	  if (temp->begin_param == tr->args[0])
	    options.active_gen_slice++;
      }
      temp = temp->next;
    }
  }
}

static void search_end_gen_slice_list(trace *tr)
{
  if (options.gen_slice != GENERAL_SLICE_LIST_NULL) {
    general_slice_list temp;
    temp = options.gen_slice;
    while (temp != GENERAL_SLICE_LIST_NULL) {
      if (codecmp(temp->end_type, temp->end, tr->type, tr->code)) {
	if (temp->end_param_active == FALSE) {
	  if (options.active_gen_slice > 0) options.active_gen_slice--;
	}
	else {
	  if (temp->end_param == tr->args[0])
	    if (options.active_gen_slice > 0) options.active_gen_slice--;
	}
      }
      temp = temp->next;
    }
  }
}


static void search_begin_function(trace *tr)
{
  if (options.function != GENERAL_SLICE_LIST_NULL) {
    general_slice_list temp;
    temp = options.function;
    while (temp != GENERAL_SLICE_LIST_NULL) {
      if (codecmp(temp->begin_type, temp->begin, tr->type, tr->code)) {
	if (temp->begin_param_active == FALSE) {
	  filter_add_thread_fun(tr->thread);
	  begin_function(tr);
	}
	else
	  if (temp->begin_param == tr->args[0]) {
	    filter_add_thread_fun(tr->thread);
	    begin_function(tr);
	  }
      }
      temp = temp->next;
    }
  } 
}

static void search_end_function(trace *tr)
{
  if (options.function != GENERAL_SLICE_LIST_NULL) {
    general_slice_list temp;
    temp = options.function;
    while (temp != GENERAL_SLICE_LIST_NULL) {
      if (codecmp(temp->end_type, temp->end, tr->type, tr->code)) {
	if (temp->end_param_active == FALSE) {
	  filter_del_thread_fun(tr->thread);
	  end_function(temp->begin, temp->begin_type, tr);
	}
	else {
	  if (temp->end_param == tr->args[0]) {
	    filter_del_thread_fun(tr->thread);
	    end_function(temp->begin, temp->begin_type, tr);
	  }
	}
      }
      temp = temp->next;
    }
  }
}


/* say if an event is valid for the filter given */
int is_valid(trace *tr)
{
  /* We found interesting information about cpu */
  if (table[tr->cpu] == -1) {
    set_pid_last_up(tr->pid, tr->clock);
    if (is_in_cpu_list(tr->cpu) == TRUE)
      if (is_in_proc_list(tr->pid) == TRUE)
	if (is_in_logic_list(logic_of_lwp(tr->pid)) == TRUE)
	  options.active_proc++;
    table[tr->cpu] = tr->pid;
  }
  if (tr->type == KERNEL) {
    if (tr->code == FKT_SWITCH_TO_CODE) {
      /* FKT_SWITCH_TO: We must update the information about cpu and last_up
	 If the old process was traced, active_proc--
	    If the thread associated was traced, active_thread--
               If it was traced in a function active_thread_fun--
	 If the new process is to be traced, active_proc++
	    If the thread associated is to be traced, active_thread++
	       If it is to be traced in a function active_thread_fun++
       */
      table[tr->cpu]= tr->args[0];
      set_pid_last_up(tr->args[0], tr->clock);
      if ((is_in_proc_list(tr->pid) == TRUE) && \
	  (is_in_cpu_list(tr->cpu) == TRUE) && \
	  (is_in_logic_list(logic_of_lwp(tr->pid)) == TRUE)) {
	options.active_proc--;
	if (is_lwp(tr->pid) == TRUE) {
	  set_active_lwp(tr->pid, FALSE);
	  if (is_in_thread_list(tr->thread) == TRUE) {
	    options.active_thread--;	
	    //	    set_thread_disactivated(tr->thread, TRUE);
	    if (is_in_thread_fun_list(tr->thread) == TRUE)
	      options.active_thread_fun--;
	  }
	}
      }
      if ((is_in_proc_list(tr->args[0]) == TRUE) && \
	  (is_in_cpu_list(tr->cpu) == TRUE) && \
	  (is_in_logic_list(logic_of_lwp(tr->args[0])) == TRUE)) {
	options.active_proc++;
	if (is_lwp(tr->args[0]) == TRUE) {
	  set_active_lwp(tr->args[0], TRUE);
	  set_cpu_lwp(tr->args[0], tr->args[0]);
	  if (is_in_thread_list(thread_of_lwp(tr->args[0])) == TRUE) {
	    options.active_thread++;
	    if (is_in_thread_fun_list(thread_of_lwp(tr->args[0])) == TRUE) 
	      options.active_thread_fun++;
	  }
	}
      }
    } else if (tr->code == FKT_USER_FORK_CODE) {
      if ((is_in_proc_list(tr->pid) == TRUE) && \
	  (is_in_cpu_list(tr->cpu) == TRUE) && \
	  (is_in_logic_list(tr->args[1])))
	filter_add_lwp(tr->pid, tr->args[1], tr->args[0], TRUE, tr->cpu);
      else filter_add_lwp(tr->pid, tr->args[1], tr->args[0], FALSE, tr->cpu);
    }
  }
  else if (tr->code == FUT_SWITCH_TO_CODE) {
    /* FUT_SWITCH_TO: update active_thread and active_thread_fun
     */
    if (is_active_lwp_of_thread(tr->thread) == TRUE) {
      if (is_in_thread_list(tr->thread) == TRUE) {
	options.active_thread--;
  	//       	set_thread_disactivated(tr->thread, TRUE);
	if (is_in_thread_fun_list(tr->thread) == TRUE)
	  options.active_thread_fun--;
      }
      if (is_in_thread_list(tr->args[1]) == TRUE) {
	options.active_thread++;
	if (is_in_thread_fun_list(tr->args[1]) == TRUE) 
	  options.active_thread_fun++;
      }
    }
    change_lwp_thread(tr->thread, tr->args[1]);
  } 
  //else if (tr->code == FUT_NEW_LWP_CODE) {
  //    /* FUT_NEW_LWP: adds to lwpthread with activity corresponding to its
  //       being traced or not
  //     */
  //    if ((is_in_proc_list(tr->args[0]) == TRUE) &&
//	(is_in_cpu_list(tr->cpu) == TRUE) &&
//	(is_in_logic_list(tr->args[1])))
  //      filter_add_lwp(tr->args[0], tr->args[1], tr->args[2], TRUE, tr->cpu);
  //  else filter_add_lwp(tr->args[0], tr->args[1], tr->args[2], FALSE, tr->cpu);
  /* { */ else if (tr->code == FUT_THREAD_BIRTH_CODE) {
    /* THREAD_BIRTH: adds this thread in graphlib and disactivate it*/
    set_thread_disactivated(tr->args[0], TRUE);
  }

  // Updates the graphical decalage for this thread
  if (tr->type == USER) {
    if (((tr->code) < 0x8000) && (tr->code > 0x40000)) {
      if (((tr->code) & 0x100) == 0) add_thread_dec(tr->thread, 1);
      if (((tr->code) & 0x100) == 0x100) add_thread_dec(tr->thread, -1);
    }
  }

  if (is_in_cpu_list(tr->cpu) == TRUE) {
    if (is_in_proc_list(tr->pid) == TRUE) {
      if (is_in_logic_list(logic_of_lwp(tr->pid)) == TRUE) {
	if (is_in_thread_list(tr->thread) == TRUE) {
	  if (is_in_evnum_list(tr) == TRUE) {
	    if (is_in_time_list(tr) == TRUE) {
	      search_begin_gen_slice_list(tr);
	      search_begin_function(tr);
	      if (options.active_gen_slice == 0) set_thread_disactivated(tr->thread, TRUE);
	      
	      options.active = options.active_proc && options.active_thread && \
		options.active_gen_slice && options.active_thread_fun;
	      
	      if (is_in_thread_fun_list(tr->thread) == FALSE) return FALSE;
	      if (options.active_gen_slice == 0) return FALSE;
	      
	      search_end_function(tr);
	      search_end_gen_slice_list(tr);
	      
	      if (is_in_event_list(tr) == FALSE) return FALSE;
	      
	      
	    } else {options.active = FALSE; return FALSE;}
	  } else {options.active = FALSE; return FALSE;}
	} else {
	  options.active = options.active_proc && options.active_thread && \
	    options.active_gen_slice && options.active_thread_fun && \
	    options.active_time && options.active_evnum_slice;
	  return FALSE;
	}
      } else {
	options.active = options.active_proc && options.active_thread && \
	  options.active_gen_slice && options.active_thread_fun && \
	  options.active_time && options.active_evnum_slice;
	return FALSE;
      }
    } else {
      options.active = options.active_proc && options.active_thread && \
	options.active_gen_slice && options.active_thread_fun && \
	options.active_time && options.active_evnum_slice;
      return FALSE;
    } 
  } else {
    options.active = options.active_proc && options.active_thread && \
      options.active_gen_slice && options.active_thread_fun && \
      options.active_time && options.active_evnum_slice;
    return FALSE;
  } 
  return TRUE;
}

int filter_pid_of_cpu(int i)
{
  return table[i];
}
