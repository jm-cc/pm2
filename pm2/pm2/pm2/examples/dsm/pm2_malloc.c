/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#include <stdio.h>

#include "pm2.h"
#include "timing.h"
#include "dsm_slot_alloc.h"

#define COUNTER 

int DSM_SERVICE;
dsm_mutex_t L;
pm2_completion_t c;
int *ptr, *start;
int private_data_size;

void f()
{
  int i, n = 20;
//  char *p = (char*)pm2_isomalloc(private_data_size);

//  dsm_mutex_lock(&L);
  tfprintf(stderr,"user thread ! ptr = %p, *ptr = %d (I am %p)\n", ptr, *ptr, marcel_self());
  for (i = 0; i < n; i++) {
    (*ptr)++;
    //    fprintf(stderr,"ptr = %p, *ptr = %d\n",ptr, *ptr);
  }
  tfprintf(stderr,"user thread finished! ptr = %p, *ptr = %d\n", ptr, *ptr);
//  dsm_mutex_unlock(&L);
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

//  dsm_mutex_lock(&L);
  tfprintf(stderr,"user thread ! ptr = %p, *ptr = %d (I am %p)\n", my_ptr, *my_ptr, marcel_self());
  for (i = 0; i < n; i++) {
    (*my_ptr)++;
    //    fprintf(stderr,"ptr = %p, *ptr = %d\n",my_ptr, *my_ptr);
  }  
  tfprintf(stderr,"user thread finished! ptr = %p, *ptr = %d\n", my_ptr, *my_ptr);

//  dsm_mutex_unlock(&L);
  pm2_completion_signal(&my_c); 
}


static void DSM_func(void)
{
  pm2_thread_create(threaded_f, NULL);
}


int pm2_main(int argc, char **argv)
{
  int i, j;

  pm2_rawrpc_register(&DSM_SERVICE, DSM_func);

  //dsm_set_default_protocol(MIGRATE_THREAD);
  dsm_set_default_protocol(LI_HUDAK);

  isoaddr_page_set_distribution(DSM_BLOCK);

  dsm_mutex_init(&L, NULL);

  pm2_init(&argc, argv);

  if (argc != 6)
    {
      fprintf(stderr, "Usage: simple <number of threads per node> <shared pages> <protocol> <offset> <private per thread data>\n");
      exit(1);
    }

//  dsm_display_page_ownership();

  private_data_size =  atoi(argv[5]);

  if(pm2_self() == 0) { /* master process */
    isoaddr_attr_t attr;

    isoaddr_attr_set_atomic(&attr);
    isoaddr_attr_set_status(&attr, ISO_SHARED);
    //isoaddr_attr_set_protocol(&attr, LI_HUDAK);
    //    isoaddr_attr_set_protocol(&attr, MIGRATE_THREAD);
    isoaddr_attr_set_protocol(&attr, atoi(argv[3]));
    start = (int *)pm2_malloc((atoi(argv[2]))*4096, &attr);
    ptr = (int *)((char*)start + atoi(argv[4]) * 4096);
    fprintf(stderr,"ptr = %p\n",ptr);

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

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
