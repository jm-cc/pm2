#include "filter.h"
#include "lwpthread.h"
#include "assert.h"

#define TRUE 1
#define FALSE 0

lwp_thread_list lwpthread;


/* each lwp is associated to the threads that are running .*/

void filter_add_lwp(int lwp, int logic, int thread, 
		    int active, short int cpu)
{
  lwp_thread_list tmp;
  tmp = (lwp_thread_list) malloc(sizeof(struct lwp_thread_list_st));
  assert(tmp != NULL);
  tmp->next = lwpthread;
  tmp->lwp = lwp;
  tmp->logic = logic;
  tmp->thread = thread;
  tmp->active = active;
  tmp->cpu = cpu;
  lwpthread = tmp;
}

int is_active_lwp_of_thread(int thread)
{
  lwp_thread_list tmp;
  tmp = lwpthread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->thread == thread) return tmp->active;
    tmp = tmp->next;
  }
  return -1;  // Error
}

int is_active_lwp(int lwp)
{
  lwp_thread_list tmp;
  tmp = lwpthread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->lwp == lwp) return tmp->active;
    tmp = tmp->next;
  }
  return -1; // Error
}

int lwp_of_thread(int thread)
{
  lwp_thread_list tmp;
  tmp = lwpthread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->thread == thread) break;
    tmp = tmp->next;
  }
  if (tmp == LWP_THREAD_LIST_NULL) return -1;
  return tmp->lwp;
}

void change_lwp_thread(int oldthread, int newthread)
{
  lwp_thread_list tmp;
  tmp = lwpthread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->thread == oldthread) break;
    tmp = tmp->next;
  }
  if (tmp == LWP_THREAD_LIST_NULL) return;  //Error
  tmp->thread = newthread;
}

int is_lwp(int lwp)
{
  lwp_thread_list tmp;
  tmp = lwpthread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->lwp == lwp) return TRUE;
    tmp = tmp->next;
  }
  return TRUE;
}

void set_active_lwp(int lwp, int active)
{
  lwp_thread_list tmp;
  tmp = lwpthread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->lwp == lwp) {
      tmp->active = active;
      return;
    }
    tmp = tmp->next;
  }
  return; // Error
}

int thread_of_lwp(int lwp)
{
  lwp_thread_list tmp;
  tmp = lwpthread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->lwp == lwp) return tmp->thread;
    tmp = tmp->next;
  }
  return -1;
}

int logic_of_lwp(int lwp)
{
  lwp_thread_list tmp;
  tmp = lwpthread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->lwp == lwp) return tmp->logic;
    tmp = tmp->next;
  }
  return -1;
}

void set_cpu_lwp(int lwp, short int cpu)
{
  lwp_thread_list tmp;
  tmp = lwpthread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->lwp == lwp) {tmp->cpu = cpu; return;}
    tmp = tmp->next;
  }
  return;  //Error
}

short int get_cpu_of_thread(int thread)
{
  lwp_thread_list tmp;
  tmp = lwpthread;
  while (tmp != LWP_THREAD_LIST_NULL) {
    if (tmp->thread == thread) {return tmp->cpu;}
    tmp = tmp->next;
  }
  return -1;  //Error
}

