
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

#include "pm2.h"
#include "marcel.h"
#include "dsm_page_manager.h"
#include "dsm_rpc.h"
#include "dsm_mutex.h"

//#define DSM_MUTEX_TRACE

// static dsm_mutexattr_t dsm_mutexattr_default ;


void DSM_LRPC_LOCK_threaded_func(void *arg)
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


void DSM_LRPC_LOCK_func(void)
{
  pm2_service_thread_create(DSM_LRPC_LOCK_threaded_func, NULL);
}


void DSM_LRPC_UNLOCK_threaded_func(void *arg)
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

void DSM_LRPC_UNLOCK_func()
{
  pm2_service_thread_create(DSM_LRPC_UNLOCK_threaded_func, NULL);
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
      pm2_rawrpc_begin(mutex->owner, DSM_LRPC_LOCK, NULL);
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
      pm2_rawrpc_begin(mutex->owner, DSM_LRPC_UNLOCK, NULL);
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



