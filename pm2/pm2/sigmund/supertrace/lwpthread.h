#ifndef LWPTHREAD_H
#define LWPTHREAD_H

#define LWPTHREAD_LIST_NULL (lwpthread_list) NULL

struct lwpthread_list_st {
  int lwp;
  int thread;
  int logic;
  short int cpu;
  struct lwpthread_list_st *next;
};

typedef struct lwpthread_list_st * lwpthread_list;

extern lwpthread_list lwpthread;

void add_lwp(int lwp, int thread, int logic);

int is_lwp(int pid);

int lwp_of_thread(int thread);

int thread_of_lwp(int lwp);

void set_switch(int oldthread, int newthread);

void set_cpu(int lwp, short int cpu);

short int cpu_of_lwp(int lwp);

#endif
