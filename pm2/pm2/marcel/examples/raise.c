
#include "marcel.h"

exception USER_ERROR = "USER_ERROR";

any_t func(any_t arg)
{
   RAISE(USER_ERROR);

   return NULL;
}

int marcel_main(int argc, char *argv[])
{
  fprintf(stderr,
	  "WARNING: this program will deliberately cause a SEGFAULT in a few us...\n");

  marcel_init(&argc, argv);

  marcel_create(NULL, NULL, func, NULL);

  marcel_delay(500);

  return 0;
}

