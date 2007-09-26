
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

/* cleanup.c */

#include "marcel.h"

#ifdef MARCEL_CLEANUP_ENABLED
#define STACK_SIZE	10000

char *mess[2] = { "boys", "girls" };

void bye(any_t arg)
{
   tfprintf(stderr, "Bye bye %s!\n", (char*) arg);
}

any_t writer(any_t arg)
{ int i;

   marcel_cleanup_push(bye, arg);

   for(i=0;i<10;i++) {
      tfprintf(stderr, "Hi %s!\n", (char*) arg);
      marcel_delay(50);
   }
   marcel_cleanup_pop(0);
   return (any_t)NULL;
}

int marcel_main(int argc, char *argv[])
{ any_t status;
  marcel_t writer1_pid, writer2_pid;
  marcel_attr_t writer_attr;

   marcel_init(&argc, argv);

   marcel_attr_init(&writer_attr);

   marcel_attr_setstacksize(&writer_attr, STACK_SIZE);

   marcel_create(&writer2_pid, &writer_attr, writer, (any_t)mess[0]);
   marcel_create(&writer1_pid, &writer_attr, writer,  (any_t)mess[1]);

   marcel_join(writer1_pid, &status);
   marcel_join(writer2_pid, &status);

   marcel_end();
   return 0;
}
#else /* MARCEL_CLEANUP_ENABLED */
#  warning Marcel cleanup must be enabled for this program
int marcel_main(int argc, char *argv[])
{
  fprintf(stderr,
	  "'marcel cleanup' feature disabled in the flavor\n");

  return 0;
}
#endif /* MARCEL_CLEANUP_ENABLED */
