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
#define PIOM_LFQUEUE_TYPE(ENAME, TYPE, LFQUEUE_NULL, LFQUEUE_SIZE)	\
  typedef TYPE ENAME ## _lfqueue_elem_t;				\
  									\
  struct ENAME ## _lfqueue_s						\
  {									\
    volatile ENAME ## _lfqueue_elem_t _queue[(LFQUEUE_SIZE)];		\
    volatile unsigned _head;						\
    volatile unsigned _tail;						\
  };									\
									\
  static inline void ENAME ## _lfqueue_init(struct ENAME ## _lfqueue_s*queue) \
  {									\
    int i;								\
    for(i = 0; i < (LFQUEUE_SIZE); i++)					\
      {									\
	queue->_queue[i] = (LFQUEUE_NULL);				\
      }									\
    queue->_head = 0;							\
    queue->_tail = 0;							\
  }									\
  									\
  static inline int ENAME ## _lfqueue_enqueue_ext(struct ENAME ## _lfqueue_s*queue, ENAME ## _lfqueue_elem_t e, int single) \
  {									\
    /* allocate a slot for value */					\
  retry:								\
    ;									\
    const unsigned head = queue->_head;					\
    const unsigned tail = queue->_tail;					\
    if(head >= tail + (unsigned)(LFQUEUE_SIZE) - 1)			\
      {									\
	/* queue full */						\
	return -1;							\
      }									\
    if(single)								\
      {									\
	queue->_head = head + 1;					\
      }									\
    else								\
      if(!__sync_bool_compare_and_swap(&queue->_head, head, head + 1))	\
	goto retry;							\
    /* slot is still NULL for concurrent readers, already reserved if concurrent writers */ \
    const unsigned cell = head % (unsigned)(LFQUEUE_SIZE);		\
    /* store value in reserved slot */					\
    assert(queue->_queue[cell] == (LFQUEUE_NULL));			\
    queue->_queue[cell] = e;						\
    return 0;								\
  }									\
									\
  static inline int ENAME ## _lfqueue_enqueue(struct ENAME ## _lfqueue_s*queue, ENAME ## _lfqueue_elem_t e) \
  { return ENAME ## _lfqueue_enqueue_ext(queue, e, 0); }		\
  									\
  static inline int ENAME ## _lfqueue_enqueue_single_writer(struct ENAME ## _lfqueue_s*queue, ENAME ## _lfqueue_elem_t e) \
  { return ENAME ## _lfqueue_enqueue_ext(queue, e, 1); }		\
  									\
  static inline ENAME ## _lfqueue_elem_t ENAME ## _lfqueue_dequeue_ext(struct ENAME ## _lfqueue_s*queue, int single) \
  {									\
    ENAME ## _lfqueue_elem_t e = (LFQUEUE_NULL);			\
    unsigned tail;							\
    /* try to dequeue */						\
  retry:								\
    tail = queue->_tail;						\
    if(tail >= queue->_head)						\
      {									\
	/* queue was empty, abort dequeue */				\
	return e;							\
      }									\
    if(single)								\
      { 								\
	queue->_tail = tail + 1;					\
      }									\
    else								\
      if(!__sync_bool_compare_and_swap(&queue->_tail, tail, tail + 1))	\
	goto retry;							\
    const unsigned cell = tail % (LFQUEUE_SIZE);			\
    e = queue->_queue[cell];						\
    while(e == (LFQUEUE_NULL)) 						\
      { __sync_synchronize(); e = queue->_queue[cell]; }		\
    queue->_queue[cell] = (LFQUEUE_NULL);				\
    assert(e != (LFQUEUE_NULL));					\
    return e;								\
  }									\
  									\
  static inline ENAME ## _lfqueue_elem_t ENAME ## _lfqueue_dequeue(struct ENAME ## _lfqueue_s*queue) \
  { return ENAME ## _lfqueue_dequeue_ext(queue, 0); }			\
									\
  static inline ENAME ## _lfqueue_elem_t ENAME ## _lfqueue_dequeue_single_reader(struct ENAME ## _lfqueue_s*queue) \
  { return ENAME ## _lfqueue_dequeue_ext(queue, 1); }			\
									\
  /** hint whether the queue is empty.					\
   *  Do not rely on it: state may have changed before dequeue		\
   *  Usefull to reduce active polling, though */		       	\
  static inline int ENAME ## _lfqueue_empty(struct ENAME ## _lfqueue_s*queue) \
  {									\
    return(queue->_tail < queue->_head);				\
  }

#endif /* PIOM_LFQUEUE_H */

