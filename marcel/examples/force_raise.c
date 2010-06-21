
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

#include "marcel.h"

#if defined(MARCEL_EXCEPTIONS_ENABLED) && defined(MARCEL_DEVIATION_ENABLED)
marcel_exception_t USER_ERROR = "USER_ERROR";

void deviation(void *arg)
{
   MARCEL_EXCEPTION_RAISE(USER_ERROR);
}

any_t func(any_t arg)
{ int i;

   MARCEL_EXCEPTION_BEGIN
     for(i=0; i<10; i++) {
       marcel_delay(100);
       tprintf("Hi %s!\n", (char *)arg);
     }
   MARCEL_EXCEPTION
     MARCEL_EXCEPTION_WHEN(USER_ERROR)
       tprintf("God! I have been disturbed!\n");
       return NULL;
   MARCEL_EXCEPTION_END
   tprintf("Ok, I was not disturbed.\n");
   return NULL;
}

int marcel_main(int argc, char *argv[])
{ marcel_t pid;
  any_t status;

   marcel_init(argc, argv);

   marcel_create(&pid, NULL, func, (any_t)"boys");

   marcel_delay(500);
   marcel_deviate(pid, deviation, NULL);

   marcel_join(pid, &status);

   marcel_end();

   return 0;
}
#else
#  warning Marcel exceptions and deviation must be enabled for this program
int marcel_main(int argc, char *argv[])
{
  fprintf(stderr, "'marcel exceptions' or 'deviation' features disabled in the flavor\n");
  return 0;
}
#endif

