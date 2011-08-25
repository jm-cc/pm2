
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

#include "rpc_defs.h"

#define NB	3

static char *mess[NB] = {
  "Hi Guys !",
  "Hi Girls !",
  "Hello world !"
};

BEGIN_SERVICE(SAMPLE)
   int i, j;

   for(i=0; i<10; i++) {
      pm2_printf("%s (I am %p on LWP %d)\n",
		 req.tab, marcel_self(), marcel_current_vp());
      for(j=5000000; j; j--) ;
   }
END_SERVICE(SAMPLE)

void f(void)
{
  pm2_rpc_wait_t att[NB];
  int i;
  LRPC_REQ(SAMPLE) req;
  pm2_attr_t attr;

  pm2_attr_init(&attr);
  for(i=0; i<NB; i++) {
    strcpy(req.tab, mess[i]);
    pm2_rpc_call(1, SAMPLE, &attr, &req, NULL, &att[i]);
  }

  for(i=0; i<NB; i++)
    pm2_rpc_wait(&att[i]);
}

int pm2_main(int argc, char **argv)
{
  pm2_rpc_init();

  DECLARE_LRPC(SAMPLE);

  pm2_init(argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2-conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) { /* master process */

    f();

    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
