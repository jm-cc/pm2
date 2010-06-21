
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

/* once.c */

#include "marcel.h"

#ifdef MARCEL_ONCE_ENABLED
static
void f(void)
{
   tprintf("Salut les gars !\n");
}

int marcel_main(int argc, char *argv[])
{
  marcel_once_t once = MARCEL_ONCE_INITIALIZER;

  marcel_init(argc, argv);

  marcel_once(&once, f);
  marcel_once(&once, f);
  marcel_once(&once, f);
  marcel_once(&once, f);

  marcel_end();
  return 0;
}
#else /* MARCEL_ONCE_ENABLED */
#  warning Marcel once must be enabled for this program
int marcel_main(int argc, char *argv[])
{
  fprintf(stderr,
	  "'marcel once' feature disabled in the flavor\n");

  return 0;
}
#endif /* MARCEL_ONCE_ENABLED */
