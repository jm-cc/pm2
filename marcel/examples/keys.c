
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

/* keys.c */

#include "marcel.h"

#ifdef MARCEL_KEYS_ENABLED
#define STACK_SIZE	10000

static marcel_key_t key;

static void imprime(void)
{
  int i;
  char *str = (char *)marcel_getspecific(key);

   for(i=0;i<10;i++) {
      marcel_printf(str);
      marcel_delay(50);
   }
}

static any_t writer1(any_t arg)
{
   marcel_setspecific(key, (any_t)"Hi boys!\n");

   imprime();

   return NULL;
}

static any_t writer2(any_t arg)
{
   marcel_setspecific(key, (any_t)"Hi girls!\n");

   imprime();

   return NULL;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t writer1_pid, writer2_pid;
  marcel_attr_t writer1_attr, writer2_attr;

   marcel_init(&argc, argv);

   marcel_attr_init(&writer1_attr);
   marcel_attr_init(&writer2_attr);

   marcel_attr_setstacksize(&writer1_attr, STACK_SIZE);
   marcel_attr_setstacksize(&writer2_attr, STACK_SIZE);

   marcel_key_create(&key, NULL);

   marcel_create(&writer2_pid, &writer2_attr, writer1, NULL);
   marcel_create(&writer1_pid, &writer1_attr, writer2, NULL);

   marcel_join(writer1_pid, &status);
   marcel_join(writer2_pid, &status);

   marcel_end();

   return 0;
}
#else /* MARCEL_KEYS_ENABLED */
#  warning Marcel keys must be enabled for this program
int marcel_main(int argc, char *argv[])
{
  fprintf(stderr,
	  "'marcel keys' feature disabled in the flavor\n");

  return 0;
}
#endif /* MARCEL_KEYS_ENABLED */
