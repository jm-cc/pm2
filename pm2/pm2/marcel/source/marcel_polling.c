
#include "marcel.h"

static marcel_lock_t __polling_lock = MARCEL_LOCK_INIT;

static poll_struct_t poll_structs[MAX_POLL_IDS];
static unsigned nb_poll_structs = 0;

poll_struct_t *__polling_tasks = NULL;

// Checks to see if some polling jobs should be done. NOTE: The
// function assumes that:
//   1) "lock_task()" was called previously ;
//   2) __polling_tasks != NULL
int __marcel_check_polling(unsigned polling_point)
{
  int waked_some_task = 0;
  poll_struct_t *ps, *p;

  mdebug("marcel_check_polling start\n");
  if(marcel_lock_tryacquire(&__polling_lock)) {

    ps = __polling_tasks;
    do {
      if(ps->polling_points & polling_point) {
	register poll_cell_t *cell;
#ifdef SMP
	__lwp_t *cur_lwp = marcel_self()->lwp;
#endif

	if(ps->nb_cells == 1 && ps->fastfunc) {
	  ps->cur_cell = ps->first_cell;
	  cell = ((poll_cell_t *)(*ps->fastfunc)(ps, ps->first_cell->arg, FALSE) ?
		  ps->first_cell : MARCEL_POLL_FAILED);
	}
	else
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
	  ps->nb_cells--;

	  if(!ps->nb_cells) {
	    /* Il faut retirer la tache de __polling_task */
	    if((p = ps->prev) == ps) {
	      __polling_tasks = NULL;
	      break;
	    } else {
	      if(ps == __polling_tasks)
		__polling_tasks = p;
	      p->next = ps->next;
	      p->next->prev = p;
	    }
	  } else {
	    /* S'il reste au moins 2 requetes ou s'il n'y a pas de
	       "fast poll", alors il faut factoriser. */
	    if(ps->nb_cells > 1 || !ps->fastfunc) {
	      mdebug("Factorizing polling");
	      (*(ps->gfunc))((marcel_pollid_t)ps);
	    }
	  }
	}
      }
      ps = ps->next;
    } while(ps != __polling_tasks);

    marcel_lock_release(&__polling_lock);
  }

  mdebug("marcel_check_polling end\n");
  return waked_some_task;
}

marcel_pollid_t marcel_pollid_create(marcel_pollgroup_func_t g,
				     marcel_poll_func_t f,
				     marcel_fastpoll_func_t h,
				     unsigned polling_points)
{
  marcel_pollid_t id;

  //LOG_IN();
  lock_task();

  if(nb_poll_structs == MAX_POLL_IDS) {
    unlock_task();
    RAISE(CONSTRAINT_ERROR);
  }

  id = &poll_structs[nb_poll_structs++];

  unlock_task();

  id->first_cell = id->cur_cell = NULL;
  id->nb_cells = 0;
  id->gfunc = g;
  id->func = f;
  id->fastfunc = h;
  id->polling_points = polling_points | MARCEL_POLL_AT_IDLE;

  mdebug("registering pollid %p (gr=%p, func=%p, fast=%p, pts=%x)\n",
	 id, g, f, h, id->polling_points);

  //LOG_OUT();
  return id;
}

void marcel_poll(marcel_pollid_t id, any_t arg)
{
  poll_cell_t cell;

  //LOG_IN();
  mdebug("Marcel_poll (thread %p)...\n", marcel_self());
  mdebug("using pollid %p (gr=%p, func=%p, fast=%p, pts=%x)\n",
	 id, id->gfunc, id->func, id->fastfunc, id->polling_points);

  lock_task();

  marcel_lock_acquire(&__polling_lock);

  if(id->fastfunc) {
    mdebug("Using Immediate FastPoll %p\n", id->fastfunc);
    id->cur_cell = &cell;
    if((*id->fastfunc)(id, arg, TRUE) != MARCEL_POLL_FAILED) {
      mdebug("Fast Poll completed ok!\n");
      marcel_lock_release(&__polling_lock);
      unlock_task();
      return;
    }
  }

  cell.task = marcel_self();
  cell.blocked = TRUE;
  cell.arg = arg;

  /* Insertion dans la liste des taches sur le meme id */
  cell.prev = NULL;
  if((cell.next = id->first_cell) != NULL)
    cell.next->prev = &cell;
  id->first_cell = &cell;
  id->nb_cells++;

  /* Si pas de "fast poll" ou si au moins 2 requetes de poll, il faut
     factoriser... */
  if(!id->fastfunc || id->nb_cells > 1) {
    mdebug("Factorizing polling");
    (*(id->gfunc))(id);
  }

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

  //LOG("marcel_poll give hand");
  marcel_give_hand(&cell.blocked, &__polling_lock);
  //LOG_OUT();
}



