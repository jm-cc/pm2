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

#include "pm2.h"
#include "marcel.h"
#include "dsm_page_manager.h"
#include "dsm_rpc.h"
#include "dsm_mutex.h"


static dsm_mutexattr_t dsm_mutexattr_default ;


int dsm_mutexattr_setowner(dsm_mutexattr_t *attr, dsm_node_t owner)
{
  attr->owner = owner;
  return 0;
}


int dsm_mutexattr_getowner(dsm_mutexattr_t *attr, dsm_node_t *owner)
{
  *owner = attr->owner;
  return 0;
}


int dsm_mutex_init(dsm_mutex_t *mutex, dsm_mutexattr_t *attr)
{
  if(!attr)
    mutex->owner = 0;
  else
    mutex->owner = attr->owner;
  marcel_mutex_init(&mutex->mutex, NULL);
  return 0;
}


BEGIN_SERVICE(DSM_LRPC_LOCK)
     marcel_mutex_lock(&req.mutex->mutex);
END_SERVICE(DSM_LRPC_LOCK)


BEGIN_SERVICE(DSM_LRPC_UNLOCK)
     marcel_mutex_unlock(&req.mutex->mutex);
END_SERVICE(DSM_LRPC_LOCK)


int dsm_mutex_lock(dsm_mutex_t *mutex)
{
  if (mutex->owner == dsm_self())
    marcel_mutex_lock(&mutex->mutex);
  else
    {
      LRPC_REQ(DSM_LRPC_LOCK) req;
      req.mutex = mutex;
      pm2_rpc(mutex->owner, DSM_LRPC_LOCK, NULL, &req, NULL);
    }
   return 0;
}


int dsm_mutex_unlock(dsm_mutex_t *mutex)
{
  if (mutex->owner == dsm_self())
    marcel_mutex_unlock(&mutex->mutex);
  else
    {
      LRPC_REQ(DSM_LRPC_UNLOCK) req;
      req.mutex = mutex;
      pm2_rpc(mutex->owner, DSM_LRPC_UNLOCK, NULL, &req, NULL);
    }
  return 0;
}


