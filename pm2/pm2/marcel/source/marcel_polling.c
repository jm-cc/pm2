/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: marcel_polling.c,v $
Revision 1.2  2000/04/11 09:07:35  rnamyst
Merged the "reorganisation" development branch.

Revision 1.1.2.1  2000/04/10 09:57:10  rnamyst
Moved the polling stuff into marcel_polling...

______________________________________________________________________________
*/

#include "marcel.h"

static marcel_lock_t __polling_lock = MARCEL_LOCK_INIT;

static poll_struct_t poll_structs[MAX_POLL_IDS];
static unsigned nb_poll_structs = 0;

static poll_struct_t *__polling_tasks = NULL;

int marcel_polling_is_required(void)
{
  return __polling_tasks != NULL;
}

// Checks to see if some polling jobs should be done. NOTE: The
// function assumes that "lock_task()" was called previously.
int marcel_check_polling(void)
{
  int waked_some_task = 0;
  poll_struct_t *ps, *p;

  if(marcel_lock_tryacquire(&__polling_lock)) {

    /* Polling tasks */
    ps = __polling_tasks;
    if(ps != NULL)
      do {
	if(++ps->count == ps->divisor) {
	  register poll_cell_t *cell;
#ifdef SMP
	  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

	  ps->count = 0;
	  cell = (poll_cell_t *)(*ps->func)(ps,
					    SCHED_DATA(cur_lwp).running_tasks,
					    marcel_sleepingthreads(),
					    marcel_blockedthreads());
	  if(cell != MARCEL_POLL_FAILED) {

	    waked_some_task = 1;
	    marcel_wake_task(cell->task, &cell->blocked);

	    /* Retrait de d'un élément de la liste */
	    if(cell->prev != NULL)
	      cell->prev->next = cell->next;
	    else
	      ps->first_cell = cell->next;
	    if(cell->next != NULL)
	      cell->next->prev = cell->prev;

	    if(ps->first_cell != NULL) {
	      /* S'il reste des éléments, alors il faut re-factoriser */
	      (*(ps->gfunc))((marcel_pollid_t)ps);
	    } else {
	      /* Sinon, il faut retirer la tache de __polling_task */
	      if((p = ps->prev) == ps) {
		__polling_tasks = NULL;
		break;
	      } else {
		if(ps == __polling_tasks)
		  __polling_tasks = p;
		p->next = ps->next;
		p->next->prev = p;
	      }
	    }
	  }
	}
	ps = ps->next;
      } while(ps != __polling_tasks);

    marcel_lock_release(&__polling_lock);
  }

  return waked_some_task;
}

marcel_pollid_t marcel_pollid_create(marcel_pollgroup_func_t g,
				     marcel_poll_func_t f,
				     int divisor)
{
  marcel_pollid_t id;

  lock_task();
  if(nb_poll_structs == MAX_POLL_IDS) {
    unlock_task();
    RAISE(CONSTRAINT_ERROR);
  }
  id = &poll_structs[nb_poll_structs++];
  unlock_task();

  id->first_cell = id->cur_cell = NULL;
  id->gfunc = g;
  id->func = f;
  id->divisor = divisor;
  id->count = 0;

  return id;
}

void marcel_poll(marcel_pollid_t id, any_t arg)
{
  poll_cell_t cell;

  lock_task();

  cell.task = marcel_self();
  cell.blocked = TRUE;
  cell.arg = arg;

  marcel_lock_acquire(&__polling_lock);

  /* Insertion dans la liste des taches sur le meme id */
  cell.prev = NULL;
  if((cell.next = id->first_cell) != NULL)
    cell.next->prev = &cell;
  id->first_cell = &cell;

  /* Factorisation "utilisateur" du polling */
  (*(id->gfunc))(id);

  /* Enregistrement éventuel dans la __polling_tasks */
  if(cell.next == NULL) {
    /* Il n'y avait pas encore un polling de ce type */
    if(__polling_tasks == NULL) {
      __polling_tasks = id;
      id->next = id->prev = id;
    } else {
      id->next = __polling_tasks;
      id->prev = __polling_tasks->prev;
      id->prev->next = __polling_tasks->prev = id;
    }
  }

  marcel_give_hand(&cell.blocked, &__polling_lock);
}



