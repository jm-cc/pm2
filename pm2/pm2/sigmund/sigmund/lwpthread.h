#ifndef LWPTHREAD_H
#define LWPTHREAD_H

extern void filter_add_lwp(int lwp, int logic, int thread, int active, short int cpu);

extern int is_active_lwp_of_thread(int thread);

extern int is_active_lwp(int lwp);

extern int lwp_of_thread(int thread);

extern void change_lwp_thread(int oldthread, int newthread);

extern int is_lwp(int lwp);

extern void set_active_lwp(int lwp, int active);

extern int thread_of_lwp(int lwp);

extern int logic_of_lwp(int lwp);

extern void set_cpu_lwp(int lwp, short int cpu);

extern short int get_cpu_of_thread(int thread);

#endif
