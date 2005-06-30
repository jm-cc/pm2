#ifndef LWPTHREAD_H
#define LWPTHREAD_H

#define LWPTHREAD_LIST_NULL (lwpthread_list) NULL

struct lwpthread_list_st {
  int lwp;		/* == kernel pid */
  int thread;
  int logic;
  short int cpu;	/* == kernel SMP CPU */
  struct lwpthread_list_st *next;
};

typedef struct lwpthread_list_st * lwpthread_list;

extern lwpthread_list lwpthread;

extern void add_lwp(int lwp, int thread, int logic);

extern int is_lwp(int pid);

extern int lwp_of_thread(int thread);

extern int thread_of_lwp(int lwp);

extern void set_switch(int oldthread, int newthread);

extern void set_cpu(int lwp, short int cpu);

extern short int cpu_of_lwp(int lwp);

extern int number_lwp();

extern int get_next_lwp();

#endif
