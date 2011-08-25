
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

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2-conf.\n");
    exit(1);
  }

  if(pm2_self() == pm2_config_size()-1) {

    pm2_printf("Hello world!\n");
    pm2_printf("argc = %d\n", argc);
    for(i=0; i<argc; i++)
      pm2_printf("argv[%d] = <%s>\n", i, argv[i]);

    pm2_halt();

  }

  pm2_exit();
  return 0;
}
