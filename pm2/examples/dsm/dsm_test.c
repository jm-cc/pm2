
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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
#include "dsm_slot_alloc.h"

#define COUNTER 

BEGIN_DSM_DATA
dsm_lock_t L;
END_DSM_DATA

int DSM_SERVICE;
//dsm_mutex_t L;
pm2_completion_t c;
int *ptr, *start;
int private_data_size;

void f()
{
  int i, n = 20;
  //  char *p = (char*)pm2_isomalloc(private_data_size);

  dsm_lock(L);
  for (i = 0; i < n; i++) {
    (*ptr)++;
  }
  dsm_unlock(L);
  pm2_completion_signal(&c); 
}


void threaded_f()
{
  int i, n = 20;
  pm2_completion_t my_c;
  int *my_ptr;
//  char *p = (char*)pm2_isomalloc(private_data_size);

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &my_c);
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&my_ptr, sizeof(int *));
  pm2_rawrpc_waitdata(); 

  dsm_lock(L);
  for (i = 0; i < n; i++) {
    (*my_ptr)++;
  }  
  dsm_unlock(L);
  pm2_completion_signal(&my_c); 
}


static void DSM_func(void)
{
  pm2_thread_create(threaded_f, NULL);
}


static void startup_func(int argc, char *argv[], void *arg)
{
  dsm_lock_attr_t attr;
  dsm_proto_t prot = atoi(argv[3]);

  //dsm_set_default_protocol(MIGRATE_THREAD);
  dsm_set_default_protocol(LI_HUDAK);

  isoaddr_page_set_distribution(DSM_BLOCK);

  dsm_lock_attr_init(&attr);
  dsm_lock_attr_register_protocol(&attr, prot);
  dsm_lock_init(&L, &attr);
}


int pm2_main(int argc, char **argv)
{
  int i, j;

#ifdef PROFILE
  profile_activate(FUT_ENABLE, PM2_PROF_MASK | DSM_PROF_MASK, 0);
#endif
 
  pm2_rawrpc_register(&DSM_SERVICE, DSM_func);

  pm2_push_startup_func(startup_func, NULL);

  pm2_init(&argc, argv);

  if (argc != 6)
    {
      fprintf(stderr, "Usage: dsm_test <number of threads per node> <shared pages> <protocol> <offset> <private per thread data>\n");
      exit(1);
    }

  private_data_size =  atoi(argv[5]);

  if(pm2_self() == 0) { /* master process */
    isoaddr_attr_t attr;

    isoaddr_attr_set_atomic(&attr);
    isoaddr_attr_set_status(&attr, ISO_SHARED);
    isoaddr_attr_set_protocol(&attr, atoi(argv[3]));
    start = (int *)pm2_malloc((atoi(argv[2]))*4096, &attr);
    ptr = (int *)((char*)start + atoi(argv[4]) * 4096);

    pm2_completion_init(&c, NULL, NULL);
       *ptr = 0;
    
    /* create local threads */
    for (i=0; i< atoi(argv[1]) ; i++)
      pm2_thread_create(f, NULL); 

    /* create remote threads */
    for (j=1; j < pm2_config_size(); j++)
      for (i=0; i< atoi(argv[1]) ; i++) {
	pm2_rawrpc_begin(j, DSM_SERVICE, NULL);
	pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
	pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&ptr, sizeof(int*));
	pm2_rawrpc_end();
      }

    for (i = 0 ; i < atoi(argv[1]) * pm2_config_size(); i++)
      pm2_completion_wait(&c);

    tfprintf(stderr, "*ptr=%d\n", *ptr );
    pm2_halt();
  }

  pm2_exit();

#ifdef PROFILE
  profile_stop();
#endif

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
