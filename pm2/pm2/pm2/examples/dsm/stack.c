
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

void h(char *t) {
//  marcel_delay(1000);
  dsm_mutex_lock(&L3);
}

void f() {

  volatile int *a;

  tfprintf(stderr,"[%p, %p] In f\n", marcel_self(), (void*) get_sp());

#ifndef SINGLE
  dsm_mutex_lock(&L1);
#endif

//  check_altstack();
  tfprintf(stderr,"[%p, %p] reading from %p...............\n", marcel_self(), (void*) get_sp(),other);

#ifndef SINGLE
  a=*other;
  *other = (int*)&a;
#endif
  tfprintf(stderr,"[%p, %p] reads a=%p from %p (&a=%p)...........\n", marcel_self(), (void*) get_sp(),a,other,&a);

  dsm_mutex_unlock(&L2);
  tfprintf(stderr,"[%p, %p] waiting.....................\n", marcel_self(), (void*) get_sp());
  /*  dsm_mutex_lock(&L3);*/
  dsm_safe_ss(h);
  tfprintf(stderr,"[%p, %p] go.....................\n", marcel_self(), (void*) get_sp());
  tfprintf(stderr,"[%p, %p] comme back to life.............\n", marcel_self(), (void*) get_sp());
  tfprintf(stderr,"[%p, %p] now a= %p (*other=%p)\n", marcel_self(), (void*) get_sp(),a,*other);
}

marcel_key_t bufkey;

void local() {

  jmp_buf buf;

  /* first, jump to the other stack */
  marcel_prepare_stack_jump(start);

  tfprintf(stderr,"[%p, %p] In local, with start=%p\n", marcel_self(), (void*) get_sp(),
	   start);

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

  tfprintf(stderr,"ending....\n");
}

/************************************************************/
/************************************************************/
/************************************************************/

void remote() {
  int **other2;

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata(); 

  other = (int**)
    (((unsigned long) dsm_get_pseudo_static_dsm_start_addr())+4*SLOT_SIZE
     -sizeof(int*));

  tfprintf(stderr,"[%p, %p] writing 0xd at %p\n", marcel_self(), (void*) get_sp(),other);
  *other = (int*) 13;

  dsm_mutex_unlock(&L1);
  tfprintf(stderr,"ok.....................................1\n");

  dsm_mutex_lock(&L2);
  other2= (int**) *other;
  tfprintf(stderr,"[%p, %p] writing 0xace at %p\n", marcel_self(), (void*) get_sp(),other2);
  *other2 = (int*) 0xACE;
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
