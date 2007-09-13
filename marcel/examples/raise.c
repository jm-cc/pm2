
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

marcel_exception_t USER_ERROR = "USER_ERROR";

any_t func(any_t arg)
{
   MARCEL_EXCEPTION_RAISE(USER_ERROR);

   return NULL;
}

int marcel_main(int argc, char *argv[])
{
  marcel_fprintf(stderr,
	  "WARNING: this program will deliberately cause an EXCEPTION in a few us...\n");

  marcel_init(&argc, argv);

  marcel_create(NULL, NULL, func, NULL);

  marcel_delay(500);

  marcel_end();

  return 0;
}

