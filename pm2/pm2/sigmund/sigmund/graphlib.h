#ifndef CRAPHLIB_H
#define GRAPHLIB_H

#include "sigmund.h"

typedef struct thread_on_list_st {
  int thread;
  int disactivated;
  struct thread_on_list_st *next;
} * thread_on_list;

typedef struct lwp_on_list_st {
  int lwp;
  u_64 last_up;
  struct lwp_on_list_st *next;
} * lwp_on_list;

extern void set_lwp_last_up(int lwp, u_64 last_up);

extern u_64 get_lwp_last_up(int lwp);

extern void set_thread_disactived(int thread, int deactived);

extern int get_thread_disactivated(int thread);

#endif
