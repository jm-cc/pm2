
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#ifdef MARCEL_KEYS_ENABLED
#  define MARCEL_INTERNAL_INCLUDE
#  include "clone.h"

marcel_key_t _clone_key, _slave_key;

void clone_init(clone_t *c, int nb_slaves)
{
  marcel_key_create(&_clone_key, NULL);
  marcel_key_create(&_slave_key, NULL);

  marcel_mutex_init(&c->mutex, NULL);
  marcel_cond_init(&c->master, NULL);
  marcel_cond_init(&c->slave, NULL);
  c->total = nb_slaves + 1;
  c->nb = c->nbs = c->terminate = 0;
}

void clone_slave(clone_t *c)
{
  marcel_ctx_t buf;

  marcel_mutex_lock(&c->mutex);

  if(++c->nb != c->total) {
    marcel_cond_wait(&c->slave, &c->mutex);
  } else {
    marcel_cond_broadcast(&c->slave);
    c->nb = 0;
  }

  if(c->terminate) {
    marcel_mutex_unlock(&c->mutex);
    marcel_exit(NULL);
  }

  marcel_setspecific(_clone_key,
		     (any_t)((unsigned long)marcel_stackbase(marcel_self()) -
			     (unsigned long)marcel_stackbase(c->master_pid)));

  memcpy(&buf, &c->master_jb, sizeof(marcel_ctx_t));

  marcel_ctx_get_sp(buf) = marcel_ctx_get_sp(buf) + clone_my_delta();
#  ifdef marcel_ctx_get_fp
  marcel_ctx_get_fp(buf) = marcel_ctx_get_fp(buf) + clone_my_delta();
#  endif
#  ifdef marcel_ctx_get_bsp
  marcel_ctx_get_bsp(buf) = marcel_ctx_get_bsp(buf) + clone_my_delta();
#  endif

  marcel_mutex_unlock(&c->mutex);

  marcel_ctx_setcontext(buf, 1);
}

void clone_slave_ends(clone_t *c)
{
  marcel_mutex_lock(&c->mutex);

  if(++c->nbs == c->total - 1) {
    marcel_cond_signal(&c->master);
  }

  marcel_mutex_unlock(&c->mutex);

  marcel_ctx_setcontext(my_data()->buf, 1);
}

void clone_master(clone_t *c)
{
  marcel_mutex_lock(&c->mutex);

  if(++c->nb != c->total)
    marcel_cond_wait(&c->slave, &c->mutex);
  else {
    marcel_cond_broadcast(&c->slave);
    c->nb = 0;
  }

  marcel_mutex_unlock(&c->mutex);
}

void clone_master_ends(clone_t *c)
{
  marcel_mutex_lock(&c->mutex);

  if(c->nbs != c->total - 1)
    marcel_cond_wait(&c->master, &c->mutex);
  c->nbs = 0;

  marcel_mutex_unlock(&c->mutex);
}

void clone_terminate(clone_t *c)
{
  marcel_mutex_lock(&c->mutex);

  c->terminate = 1;
  marcel_cond_broadcast(&c->slave);

  marcel_mutex_unlock(&c->mutex);
}
#else /* MARCEL_KEYS_ENABLED */
#  warning Marcel keys must be enabled for this program
#endif /* MARCEL_KEYS_ENABLED */
