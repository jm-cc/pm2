#include "filter.h"
#include "tracelib.h"
#include "assert.h"
#include <stdlib.h>
#include <stdio.h>
#include "fut_code.h"

filter options;

struct time_st {
  u_64 active_time;
  u_64 idle_time;
  u_64 last_time;
  int old_active;
  int first;
};

struct 

#define NB_PROC 16

int table[NB_PROC];

static int codecmp(mode type1, int code1, mode type2, int code2)
{
  if (type1 != type2) return FALSE;
  if (type1 == USER) {
    return (code1 >> 8 == code2 >> 8);
  }
  if (code2 >= FKT_UNSHIFTED_LIMIT_CODE)
    return (code1 >> 8 == code2 >> 8);
  else return (code1 == code2);
}

void init_filter() 
{
  int i;
  for(i=0; i < NB_PROC; i++) table[i] = FALSE;
  options.thread = THREAD_LIST_NULL;
  options.proc = PROC_LIST_NULL;
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
  //  options.active = 1;
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

static int is_in_proc_list(int proc)
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

void filter_add_event(mode type, int code)
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
  // options.active = 0;
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
  //  options.active = 0;
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

/*
  void search_begin_evnum_slice_list(trace *tr)
  {
  if (options.evnum_slice != EVNUM_SLICE_LIST_NULL) {
  evnum_slice_list temp;
  temp = options.evnum_slice;
  while (temp != EVNUM_SLICE_LIST_NULL) {
  if (temp->begin == tr->number)
  options.active_evnum_slice++;
  temp = temp->next;
  }
  }
  }
  
  void search_end_evnum_slice_list(trace *tr)
  {
  if (options.evnum_slice != EVNUM_SLICE_LIST_NULL) {
  evnum_slice_list temp;
  temp = options.evnum_slice;
  while (temp != EVNUM_SLICE_LIST_NULL) {
  if (temp->end == tr->number)
  if (options.active_evnum_slice > 0) options.active_evnum_slice--;
  temp = temp->next;
  }
  } 
  }
*/

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

static void filter_add_thread_fun(int thread)
{
  thread_fun_list tmp;
  tmp = options.thread_fun;
  while (tmp != THREAD_FUN_LIST_NULL) {
    if (tmp->thread == thread) break;
    tmp =tmp->next;
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
  if (options.thread_fun == NULL) return; // Erreur
  tmp = options.thread_fun->next;
  if (options.thread_fun->thread == thread) {
    options.thread_fun->number--;
    if (options.thread_fun->number == 0){
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
	prev->next = tmp->next;
	free(tmp);
      }
      return;
    }
  }
  return; // Erreur
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
  //  options.active = 0;
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
	if (temp->begin_param_active == FALSE)
	  filter_add_thread_fun(tr->thread);
	else
	  if (temp->begin_param == tr->args[0])
	    filter_add_thread_fun(tr->thread);
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
	}
	else {
	  if (temp->end_param == tr->args[0])
	    filter_del_thread_fun(tr->thread);
	}
      }
      temp = temp->next;
    }
  }
}

/*
  int is_valid(trace *tr)
  { 
  if (is_in_cpu_list(tr->proc) == FALSE) return FALSE;
  if (is_in_proc_list(tr->pid) == FALSE) return FALSE;
  if (is_in_thread_list(tr->thread) == FALSE) return FALSE;
  if (is_in_event_list(tr) == FALSE) return FALSE;
  if (is_in_time_list(tr) == FALSE) return FALSE;
  search_begin_evnum_slice_list(tr);
  search_begin_gen_slice_list(tr);
  search_begin_function(tr);
  
  //options.active = options.active_time && options.active_evnum_slice && options.active_gen_slice;
  
  if (options.active_time && options.active_evnum_slice && options.active_gen_slice == 0) return FALSE;
  
  search_end_evnum_slice_list(tr);
  search_end_gen_slice_list(tr);
  
  
  if (is_in_thread_fun_list(tr->thread) == FALSE) return FALSE;
  
  search_end_function(tr);  
  
  return TRUE;
  }
*/

static void filter_add_lwp(int lwp, int thread, int active)
{
  lwp_thread_list tmp;
  tmp = (lwp_thread_list) malloc(sizeof(struct lwp_thread_list_st));
  assert(tmp != NULL);
  tmp->next = options.lwp_thread;
  tmp->lwp = lwp;
  tmp->thread = thread;
  tmp->active = active;
  options.lwp_thread = tmp;
}

static int is_active_lwp_of_thread(int thread)
{
  lwp_thread_list tmp;
  tmp = options.lwp_thread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->thread == thread) return tmp->active;
    tmp = tmp->next;
  }
  return -1;  // Erreur
}

static int is_active_lwp(int lwp)
{
  lwp_thread_list tmp;
  tmp = options.lwp_thread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->lwp == lwp) return tmp->active;
    tmp = tmp->next;
  }
  return -1; // Erreur
}

static int lwp_of_thread(int thread)
{
  lwp_thread_list tmp;
  tmp = options.lwp_thread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->thread == thread) break;
    tmp = tmp->next;
  }
  if (tmp == LWP_THREAD_LIST_NULL) return -1;
  return tmp->lwp;
}

static void change_lwp_thread(int oldthread, int newthread)
{
  lwp_thread_list tmp;
  tmp = options.lwp_thread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->thread == oldthread) break;
    tmp = tmp->next;
  }
  if (tmp == LWP_THREAD_LIST_NULL) return;  //Erreur
  tmp->thread = newthread;
}

