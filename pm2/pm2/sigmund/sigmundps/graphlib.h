#ifndef CRAPHLIB_H
#define GRAPHLIB_H

#include "sigmund.h"

typedef struct thread_on_list_st {
  int thread;
  int disactivated;
  mode type;
  u_64 last_up; // ?
  int dec;
  struct thread_on_list_st *next;
} * thread_on_list;

typedef struct lwp_on_list_st {
  int lwp;
  u_64 last_up;
  struct lwp_on_list_st *next;
} * lwp_on_list;

extern void set_lwp_last_up(int lwp, u_64 last_up);

extern u_64 get_lwp_last_up(int lwp);

extern void set_thread_disactivated(int thread, int disactivated);

extern int get_thread_disactivated(int thread);

extern mode get_last_type_thread(int thread);

extern void set_last_type_thread(int thread, mode type);

extern void set_thread_last_up(int thread, u_64 last_up);

extern u_64 get_thread_last_up(int thread);

extern void add_thread_dec(int thread, int add);

extern int get_thread_dec(int thread);

#endif
