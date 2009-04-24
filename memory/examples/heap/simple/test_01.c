#include <stdio.h>

#if !defined(MM_HEAP_ENABLED)
int main(int argc, char *argv[]) {
  fprintf(stderr, "This application needs 'Heap allocator' to be enabled\n");
}
#else

#include <marcel.h>
#include <numaif.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#define DOWN 2
#define NB_THREADS 16
#define SIZE 2*MA_CACHE_SIZE

marcel_bubble_t b0;

marcel_t *threads;
marcel_barrier_t barrier;
marcel_barrier_t startbarrier;
marcel_cond_t cond;
marcel_mutex_t mutex;

tbx_tick_t t1, t2;

int finished = 0;

any_t alloc(any_t arg) {
  void *data;
  int i;
  int id = (intptr_t) arg;

  marcel_fprintf(stderr,"launched for i %d\n", id);

  data = marcel_malloc_customized(SIZE, HIGH_WEIGHT, 1, -1, 0);

  marcel_fprintf(stderr,"thread %p (%d) fait malloc -> data %p\n", MARCEL_SELF, id, data);
  //marcel_see_allocated_memory(&MARCEL_SELF->as_entity);

  /* ready for main */
  marcel_barrier_wait(&barrier);
  //marcel_start_remix();
  marcel_barrier_wait(&startbarrier);

  int load = *marcel_stats_get(MARCEL_SELF, load);

  int sum;
  for (i = 0 ; i < load*10000 ; ++i) {
    if (i % 10000 == 0) {
      //       marcel_fprintf(stderr,".");
      --*marcel_stats_get(MARCEL_SELF, load);
    }
    sum += i;
  }

  /* liberation */
  marcel_free_customized(data);
  marcel_fprintf(stderr,"thread %p (%d) fait free -> data %p\n", MARCEL_SELF, id, data);

  //   marcel_fprintf(stderr,"*\n");
  finished ++;

  if (finished == NB_THREADS) {
    //marcel_stop_remix();
    marcel_cond_signal(&cond);
  }

  marcel_fprintf(stderr,"thread %p (%d) returns and %d are finished\n", MARCEL_SELF, id, finished);
  return NULL;
}

int main(int argc, char *argv[]) {
  int j;
  double temps;

  marcel_init(&argc,argv);
#ifdef PROFILE
  profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

  marcel_barrier_init(&barrier, NULL, NB_THREADS);
  marcel_barrier_init(&startbarrier, NULL, NB_THREADS + 1);
  marcel_cond_init(&cond, NULL);
  marcel_mutex_init(&mutex, NULL);

  /* construction */
  marcel_bubble_init(&b0);

  threads = malloc(NB_THREADS * sizeof(marcel_t));
  /* lancement des threads pour allouer */
  for(j=0 ; j<NB_THREADS ; j++) {
    marcel_attr_t attr;
    marcel_t t = threads[j];
    marcel_attr_init(&attr);
    marcel_attr_setnaturalbubble(&attr, &b0);
    marcel_attr_setid(&attr,0);
    marcel_attr_setprio(&attr,0);
    marcel_attr_setname(&attr,"thread");
    marcel_create(&t, &attr, alloc, (any_t) (intptr_t) (j+1));
    *marcel_stats_get(t, load) = 1000;
  }

  /* lancer la bulle */
  marcel_wake_up_bubble(&b0);

  /* ordonnancement */
  marcel_start_playing();
  //marcel_bubble_badspread(&b0, marcel_topo_level(2,0), 0);

  /* commencer par les threads */
  marcel_fprintf(stderr,"waiting for threads to start\n");
  marcel_barrier_wait(&startbarrier);
  TBX_GET_TICK(t1);

  /* attente de liberation */
  if (finished != NB_THREADS) {
    marcel_cond_wait(&cond, &mutex);
  }
  TBX_GET_TICK(t2);

  temps = TBX_TIMING_DELAY(t1, t2);
  marcel_printf("time = %d usec\n", temps);

  /* destroy */
  marcel_barrier_destroy(&barrier);
  marcel_barrier_destroy(&startbarrier);
  marcel_cond_destroy(&cond);
  marcel_mutex_destroy(&mutex);

#ifdef PROFILE
  profile_stop();
#endif
  return 0;
}

#endif
