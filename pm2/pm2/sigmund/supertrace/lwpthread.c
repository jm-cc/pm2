#include "lwpthread.h"
#include "assert.h"

lwpthread_list lwpthread;

void add_lwp(int lwp, int thread, int logic)
{
  lwpthread_list tmp;
  tmp = (lwpthread_list) malloc(sizeof(struct lwpthread_list_st));
  assert(tmp != NULL);
  tmp->thread = thread;
  tmp->lwp = lwp;
  tmp->next = lwpthread;
  tmp->logic = logic;
  lwpthread = tmp;
}

int is_lwp(int pid)
{
  lwpthread_list tmp;
  tmp = lwpthread;
  while(tmp != LWPTHREAD_LIST_NULL) {
    if (tmp->lwp == pid) return 1;
    tmp = tmp->next;
  }
  return 0;
}

int lwp_of_thread(int thread)
{
  lwpthread_list tmp;
  tmp = lwpthread;
  while(tmp != LWPTHREAD_LIST_NULL) {
    if (tmp->thread == thread) return tmp->lwp;
    tmp = tmp->next;
  }
  return -1;
}

void set_switch(int oldthread, int newthread)
{
  lwpthread_list tmp;
  tmp = lwpthread;
  while(tmp != LWPTHREAD_LIST_NULL) {
    if (tmp->thread == oldthread) tmp->thread = newthread;
    tmp = tmp->next;
  }
  // Erreur
}
