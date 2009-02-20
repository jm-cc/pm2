#include <marcel.h>

#ifdef MA__BUBBLES

marcel_barrier_t shake_barrier;

static void *
my_shake (void *arg)
{
  /* Wait for the main thread to end its initial thread
     distribution. */
  marcel_barrier_wait (&shake_barrier);

  /* Shake it baby! */
  marcel_bubble_shake ();

  return NULL;
}

int
main (int argc, char **argv)
{
  unsigned int i, nb_vps;
  marcel_attr_t thread_attr;
  marcel_barrierattr_t barrier_attr;

  marcel_init (&argc, argv);

  nb_vps = marcel_nbvps ();
  marcel_t tids[nb_vps];
  marcel_attr_init (&thread_attr);
  marcel_attr_setinitbubble (&thread_attr, &marcel_root_bubble);
  marcel_barrierattr_init (&barrier_attr);
  marcel_barrierattr_setmode (&barrier_attr, MA_BARRIER_YIELD_MODE);
  marcel_barrier_init (&shake_barrier, &barrier_attr, nb_vps + 1);

  for (i = 0; i < nb_vps; i++)
    marcel_create (&tids[i], &thread_attr, my_shake, NULL);

  /* Distribute the threads. */
  marcel_bubble_sched_begin ();

  /* Let the threads shake! */
  marcel_barrier_wait (&shake_barrier);

  for (i = 0; i < nb_vps; i++)
    marcel_join (tids[i], NULL);

  marcel_end ();

  return 0;
}
#else /* MA__BUBBLES */
int main(int argc, char *argv[]) {
   fprintf(stderr,"%s needs bubbles\n",argv[0]);
   return 0;
}
#endif /* MA__BUBBLES */
