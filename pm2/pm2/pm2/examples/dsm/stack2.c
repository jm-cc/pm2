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

#include <sys/mman.h>
#include <stdio.h>
#include "pm2.h"


#if !defined(DSM_SHARED_STACK) || !defined(ENABLE_STACK_JUMPING)
#warning "This example needs DSM SHARED_STACK and MARCEL STACK_JUMP options"
#include <stdio.h>
int pm2_main(int argc, char **argv) {
  fprintf(stderr,"This example needs DSM SHARED_STACK and MARCEL STACK_JUMP options\n");
  return 0;
}
#else

#undef SINGLE

int DSM_SERVICE;

dsm_mutex_t L1,L2, L3;

int *start; // begining of the new stack
int **other; // a page in process 1

pm2_completion_t c;


#define MY_ALIGN(X) ((((unsigned long) X)+(SLOT_SIZE-1)) & ~(SLOT_SIZE-1))

void f() {

  volatile int *a;

  tfprintf(stderr,"[%p, %p] In f\n", marcel_self(), (void*) get_sp());

#ifndef SINGLE
  dsm_mutex_lock(&L1);
#endif

//  check_altstack();
  tfprintf(stderr,"[%p, %p] reading from %p...............\n", marcel_self(), (void*) get_sp(),other);

//  mlockall(MCL_FUTURE);

#ifndef SINGLE
  TIMING_EVENT("Before SIGSEGV1");
  a=*other;
  TIMING_EVENT("After SIGSEGV1");
  tfprintf(stderr,"[%p, %p] reading %d from %p......\n", marcel_self(), (void*) get_sp(),
	   a, other);

#endif
  dsm_mutex_unlock(&L2);
  dsm_mutex_lock(&L3);
  
  TIMING_EVENT("Before SIGSEGV2");
  a=*other;
  TIMING_EVENT("After SIGSEGV2");

  tfprintf(stderr,"[%p, %p] reading from %p........\n", marcel_self(), (void*) get_sp(),
	   a, other);

}

marcel_key_t bufkey;

void local() {

  jmp_buf buf;

  /* first, jump to the other stack */
  marcel_prepare_stack_jump(start);

  *((void**) (((void*) (get_sp()  & ~(SLOT_SIZE-1)) )+(4096*6-16)))=0;


  tfprintf(stderr,"[%p, %p] In local, with start=%p\n", marcel_self(), (void*) get_sp());

  if (setjmp(buf)==0) {
    marcel_setspecific(bufkey, (any_t) &buf);
    set_sp(((void*)start)+SLOT_SIZE-4096-16);
    f();
    tfprintf(stderr,"[%p, %p] Back in local, sp= %p\n", marcel_self(), (void*) get_sp(),get_sp());
    longjmp( *(jmp_buf*) marcel_getspecific(bufkey) ,1);
  } 
  tfprintf(stderr,"[%p, %p] Return ok with sp=%p\n", marcel_self(), (void*) get_sp(),
	   (void*) get_sp());

  /* that's all */
  pm2_completion_signal(&c); 
}

/************************************************************/
/************************************************************/
/************************************************************/

void remote() {

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata(); 

  other = (int**)
    (((unsigned long) dsm_get_pseudo_static_dsm_start_addr())+4*SLOT_SIZE
     -sizeof(int*));

  tfprintf(stderr,"[%p, %p] writing at %p\n", marcel_self(), (void*) get_sp(),other);
  *other = (int*) 13;

  dsm_mutex_unlock(&L1);
  tfprintf(stderr,"ok.....................................1\n");

  dsm_mutex_lock(&L2);
  *other = (int*) 0xACE;
  tfprintf(stderr,"ok.....................................2\n");
  dsm_mutex_unlock(&L3);
  tfprintf(stderr,"ok.....................................3\n");
  pm2_completion_signal(&c); 
}

static void DSM_func(void) {
  pm2_thread_create(remote, NULL);
}

/************************************************************/
/************************************************************/
/************************************************************/

int pm2_main(int argc, char **argv) {
  int i;
  
  pm2_rawrpc_register(&DSM_SERVICE, DSM_func);
  
  dsm_set_pseudo_static_area_size(4*SLOT_SIZE);
  dsm_set_default_protocol(LI_HUDAK);

  pm2_set_dsm_page_distribution(DSM_BLOCK); /* half on each process */
  
  dsm_mutex_init(&L1, NULL);
  dsm_mutex_init(&L2, NULL);
  dsm_mutex_init(&L3, NULL);

  pm2_init(&argc, argv);
  
  marcel_key_create(&bufkey,0);

#ifndef SINGLE
  if (pm2_config_size() != 2) {    
    fprintf(stderr, "Sorry, but this programs requires 2 processes.\n");
    exit(-1);
  }
#endif

  /*  dsm_display_page_ownership(); */
  start = (int*) MY_ALIGN(dsm_get_pseudo_static_dsm_start_addr());
  other = (int**)
    (((unsigned long) dsm_get_pseudo_static_dsm_start_addr())+4*SLOT_SIZE-sizeof(int*));
  
  if(pm2_self() == 0) { /* master process */
    pm2_completion_init(&c, NULL, NULL);
        
#ifndef SINGLE
    dsm_mutex_lock(&L1);
    dsm_mutex_lock(&L2);
    dsm_mutex_lock(&L3);
#endif

#ifndef SINGLE
    /* create a local thread */
    pm2_thread_create(local, NULL); 

    /* create a remote thread */
    pm2_rawrpc_begin(1, DSM_SERVICE, NULL);
    pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
    pm2_rawrpc_end();

    for (i = 0 ; i < 2; i++)
#endif
      pm2_completion_wait(&c);
    
    pm2_halt();
  }
  
  pm2_exit();
  
  tfprintf(stderr, "Main is ending\n");
  return 0;
}

#endif
