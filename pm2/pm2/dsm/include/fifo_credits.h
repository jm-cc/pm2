
#ifndef FIFO_CREDITS_IS_DEF
#define FIFO_CREDITS_IS_DEF

#include "dsm_const.h" /* access_t */

typedef dsm_node_t fifo_item_t;

typedef struct s_fifo
{
  fifo_item_t *head;
  fifo_item_t *start;
  fifo_item_t *end;
  marcel_sem_t sem;
  int size;
} fifo_t;

void fifo_init(fifo_t *fifo, int fifo_size);

void fifo_set_next(fifo_t *fifo, dsm_node_t node);

dsm_node_t fifo_get_next(fifo_t *fifo);

void fifo_exit(fifo_t *fifo);

#endif

