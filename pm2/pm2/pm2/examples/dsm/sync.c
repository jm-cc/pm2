
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

#include <stdio.h>
#include "pm2.h"

#include "pm2_sync.h"

BEGIN_DSM_DATA
int toto = 0;
END_DSM_DATA



static void startup_func(int argc, char *argv[], void *arg)
{
  pm2_barrier_init((pm2_barrier_t *)arg);
}

int pm2_main(int argc, char **argv)
{
  int i, j, nodes;
  pm2_barrier_t b;

  dsm_set_default_protocol(HBRC);

  pm2_set_dsm_page_distribution(DSM_CYCLIC, 16);

  pm2_push_startup_func(startup_func, (void *)&b);

  pm2_init(&argc, argv);

  if (argc != 2)
    {
      fprintf(stderr, "Usage: sync <iterations>\n");
      exit(1);
    }

  if ((nodes = pm2_config_size()) < 2)
    {
      fprintf(stderr, "This program requires at least 2 nodes\n");
      exit(1);
    }

  dsm_display_page_ownership();

  for (i = 0 ; i < atoi(argv[1]); i++)
    {
      for (j = 0; j < nodes; j++)
	{
	  if(pm2_self() == j) 
	    { /* master process */
	      tfprintf(stderr, "i = %d, toto=%d\n", i, toto);
	      toto++;
	      tfprintf(stderr, "i = %d, toto=%d\n", i, toto);
	    }
	  pm2_barrier(&b);
	}
    }
  tfprintf(stderr, ">>>>Avant toto final\n");
  tfprintf(stderr, ">>>>toto final =%d\n", toto);
  tfprintf(stderr, ">>>>Apres toto final\n");
  pm2_barrier(&b);
  if(pm2_self() == 0) 
    pm2_halt();
  pm2_exit();
  
  tfprintf(stderr, "Main is ending\n");
  return 0;
}