int is_lwp(int lwp)
{
  lwp_thread_list tmp;
  tmp = options.lwp_thread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->lwp == lwp) return TRUE;
    tmp = tmp->next;
  }
  return TRUE;
}

void set_active_lwp(int lwp, int active)
{
  lwp_thread_list tmp;
  tmp = options.lwp_thread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->lwp == lwp) {
      tmp->active = active;
      return;
    }
    tmp = tmp->next;
  }
  return; // Erreur
}

/*
  void set_active_thread(int thread, int active)
  {
  lwp_thread_list tmp;
  tmp = options.lwp_thread;
  while (tmp != LWP_THREAD_LIST_NULL) {
  if (tmp->thread == thread) {
  tmp->active_thread = active;
  return;
  }
  tmp = tmp->next;
  }
  return; // Erreur
  }
  
  int is_active_thread(int thread)
  {
  lwp_thread_list tmp;
  tmp = options.lwp_thread;
  while (tmp != LWP_THREAD_LIST_NULL) {
  if (tmp->thread == thread) return tmp->active_thread;
  tmp = tmp->next;
  }
  return -1; // Erreur
  }
*/

int thread_of_lwp(int lwp)
{
lwp_thread_list tmp;
  tmp = options.lwp_thread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->lwp == lwp) return tmp->thread;
    tmp = tmp->next;
  }
  return -1;
}



int is_valid_new(trace *tr)
{
  if (table[tr->proc] == FALSE) {
    if (is_in_cpu_list(tr->proc) == TRUE)
      if (is_in_proc_list(tr->pid) == TRUE)
	options.active_proc++;
    table[tr->proc] = TRUE;
  }
  if (tr->type == KERNEL) {
    if (tr->code >> 8 == FKT_SWITCH_TO_CODE) {
      if ((is_in_proc_list(tr->pid) == TRUE) && (is_in_cpu_list(tr->proc) == TRUE)) {
	options.active_proc--;
	if (is_lwp(tr->pid) == TRUE) {
	  set_active_lwp(tr->pid, FALSE);
	  if (is_in_thread_list(tr->thread) == TRUE) {
	    options.active_thread--;
	    //	    set_active_thread(tr->thread, FALSE);
	    if (is_in_thread_fun_list(tr->thread) == TRUE)
	      options.active_thread_fun--;
	  }
	}
      }
      if ((is_in_proc_list(tr->args[1]) == TRUE) && (is_in_cpu_list(tr->args[0]) == TRUE)) {
	options.active_proc++;
	if (is_lwp(tr->args[1]) == TRUE) {
	  set_active_lwp(tr->args[1], TRUE);
	  if (is_in_thread_list(thread_of_lwp(tr->args[1])) == TRUE) {
	    options.active_thread++;
	    //	    set_active_thread(thread_of_lwp(tr->args[1]), TRUE);
	    if (is_in_thread_fun_list(thread_of_lwp(tr->args[1])) == TRUE) 
	      options.active_thread_fun++;
	  }
	}
      }
    }
  } else if (tr->code >> 8 == FUT_SWITCH_TO_CODE) {
    if (is_active_lwp_of_thread(tr->thread) == TRUE) {
      if (is_in_thread_list(tr->thread) == TRUE) {
	options.active_thread--;
	//	set_active_thread(tr->thread, FALSE);
	if (is_in_thread_fun_list(tr->thread) == TRUE)
	  options.active_thread_fun--;
      }
      if (is_in_thread_list(tr->args[0]) == TRUE) {
	options.active_thread++;
	//	set_active_thread(tr->thread, TRUE);        // not tr->args[0] we haven't switch yet in fact....
	if (is_in_thread_fun_list(tr->args[0]) == TRUE) 
	  options.active_thread_fun++;
      }
    }
    change_lwp_thread(tr->thread, tr->args[0]);
  } else if (tr->code >> 8 == FUT_KEYCHANGE_CODE) {
    if ((is_in_proc_list(tr->pid) == TRUE) && (is_in_cpu_list(tr->proc) == TRUE))
      filter_add_lwp(tr->proc, tr->thread, TRUE);
    else filter_add_lwp(tr->proc, tr->thread, FALSE);
  }

  if (is_in_cpu_list(tr->proc) == TRUE) {
    if (is_in_proc_list(tr->pid) == TRUE) {
      if (is_in_thread_list(tr->thread) == TRUE) {
	if (is_in_evnum_list(tr) == TRUE) {
	  if (is_in_time_list(tr) == TRUE) {
	    search_begin_gen_slice_list(tr);
	    search_begin_function(tr);
	    

	    options.active = options.active_proc && options.active_thread && options.active_gen_slice && options.active_thread_fun;

	    if ((is_in_thread_fun_list(tr->thread) == FALSE) || (options.active_gen_slice == 0)) return FALSE;

	    search_end_function(tr);
	    search_end_gen_slice_list(tr);
	  
	    
	  } else {options.active = FALSE; return FALSE;}
	} else {options.active = FALSE; return FALSE;}
      } else {
	options.active = options.active_proc && options.active_thread && options.active_gen_slice && options.active_thread_fun && options.active_time && options.active_evnum_slice;
	return FALSE;
      }
    } else {
      options.active = options.active_proc && options.active_thread && options.active_gen_slice && options.active_thread_fun && options.active_time && options.active_evnum_slice;
      return FALSE;
    }
  } else {
    options.active = options.active_proc && options.active_thread && options.active_gen_slice && options.active_thread_fun && options.active_time && options.active_evnum_slice;
    return FALSE;
  }
  return TRUE;
}
