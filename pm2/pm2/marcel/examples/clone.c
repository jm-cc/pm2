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
*/


#include "clone.h"
#include <sys/archdep.h>

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
  jmp_buf buf;

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
		     (any_t)((long)marcel_stackbase(marcel_self()) -
			     (long)marcel_stackbase(c->master_pid)));

  memcpy(&buf, &c->master_jb, sizeof(jmp_buf));

  (long)SP_FIELD(buf) = (long)SP_FIELD(buf) + clone_my_delta();
#ifdef FP_FIELD
  (long)FP_FIELD(buf) = (long)FP_FIELD(buf) + clone_my_delta();
#endif

  marcel_mutex_unlock(&c->mutex);

  longjmp(buf, 1);
}

void clone_slave_ends(clone_t *c)
{
  marcel_mutex_lock(&c->mutex);

  if(++c->nbs == c->total - 1) {
    marcel_cond_signal(&c->master);
  }

  marcel_mutex_unlock(&c->mutex);

  longjmp(my_data()->buf, 1);
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
