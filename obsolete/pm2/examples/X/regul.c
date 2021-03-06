
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
#include "regul.h"

#define MAX_THREADS	  1024
#define FREQ_OBS          100
#define GROSSE_VARIATION  1

#define abs(x)  (((x) < 0) ? -(x) : (x))

static unsigned local_load = 0, other_load = 0;
static int module_precedent, module_suivant;

static tbx_bool_t do_regulate = tbx_false, finished = tbx_false;

static int LOAD;

static unsigned charge_locale(void)
{
  unsigned nb;

  pm2_freeze();
  pm2_threads_list(0, NULL, &nb, MIGRATABLE_ONLY);
  pm2_unfreeze();

  return nb;
}

static void informer(void)
{
  pm2_rawrpc_begin(module_precedent, LOAD, NULL);
  old_mad_pack_int(MAD_IN_HEADER, &local_load, 1);
  pm2_rawrpc_end();
}

static void reguler(void)
{ 
  /* liste de threads par ordre de priorit� d�croissante : */
  static marcel_t pids[MAX_THREADS], to_migrate[MAX_THREADS];

  int  n = 0, nb;
  unsigned quantite_a_migrer;

   if(local_load > other_load + 1) {

      pm2_freeze();

      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      if(nb >= MAX_THREADS)
         MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);

      quantite_a_migrer = (local_load - other_load) / 2;
#ifdef DEBUG
      fprintf(stderr, "quantite a migrer = %d\n", quantite_a_migrer);
#endif
      for(n=0; (quantite_a_migrer && n < nb); n++) {
	quantite_a_migrer--;;
	to_migrate[n] = pids[n];
      }

      pm2_migrate_group(to_migrate, n, module_suivant);
#ifdef DEBUG
      if(n > 0)
	printf("D�part de %d threads.\n", n);
#endif
   }
}

void load_service(void)
{
  old_mad_unpack_int(MAD_IN_HEADER, &other_load, 1);
  pm2_rawrpc_waitdata();
}

void spy(void *arg)
{
  int charge;

  while(!finished) {
    marcel_delay(FREQ_OBS);

    if(do_regulate) {
      charge = charge_locale();
      if(abs(charge - local_load) >= GROSSE_VARIATION) {
	local_load = charge;
	informer();
      } else
	local_load = charge;
      reguler();
    }
  }
}

void regul_rpc_init(void)
{
  pm2_rawrpc_register(&LOAD, load_service);
}

void regul_init(void)
{
  module_precedent = (pm2_self() + pm2_config_size() - 1) % pm2_config_size();
  module_suivant = (pm2_self() + 1) % pm2_config_size();

  pm2_thread_create(spy, NULL);
}

void regul_start(void)
{
  do_regulate = tbx_true;
}

void regul_stop(void)
{
  do_regulate = tbx_false;
}

void regul_exit()
{
  finished = tbx_true;
}

