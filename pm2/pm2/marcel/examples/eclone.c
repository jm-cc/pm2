

#include "clone.h"

static clone_t clone;

#define N       3
#define BOUCLES 2

any_t master(any_t arg)
{
  int b;
  volatile int i;

  marcel_detach(marcel_self());

  for(b=0; b<BOUCLES; b++) {

    marcel_delay(1000);

    i = 5;

    CLONE_BEGIN(&clone);

    fprintf(stderr,
	    "Je suis en section parallele "
	    "(delta = %lx, i = %d)\n",
	    clone_my_delta(),
	    ++(*((int *)((char *)&i - clone_my_delta()))));

    marcel_delay(1000);

    CLONE_END(&clone);

    fprintf(stderr, "ouf, les esclaves ont termine\n");
  }

  clone_terminate(&clone);

  return NULL;
}

any_t slave(any_t arg)
{
  marcel_detach(marcel_self());

  CLONE_WAIT(&clone);

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  int i;

  marcel_init(&argc, argv);

  clone_init(&clone, N);

  marcel_create(NULL, NULL, master, NULL);

  for(i=0; i<N; i++)
    marcel_create(NULL, NULL, slave, NULL);

  marcel_end();

  return 0;
}
