
#include <marcel.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef struct {
  marcel_sem_t mutex, wait;
  int nb;
} marcel_bar_t_np;

extern void marcel_barinit_np(marcel_bar_t_np *bar);

extern void marcel_create_np(void *(*func)(void));

extern void marcel_wait_for_end_np(int p);

extern void marcel_signal_end_np(int self, int p);

extern void marcel_barrier_np(marcel_bar_t_np *bar, int p);
