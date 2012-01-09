/** @file lock-free FIFO as circular array
 * @author Alexandre DENIS
 * @note code copied from Puk to avoid a dependancy
 */

#ifndef PIOM_LFQUEUE_H
#define PIOM_LFQUEUE_H

/** builds a lock-free FIFO type and functions.
 *  ENAME is base name for symbols
 *  TYPE is the type of elements in queue- must be atomic (1, 2, 4, or 8 bytes long)
 *  LFQUEUE_NULL is a nil value to fill and detect empty cells
 *  LFQUEUE_SIZE is the array size
 *  Only 'init', 'enqueue', 'dequeue' operations are available.
 *  Others operations such as 'empty', 'size' are useless for lock-free
 *  Iterators are not possible.
 *  push_front/push_back/po_front/pop_back is probably possible at a high
 *  cost in code size and performance. For simplicity and performance, only
 *  'enqueue' = push_back; 'dequeue' = pop_front are available (2 atomic ops each).
 */
#define PIOM_LFQUEUE_TYPE(ENAME, TYPE, LFQUEUE_NULL, LFQUEUE_SIZE)       \
  typedef TYPE ENAME ## _lfqueue_elem_t;                                 \
                                                                        \
  struct ENAME ## _lfqueue_s                                            \
  {                                                                     \
    ENAME ## _lfqueue_elem_t _queue[LFQUEUE_SIZE];                      \
    volatile unsigned _head;                                            \
    volatile unsigned _tail;                                            \
  };                                                                    \
                                                                        \
  static inline void ENAME ## _lfqueue_init(struct ENAME ## _lfqueue_s*queue)   \
  {                                                                     \
    int i;                                                              \
    for(i = 0; i < LFQUEUE_SIZE; i++)                                   \
      {                                                                 \
        queue->_queue[i] = LFQUEUE_NULL;                                \
      }                                                                 \
    queue->_head = 0;                                                   \
    queue->_tail = 0;                                                   \
  }                                                                     \
                                                                        \
  static inline int ENAME ## _lfqueue_enqueue(struct ENAME ## _lfqueue_s*queue, ENAME ## _lfqueue_elem_t e) \
  {                                                                     \
    /* allocate a slot for value */                                     \
    const unsigned head = __sync_fetch_and_add(&queue->_head, 1);       \
    const unsigned guard = 8;                                           \
    const unsigned tail = queue->_tail;                                 \
    if(head > tail + LFQUEUE_SIZE - guard)                              \
      {                                                                 \
        /* queue full */                                                \
        __sync_fetch_and_sub(&queue->_head, 1);                         \
        return -1;                                                      \
      }                                                                 \
    /* slot is still NULL for concurrent readers, already reserved if concurrent writers */ \
    const unsigned cell = head % LFQUEUE_SIZE;                          \
    /* store value in reserved slot */                                  \
    __sync_val_compare_and_swap(&queue->_queue[cell], LFQUEUE_NULL, e); \
    return 0;                                                           \
  }                                                                     \
                                                                        \
  static inline ENAME ## _lfqueue_elem_t ENAME ## _lfqueue_dequeue(struct ENAME ## _lfqueue_s*queue) \
  {                                                                     \
    ENAME ## _lfqueue_elem_t e = LFQUEUE_NULL;                          \
    /* try to dequeue */                                                \
    const unsigned tail = __sync_fetch_and_add(&queue->_tail, 1);       \
    if(tail >= queue->_head)                                            \
      {                                                                 \
        /* queue was empty, abort dequeue */                            \
        __sync_fetch_and_sub(&queue->_tail, 1);                         \
        return e;                                                       \
      }                                                                 \
    const unsigned cell = tail % LFQUEUE_SIZE;                          \
    e = __sync_lock_test_and_set(&queue->_queue[cell], LFQUEUE_NULL);   \
    if(e == LFQUEUE_NULL)                                               \
      {                                                                 \
        /* queue was empty with a concurrent enqueue, abort dequeue */  \
        __sync_fetch_and_sub(&queue->_tail, 1);                         \
      }                                                                 \
    return e;                                                           \
}

#endif /* PIOM_LFQUEUE_H */

