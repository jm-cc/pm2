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

//#define DSM_MUTEX_TRACE

// static dsm_mutexattr_t dsm_mutexattr_default ;


void DSM_LRPC_LOCK_threaded_func()
{
  dsm_mutex_t *mutex;
  pm2_completion_t c;
  
  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS,
		  (char *)&mutex, sizeof(dsm_mutex_t *));
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata();
  marcel_mutex_lock(&mutex->mutex);
  pm2_completion_signal(&c);
#ifdef DSM_MUTEX_TRACE
  fprintf(stderr, " sent lock called\n");
#endif
}


void DSM_LRPC_LOCK_func()
{
  pm2_thread_create(DSM_LRPC_LOCK_threaded_func, NULL);
}


void DSM_LRPC_UNLOCK_func(void)
{
  dsm_mutex_t *mutex;
  pm2_completion_t c;

  pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS,
		  (char *)&mutex, sizeof(dsm_mutex_t *));
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata();
  marcel_mutex_unlock(&mutex->mutex);
  pm2_completion_signal(&c);
}


int dsm_mutex_lock(dsm_mutex_t *mutex)
{
#ifdef DSM_MUTEX_TRACE
  fprintf(stderr, " dsm_mutex_lock called\n");
#endif
  if (mutex->owner == dsm_self())
    marcel_mutex_lock(&mutex->mutex);
  else
    {
      pm2_completion_t c;

      pm2_completion_init(&c, NULL, NULL);
      pm2_rawrpc_begin((int)mutex->owner, DSM_LRPC_LOCK, NULL);
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS,
		    (char *)&mutex, sizeof(dsm_mutex_t *));
      pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
      pm2_rawrpc_end();
      pm2_completion_wait(&c);
    }
#ifdef DSM_MUTEX_TRACE
  fprintf(stderr, " got dsm_mutex \n");
#endif
   return 0;
}


int dsm_mutex_unlock(dsm_mutex_t *mutex)
{
#ifdef DSM_MUTEX_TRACE
  fprintf(stderr, " dsm_mutex_unlock called\n");
#endif
  if (mutex->owner == dsm_self())
    marcel_mutex_unlock(&mutex->mutex);
  else
    {
      pm2_completion_t c;

      pm2_completion_init(&c, NULL, NULL);
      pm2_rawrpc_begin((int)mutex->owner, DSM_LRPC_UNLOCK, NULL);
      pm2_pack_byte(SEND_SAFER, RECV_EXPRESS,
		    (char *)&mutex, sizeof(dsm_mutex_t *));
      pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
      pm2_rawrpc_end();
      pm2_completion_wait(&c);
    }
#ifdef DSM_MUTEX_TRACE
  fprintf(stderr, " released dsm_mutex \n");
#endif
  return 0;
}


