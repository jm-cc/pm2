#include <marcel_np.h>

/*  #define DEBUG */


extern marcel_mutex_t marcel_end_lock_np;
extern marcel_cond_t marcel_end_cond_np;
extern int marcel_end_np_np;

void marcel_create_np(void *(*func)(void)) {
  marcel_t t;
  marcel_attr_t attr;

  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
  /* marcel_attr_setvp_np(&attr, MARCEL_BEST_VP); */
  marcel_create(&t, &attr, (marcel_func_t)func, NULL);
}

void marcel_wait_for_end_np(int p)
{
  marcel_mutex_lock(&marcel_end_lock_np);
  while (marcel_end_np_np < p)
     marcel_cond_wait(&marcel_end_cond_np, &marcel_end_lock_np); 
  marcel_mutex_unlock(&marcel_end_lock_np);
}


void marcel_signal_end_np(int self, int p)
{
    marcel_mutex_lock(&marcel_end_lock_np); 
    marcel_end_np_np++;
#ifdef DEBUG2
  printf("signal end, I am %p, #threads = %d\n",marcel_self(), marcel_end_np_np);
#endif
     marcel_mutex_unlock(&marcel_end_lock_np); 
     marcel_cond_signal(&marcel_end_cond_np); 
}

void marcel_barinit_np(marcel_bar_t_np *bar)
{
  marcel_sem_init(&bar->mutex, 1);
  marcel_sem_init(&bar->wait, 0);
  bar->nb = 0;
}

void marcel_barrier_np(marcel_bar_t_np *bar, int p)
{
  marcel_sem_P(&bar->mutex);
  if(++bar->nb == p) {
    marcel_sem_unlock_all(&bar->wait);
    bar->nb = 0;
    marcel_sem_V(&bar->mutex);
  } else {
    marcel_sem_VP(&bar->mutex, &bar->wait);
  }
}
