
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


