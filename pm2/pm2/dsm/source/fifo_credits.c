
//#define FIFO_DEBUG

#include "marcel.h" // tmalloc, marcel_sem ops
#include "fifo_credits.h"


void fifo_init(fifo_t *fifo, int fifo_size)
{
  if ((fifo->head = fifo->end = (fifo_item_t *)tmalloc(fifo_size * sizeof(fifo_item_t))) == NULL)
    RAISE(STORAGE_ERROR);
  fifo->start = NULL;
  marcel_sem_init(&fifo->sem, fifo_size);
  fifo->size = fifo_size;
}

   /* 
      fifo_set_next and fifo_get_next are called within a critical
      section which guarantees that there can be no concurrent calls
      to these functions. Therefore, there is no need for additional
      synchronization.  */

void fifo_set_next(fifo_t *fifo, dsm_node_t node)
{
#ifdef FIFO_DEBUG
  tfprintf(stderr, "fifo_set_next: start\n");
#endif
  marcel_sem_P(&fifo->sem);

#ifdef FIFO_DEBUG
  tfprintf(stderr, "fifo_set_next: P\n");
#endif
  /*
    Here we know there is at least one case available in the array.
  */
  if (fifo->end == NULL)
    RAISE(PROGRAM_ERROR);

  *(fifo->end) = node;

  if (fifo->start == NULL)
    fifo->start = fifo->end;

  if (fifo->end >= fifo->head + fifo->size - 1)
    fifo->end = fifo->head;
  else
    fifo->end++;

  if (fifo->end == fifo->start) /* the fifo is now full */
    fifo->end = NULL;
#ifdef FIFO_DEBUG
  tfprintf(stderr, "fifo_set_next: end\n");
#endif
}


dsm_node_t fifo_get_next(fifo_t *fifo)
{
  dsm_node_t node;

#ifdef FIFO_DEBUG
  tfprintf(stderr, "fifo_get_next: start\n");
#endif
  if (fifo->start == NULL)
    {
#ifdef FIFO_DEBUG
      tfprintf(stderr, "fifo_get_next: empty queue\n");
#endif
      return NO_NODE;
    }

  if (fifo->end == NULL)
    fifo->end = fifo->start;

  node = *(fifo->start);
 
  if (fifo->start>= fifo->head + fifo->size - 1)
    fifo->start = fifo->head;
  else
    fifo->start++;  

  if (fifo->end == fifo->start) /* the fifo is now empty */
    fifo->start = NULL; 

  marcel_sem_V(&fifo->sem);
#ifdef FIFO_DEBUG
  tfprintf(stderr, "fifo_get_next: end\n");
#endif
  return node;
}


void fifo_exit(fifo_t *fifo)
{
  tfree(fifo->head);
  fifo->start = fifo->end = NULL;
}

