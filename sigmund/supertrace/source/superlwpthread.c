#include "superlwpthread.h"
#include <assert.h>
#include <stdlib.h>

lwpthread_list lwpthread;      // List of lwpthread (correspondance between threads and lwps)


/* add another lwp in the lwpthread list */
void add_lwp(int lwp, int thread, int logic)
{
  lwpthread_list tmp;
  //  printf("Adding lwp=%d thread=%d\n", lwp, thread);
  tmp = (lwpthread_list) malloc(sizeof(struct lwpthread_list_st));
  assert(tmp != NULL);
  tmp->thread = thread;
  tmp->lwp = lwp;
  tmp->next = lwpthread;
  tmp->logic = logic;
  tmp->cpu = 0;
  lwpthread = tmp;
}

/* return 1 if a pid is associated to a lwp
   contained in the lwpthread list */
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

/* return the lwp associated to a  thread,
   and -1 if the thread does not exists */
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

/* return the thread associated to a lwp,
   and -1 if the lwp does not exists */
int thread_of_lwp(int lwp)
{
  lwpthread_list tmp;
  tmp = lwpthread;
  while(tmp != LWPTHREAD_LIST_NULL) {
    if (tmp->lwp == lwp) return tmp->thread;
    tmp = tmp->next;
  }
  return -1;
}


/* switch for the user mode, no lwp changing */
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

/* switch in the kernel */
void set_cpu(int lwp, short int cpu)
{
  lwpthread_list tmp;
  tmp = lwpthread;
  //  printf("Set cpu of %d to %d\n", lwp, cpu);
  while(tmp != LWPTHREAD_LIST_NULL) {
    if (tmp->lwp == lwp) {tmp->cpu = cpu; return;}
    tmp = tmp->next;
  }
  return ; //Erreur
}

/* return the cpu associated to a lwp,
   and -1 if the lwp does not exists */
short int cpu_of_lwp(int lwp)
{
  lwpthread_list tmp;
  tmp = lwpthread;
  while(tmp != LWPTHREAD_LIST_NULL) {
    if (tmp->lwp == lwp) return tmp->cpu;
    tmp = tmp->next;
  }
  return -1;
}


/* returns the number of threads in the twpthread_list */
int number_lwp()
{
  int i = 0;
  lwpthread_list tmp;
  tmp = lwpthread;
  while(tmp != LWPTHREAD_LIST_NULL) {
    i++;
    tmp = tmp->next;
  }
  return i;
}


/* return the next lwp number, 
   and -1 if there is no next lwp left */
int get_next_lwp()
{
  lwpthread_list tmp;
  int lwp;
  tmp = lwpthread;
  if (lwpthread == NULL) return -1;
  lwpthread = lwpthread->next;
  lwp = tmp->lwp;
  free(tmp);
  return lwp;
}
