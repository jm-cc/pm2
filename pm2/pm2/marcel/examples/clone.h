
#ifndef CLONE_IS_DEF
#define CLONE_IS_DEF

#include "marcel.h"

typedef struct {
  marcel_mutex_t mutex;
  marcel_cond_t master, slave;
  int nb, nbs, total, terminate;
  jmp_buf master_jb;
  marcel_t master_pid;
} clone_t;

typedef struct {
  jmp_buf buf;
} slave_data;

void clone_init(clone_t *c, int nb_slaves);

void clone_slave(clone_t *c); /* Appele par chaque "esclave" */

void clone_slave_ends(clone_t *c); /* Appele par chaque "esclave" */

void clone_master(clone_t *c); /* Appele par le thread scalaire */

void clone_master_ends(clone_t *c); /* Appele par le thread scalaire */

void clone_terminate(clone_t *c); /* Appele par le thread scalaire */

extern marcel_key_t _clone_key, _slave_key;

#define clone_my_delta()   ((long)marcel_getspecific(_clone_key))

#define CLONE_END(c) if(clone_my_delta() == 0) \
                        clone_master_ends(c); \
                     else \
		        clone_slave_ends(c)

#define my_data()  ((slave_data *)marcel_getspecific(_slave_key))

#define CLONE_BEGIN(c) \
   marcel_setspecific(_clone_key, NULL); \
   (c)->master_pid = marcel_self(); \
   if(setjmp((c)->master_jb) == 0) \
     clone_master(c);


#define CLONE_WAIT(c) \
   marcel_setspecific(_slave_key, (any_t)tmalloc(sizeof(slave_data))), \
   setjmp(my_data()->buf), \
   clone_slave(c)

#endif
