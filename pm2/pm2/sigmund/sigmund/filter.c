#include "filter.h"
#include "tracelib.h"
#include "assert.h"
#include <stdlib.h>
#include <stdio.h>

filter options;

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
  options.thread = THREAD_LIST_NULL;
  options.proc = PROC_LIST_NULL;
  options.cpu = CPU_LIST_NULL;
  options.event = EVENT_LIST_NULL;
  options.time = TIME_SLICE_LIST_NULL;
  options.evnum_slice = EVNUM_SLICE_LIST_NULL;
  options.gen_slice = GENERAL_SLICE_LIST_NULL;
  options.function = GENERAL_SLICE_LIST_NULL;
  options.thread_fun = THREAD_LIST_NULL;
  options.active_time = 1;
  options.active_evnum_slice = 1;
  options.active_gen_slice = 1;
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
}

static void filter_add_thread_fun(int thread)
{
  thread_list tmp;
  //  fprintf(stderr,"ADD\n");
  tmp = (thread_list) malloc(sizeof(struct thread_list_st));
  assert(tmp != NULL);
  tmp->next = options.thread_fun;
  tmp->thread = thread;
  options.thread_fun = tmp;
}

static void filter_del_thread_fun(int thread)
{
  thread_list tmp;
  thread_list prev;
  //  fprintf(stderr,"DEL\n");
  if (options.thread_fun == NULL) return; // Erreur
  tmp = options.thread_fun->next;
  if (options.thread_fun->thread == thread) {
    free(options.thread_fun);
    options.thread_fun = tmp;
    return;
  }
  prev = options.thread;
  while (tmp != NULL) {
    if (tmp->thread == thread) {
      prev->next = tmp->next;
      free(tmp);
      return;
    }
  }
  return; // Erreur
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

void filter_add_cpu(short int cpu)
{
  cpu_list tmp;
  tmp = (cpu_list) malloc(sizeof(struct cpu_list_st));
  assert(tmp != NULL);
  tmp->next = options.cpu;
  tmp->cpu = cpu;
  options.cpu = tmp;
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

void filter_add_time_slice(u_64 begin, u_64 end)
{
  time_slice_list tmp;
  tmp = (time_slice_list) malloc(sizeof(struct time_slice_list_st));
  assert(tmp != NULL);
  tmp->next = options.time;
  tmp->t.begin = begin;
  tmp->t.end = end;
  options.time = tmp;
  options.active_time = 0;
  // options.active = 0;
}

void filter_add_evnum_slice(unsigned int begin, unsigned int end)
{
  evnum_slice_list tmp;
  tmp = (evnum_slice_list) malloc(sizeof(struct evnum_slice_list_st));
  assert(tmp != NULL);
  tmp->next = options.evnum_slice;
  tmp->t.begin = begin;
  tmp->t.end = end;
  options.evnum_slice = tmp;
  options.active_evnum_slice = 0;
  //  options.active = 0;
}

void filter_add_gen_slice(mode begin_type, int begin, char begin_param_active, 
			  int begin_param, mode end_type, int end, 
			  char end_param_active, int end_param)
{
  general_slice_list tmp;
  tmp = (general_slice_list) malloc(sizeof(struct general_slice_list_st));
  assert(tmp != NULL);
  tmp->next = options.gen_slice;
  tmp->t.begin_type = begin_type;
  tmp->t.begin = begin;
  tmp->t.end_type = end_type;
  tmp->t.end = end;
  tmp->t.begin_param_active = begin_param_active;
  tmp->t.end_param_active = end_param_active;
  tmp->t.begin_param = begin_param;
  tmp->t.end_param = end_param;
  options.gen_slice = tmp;
  options.active_gen_slice = 0;
  //  options.active = 0;
}

void filter_add_function(mode begin_type, int begin, char begin_param_active, 
			  int begin_param, mode end_type, int end, 
			  char end_param_active, int end_param)
{
  general_slice_list tmp;
  tmp = (general_slice_list) malloc(sizeof(struct general_slice_list_st));
  assert(tmp != NULL);
  tmp->next = options.function;
  tmp->t.begin_type = begin_type;
  tmp->t.begin = begin;
  tmp->t.end_type = end_type;
  tmp->t.end = end;
  tmp->t.begin_param_active = begin_param_active;
  tmp->t.end_param_active = end_param_active;
  tmp->t.begin_param = begin_param;
  tmp->t.end_param = end_param;
  options.function = tmp;
}

// Attention le end mange une trace la dernière...
int is_valid2(trace *tr)
{ 
  if (options.time != TIME_SLICE_LIST_NULL) {
    time_slice_list temp;
    temp = options.time;
    while (temp != TIME_SLICE_LIST_NULL) {
      if (((unsigned) temp->t.begin <= (unsigned) tr->clock) &&
	  ((unsigned) temp->t.end >= (unsigned) tr->clock))
	break;
      temp = temp->next;
    }
    if (temp == TIME_SLICE_LIST_NULL) {
      if ((options.gen_slice == GENERAL_SLICE_LIST_NULL) &&
	  (options.evnum_slice == EVNUM_SLICE_LIST_NULL))
	options.active_time = 1;
      else options.active_time = 0;
      return FALSE;
    }
  }
  if (options.evnum_slice != EVNUM_SLICE_LIST_NULL) {
    evnum_slice_list temp;
    temp = options.evnum_slice;
    while (temp != EVNUM_SLICE_LIST_NULL) {
      if (temp->t.begin == tr->number)
	options.active_evnum_slice++;
      temp = temp->next;
    }
  }
  if (options.gen_slice != GENERAL_SLICE_LIST_NULL) {
    general_slice_list temp;
    temp = options.gen_slice;
    while (temp != GENERAL_SLICE_LIST_NULL) {
      if (codecmp(temp->t.begin_type, temp->t.begin, tr->type, tr->code)) {
	if (temp->t.begin_param_active == FALSE)
	  options.active_gen_slice++;
	else
	  if (temp->t.begin_param == tr->args[0])
	    options.active_gen_slice++;
      }
      temp = temp->next;
    }
  }
  options.active = options.active_time && options.active_evnum_slice && options.active_gen_slice;
  if (options.active == 0) return FALSE;
  if (options.thread != THREAD_LIST_NULL) {
    thread_list temp;
    temp = options.thread;
    while (temp != THREAD_LIST_NULL) {
      if (temp->thread == tr->thread) break;
      temp = temp->next;
    }
    if (temp == THREAD_LIST_NULL) return FALSE;
  }
  if (options.proc != PROC_LIST_NULL) {
    proc_list temp;
    temp = options.proc;
    while (temp != PROC_LIST_NULL) {
      if (temp->proc == tr->pid) break;
      temp = temp->next;
    }
    if (temp == PROC_LIST_NULL) return FALSE;
  }
  if (options.cpu != CPU_LIST_NULL) {
    cpu_list temp;
    temp = options.cpu;
    while (temp != CPU_LIST_NULL) {
      if (temp->cpu == tr->proc) break;
      temp = temp->next;
    }
    if (temp == CPU_LIST_NULL) return FALSE;
  }
  if (options.event != EVENT_LIST_NULL) {
    event_list temp;
    temp = options.event;
    while (temp != EVENT_LIST_NULL) {
      if (codecmp(temp->type, temp->code, tr->type, tr->code)) break;
      temp = temp->next;
    }
    if (temp == EVENT_LIST_NULL) return FALSE;
  }
  if (options.thread_fun != THREAD_LIST_NULL) {
    thread_list temp;
    temp = options.thread_fun;
    while (temp != THREAD_LIST_NULL) {
      if (temp->thread == tr->thread) break;
      temp = temp->next;
    }
    if (temp == THREAD_LIST_NULL) return FALSE;
  }
  if (options.evnum_slice != EVNUM_SLICE_LIST_NULL) {
    evnum_slice_list temp;
    temp = options.evnum_slice;
    while (temp != EVNUM_SLICE_LIST_NULL) {
      if (temp->t.end == tr->number)
	if (options.active_evnum_slice > 0) options.active_evnum_slice--;
      temp = temp->next;
    }
  }
  if (options.gen_slice != GENERAL_SLICE_LIST_NULL) {
    general_slice_list temp;
    temp = options.gen_slice;
    while (temp != GENERAL_SLICE_LIST_NULL) {
      if (codecmp(temp->t.end_type, temp->t.end, tr->type, tr->code)) {
	if (temp->t.end_param_active == FALSE) {
	  if (options.active_gen_slice > 0) options.active_gen_slice--;
	}
	else {
	  if (temp->t.end_param == tr->args[0])
	    if (options.active_gen_slice > 0) options.active_gen_slice--;
	}
      }
      temp = temp->next;
    }
  }
  return TRUE;
}


// Attention le end mange une trace la dernière...


int is_valid(trace *tr)
{ 
  if (options.cpu != CPU_LIST_NULL) {
    cpu_list temp;
    temp = options.cpu;
    while (temp != CPU_LIST_NULL) {
      if (temp->cpu == tr->proc) break;
      temp = temp->next;
    }
    if (temp == CPU_LIST_NULL) return FALSE;
  }
  if (options.proc != PROC_LIST_NULL) {
    proc_list temp;
    temp = options.proc;
    while (temp != PROC_LIST_NULL) {
      if (temp->proc == tr->pid) break;
      temp = temp->next;
    }
    if (temp == PROC_LIST_NULL) return FALSE;
  }
  if (options.thread != THREAD_LIST_NULL) {
    thread_list temp;
    temp = options.thread;
    while (temp != THREAD_LIST_NULL) {
      if (temp->thread == tr->thread) break;
      temp = temp->next;
    }
    if (temp == THREAD_LIST_NULL) return FALSE;
  }

  if (options.event != EVENT_LIST_NULL) {
    event_list temp;
    temp = options.event;
    while (temp != EVENT_LIST_NULL) {
      if (codecmp(temp->type, temp->code, tr->type, tr->code)) break;
      temp = temp->next;
    }
    if (temp == EVENT_LIST_NULL) return FALSE;
  }
  if (options.time != TIME_SLICE_LIST_NULL) {
    time_slice_list temp;
    temp = options.time;
    while (temp != TIME_SLICE_LIST_NULL) {
      if (((unsigned) temp->t.begin <= (unsigned) tr->clock) &&
	  ((unsigned) temp->t.end >= (unsigned) tr->clock))
	break;
      temp = temp->next;
    }
    if (temp == TIME_SLICE_LIST_NULL) {
      if ((options.gen_slice == GENERAL_SLICE_LIST_NULL) &&
	  (options.evnum_slice == EVNUM_SLICE_LIST_NULL))
	options.active_time = 1;
      else options.active_time = 0;
      return FALSE;
    }
  }
  if (options.evnum_slice != EVNUM_SLICE_LIST_NULL) {
    evnum_slice_list temp;
    temp = options.evnum_slice;
    while (temp != EVNUM_SLICE_LIST_NULL) {
      if (temp->t.begin == tr->number)
	options.active_evnum_slice++;
      temp = temp->next;
    }
  }
  
  if (options.function != GENERAL_SLICE_LIST_NULL) {
    general_slice_list temp;
    temp = options.function;
    while (temp != GENERAL_SLICE_LIST_NULL) {
      if (codecmp(temp->t.begin_type, temp->t.begin, tr->type, tr->code)) {
	if (temp->t.begin_param_active == FALSE)
	  filter_add_thread_fun(tr->thread);
	else
	  if (temp->t.begin_param == tr->args[0])
	    filter_add_thread_fun(tr->thread);
      }
      temp = temp->next;
    }
  }
  
  if (options.gen_slice != GENERAL_SLICE_LIST_NULL) {
    general_slice_list temp;
    temp = options.gen_slice;
    while (temp != GENERAL_SLICE_LIST_NULL) {
      if (codecmp(temp->t.begin_type, temp->t.begin, tr->type, tr->code)) {
	if (temp->t.begin_param_active == FALSE)
	  options.active_gen_slice++;
	else
	  if (temp->t.begin_param == tr->args[0])
	    options.active_gen_slice++;
      }
      temp = temp->next;
    }
  }
 

  //options.active = options.active_time && options.active_evnum_slice && options.active_gen_slice;
  
  if (options.active_time && options.active_evnum_slice && options.active_gen_slice== 0) return FALSE;
 





  if (options.evnum_slice != EVNUM_SLICE_LIST_NULL) {
    evnum_slice_list temp;
    temp = options.evnum_slice;
    while (temp != EVNUM_SLICE_LIST_NULL) {
      if (temp->t.end == tr->number)
	if (options.active_evnum_slice > 0) options.active_evnum_slice--;
      temp = temp->next;
    }
  } 
  
  if (options.gen_slice != GENERAL_SLICE_LIST_NULL) {
    general_slice_list temp;
    temp = options.gen_slice;
    while (temp != GENERAL_SLICE_LIST_NULL) {
      if (codecmp(temp->t.end_type, temp->t.end, tr->type, tr->code)) {
	if (temp->t.end_param_active == FALSE) {
	  if (options.active_gen_slice > 0) options.active_gen_slice--;
	}
	else {
	  if (temp->t.end_param == tr->args[0])
	    if (options.active_gen_slice > 0) options.active_gen_slice--;
	}
      }
      temp = temp->next;
    }
  }
  if (options.thread_fun != THREAD_LIST_NULL) {
    thread_list temp;
    temp = options.thread_fun;
    while (temp != THREAD_LIST_NULL) {
      if (temp->thread == tr->thread) break;
      temp = temp->next;
    }
    if (temp == THREAD_LIST_NULL) return FALSE;
  } else if (options.function != GENERAL_SLICE_LIST_NULL) {
    return FALSE;
  }


  if (options.function != GENERAL_SLICE_LIST_NULL) {
    general_slice_list temp;
    temp = options.function;
    while (temp != GENERAL_SLICE_LIST_NULL) {
      if (codecmp(temp->t.end_type, temp->t.end, tr->type, tr->code)) {
	if (temp->t.end_param_active == FALSE) {
	  filter_del_thread_fun(tr->thread);
	}
	else {
	  if (temp->t.end_param == tr->args[0])
	    filter_del_thread_fun(tr->thread);
	}
      }
      temp = temp->next;
    }
  }
  return TRUE;
}
