
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

/* userspace.c */

#include "marcel.h"

#ifdef MARCEL_USERSPACE_ENABLED
#define STACK_SIZE	10000

#define MESSAGE		"Hi boys !"

static
any_t writer(any_t arg)
{
  int i, j;
  char *s;

  marcel_getuserspace(marcel_self(), (any_t *)&s);

  for(i=0;i<10;i++) {
    tfprintf(stderr, "%s\n", s);
    j = 10000; while(j--);
  }
  return NULL;
}

int marcel_main(int argc, char *argv[])
{ any_t status;
  marcel_t writer_pid;
  marcel_attr_t writer_attr;
  void *ptr;

   marcel_init(&argc, argv);

   marcel_attr_init(&writer_attr);

   marcel_attr_setuserspace(&writer_attr, strlen(MESSAGE)+1);
   marcel_attr_setstacksize(&writer_attr, STACK_SIZE);

   marcel_create(&writer_pid, &writer_attr, writer, NULL);

   marcel_getuserspace(writer_pid, &ptr);
   strcpy(ptr, MESSAGE);

   marcel_run(writer_pid, NULL);

   marcel_join(writer_pid, &status);

   tfprintf(stderr, "marcel ended\n");

   marcel_end();
   return 0;
}
#else
#  warning Marcel userspace must be enabled for this program
int marcel_main(int argc, char *argv[])
{
  fprintf(stderr, "'marcel userspace' feature disabled in the flavor\n");
  return 0;
}
#endif /* MARCEL_USERSPACE_ENABLED */
