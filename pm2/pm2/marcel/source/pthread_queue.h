/* Linuxthreads - a simple clone()-based implementation of Posix        */
/* threads for Linux.                                                   */
/* Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)              */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU Library General Public License for more details.                 */

/* Waiting queues */

/* Waiting queues are represented by lists of thread descriptors
   linked through their p_nextwaiting field.  The lists are kept
   sorted by decreasing priority, and then decreasing waiting time. */
#define ASSERT(expr) (void)(0) //assert(expr) //VD:

static inline void enqueue(marcel_descr * q, marcel_descr th)
{
  //VD:int prio = th->p_priority;
  ASSERT(th->p_nextwaiting == NULL);
  for (; *q != NULL; q = &((*q)->p_nextwaiting)) {
    //VD:if (prio > (*q)->p_priority) {
      th->p_nextwaiting = *q;
      *q = th;
      return;
      //VD:}
  }
  *q = th;
}

static inline marcel_descr dequeue(marcel_descr * q)
{
  marcel_descr th;
  th = *q;
  if (th != NULL) {
    *q = th->p_nextwaiting;
    th->p_nextwaiting = NULL;
  }
  return th;
}

static inline int remove_from_queue(marcel_descr * q, marcel_descr th)
{
  for (; *q != NULL; q = &((*q)->p_nextwaiting)) {
    if (*q == th) {
      *q = th->p_nextwaiting;
      th->p_nextwaiting = NULL;
      return 1;
    }
  }
  return 0;
}

static inline int queue_is_empty(marcel_descr * q)
{
    return *q == NULL;
}
