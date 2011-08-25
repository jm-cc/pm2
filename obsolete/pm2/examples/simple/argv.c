
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

#include "pm2.h"

int pm2_main(int argc, char **argv)
{
  int i;

  pm2_init(argc, argv);

  printf("argc = %d\n", argc);
  for(i=0; i<argc; i++)
    printf("argv[%d] = <%s>\n", i, argv[i]);

  if(pm2_self() == 0)
    pm2_halt();

  pm2_exit();
  return 0;
}
