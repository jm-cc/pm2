#include "marcel.h"

#ifdef MARCEL_NUMA
#define NB_THREADS 8
#define TAB_SIZE 1024 * 256
#define NB_LOOPS 10000
#define BUBBLE_MODE 0

#if 1
#define debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__);
#else
#define debug(fmt, ...) (void)0
#endif

static int tabs[NB_THREADS / 2][TAB_SIZE];

any_t f(any_t arg)
{
  int i, k;
  int tmp;

  marcel_t self = marcel_self();
  int team;

  team = (self->id)%(NB_THREADS / 2);
  debug("id: %d, team:%d\n", self->id, team);

  for (k = 0; k < NB_LOOPS; k++)
    for (i = 0; i < TAB_SIZE; i++)
      tabs[team][i] = 42;
}

int
marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_bubble_t b[4];
  marcel_t pid[NB_THREADS];
  marcel_attr_t attr;
  int i;

  marcel_trace_on();
  marcel_init(&argc, argv);
  marcel_attr_init(&attr);

  if (BUBBLE_MODE)
    {
      for (i = 0; i < 4; i++)
	{
	  marcel_bubble_init(&b[i]);
	  marcel_bubble_setid(&b[i], i);

	  marcel_bubble_insertbubble(&marcel_root_bubble, &b[i]);
	}
    }

  for (i = 0; i < NB_THREADS; i++)
    {
      if (BUBBLE_MODE)
	{
	  int index = i%(NB_THREADS / 2);

	  marcel_attr_setnaturalbubble(&attr, &b[index]);
	  debug("id = %d, bubble = %d\n", i, index);
	}

      marcel_attr_setid(&attr, i);
      marcel_create(&pid[i], &attr, f, NULL);
    }

  if (BUBBLE_MODE) {
    for (i = 0; i < 4; i++)
      marcel_bubble_submit(&b[i]);
  }
  else
    {
      for (i = 0; i < NB_THREADS; i++)
	marcel_join(pid[i], NULL);
    }

  marcel_end();
  return 0;
}
#else /* MARCEL_NUMA */
#  warning Marcel NUMA mode must be selected for this program
int marcel_main(int argc, char *argv[])
{
  fprintf(stderr,
	  "'marcel NUMA' mode required in the flavor\n");

  return 0;
}
#endif /* MARCEL_NUMA */

