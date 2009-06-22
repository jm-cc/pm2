/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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

/* Test the behavior of Marcel wrt. setjmp/longjmp(3).  */

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

int
main (int argc, char *argv[])
{
  jmp_buf buf;

  printf("set jump 1\n");
  if (!setjmp(buf)) {
    printf("long jump 1\n");
    longjmp(buf, 1);
  }
  printf("jumped 1\n");

  printf("set jump 2\n");
  if (!sigsetjmp(buf, 1)) {
    printf("long jump 2\n");
    siglongjmp(buf, 1);
  }
  printf("jumped 2\n");

  return EXIT_SUCCESS;
}
