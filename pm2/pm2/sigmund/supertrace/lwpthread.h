#ifndef LWPTHREAD_H
#define LWPTHREAD_H

#define LWP_THREAD_LIST_NULL (lwpthread_list) NULL

struct lwpthread_list_st {
  int lwp;
  int thread;
  int logic;
  struct lwpthread_list_st *next;
};

typedef struct lwpthread_list_st * lwpthread_list;

extern lwpthread_list lwpthread;

void add_lwp(int lwp, int thread, int logic);

int is_lwp(int pid);

int lwp_of_thread(int thread);

void set_switch(int oldthread, int newthread);

#endif
