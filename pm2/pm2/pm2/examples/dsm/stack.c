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


int DSM_SERVICE;

dsm_mutex_t L;

void *start; // begining of the new stack
pm2_completion_t c;


BEGIN_DSM_DATA
int *adr;
END_DSM_DATA

jmp_buf buf;

#define MY_ALIGN(X) ((((unsigned long) X)+(SLOT_SIZE-1)) & ~(SLOT_SIZE-1))

void f() {
  int a;
  tfprintf(stderr,"[%p] In f, sp at %p\n", marcel_self(),&a);
}

void local() {

  int a;

  /* first, jump to the other stack */
  marcel_prepare_stack_jump(start);

  tfprintf(stderr,"[%p] In local, sp at %p\n", marcel_self(),&a);
  if (setjmp(buf)==0) {
    set_sp(start+SLOT_SIZE-80);
    tfprintf(stderr,"[%p] Go to f\n", marcel_self());
    f();
    longjmp(buf,1);
  } else {
    tfprintf(stderr,"[%p] Return ok\n", marcel_self());
  }

  /* that's all */
  pm2_completion_signal(&c); 
}

void remote() {
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata(); 
  pm2_completion_signal(&c); 
}

static void DSM_func(void) {
  pm2_thread_create(remote, NULL);
}

int pm2_main(int argc, char **argv) {
  int i, j;
  
  pm2_rawrpc_register(&DSM_SERVICE, DSM_func);
  
  pm2_set_dsm_protocol(&dsmlib_ddm_li_hudak_prot);
  pm2_set_dsm_page_distribution(DSM_BLOCK); // half on each process
  
  dsm_mutex_init(&L, NULL);
  dsm_set_pseudo_static_area_size(4*SLOT_SIZE);

  pm2_init(&argc, argv);
  
  if (pm2_config_size() != 2) {    
    fprintf(stderr, "Sorry, but this programs requires 2 processes.\n");
    exit(-1);
  }

  /*  dsm_display_page_ownership(); */
  start = (void*) MY_ALIGN(dsm_get_pseudo_static_dsm_start_addr());

  if(pm2_self() == 0) { /* master process */
    pm2_completion_init(&c, NULL, NULL);
    
    /* create a local thread */
    pm2_thread_create(local, NULL); 
    
    /* create a remote thread */
    pm2_rawrpc_begin(1, DSM_SERVICE, NULL);
    pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
    pm2_rawrpc_end();

    for (i = 0 ; i < 2; i++)
      pm2_completion_wait(&c);
    
    pm2_halt();
  }
  
  pm2_exit();
  
  tfprintf(stderr, "Main is ending\n");
  return 0;
}
