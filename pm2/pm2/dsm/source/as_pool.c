/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
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

/*
 * This file manages a pool of stack for the alternate stack's signal
 * handler.
 *
 * There are only stack allocation, never stack free !
 *
 */

/* Compile the module only if the following flag is defined */
#ifdef DSM_SHARED_STACK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "tbx.h"
#include "pm2debug.h"
#include "sys/isomalloc_archdep.h"
#include "as_pool.h"


#ifdef LINUX
#include <asm/unistd.h>
#ifndef __NR_sigaltstack
#define __NR_sigaltstack       186
#endif
inline _syscall2(int, sigaltstack,  const stack_t *,s, stack_t *,os);
#endif

/************************************************************/
/************************************************************/
/************************************************************/

/* GLOBALS !!!!!!!!!!!!!!!!!!! */

p_tbx_memory_t as_head;
static void* current_stack;

/************************************************************/
/************************************************************/
/************************************************************/

/* Private functions */

/* none ! */


/************************************************************/
/************************************************************/
/************************************************************/

/* Public functions */

/* Note: 
 *
 *    void __inline__ dsm_as_free_stack(void *ptr);
 *
 *    is defined as inline in as_pool.h
 *
 */

void* dsm_as_setup_altstack() {

  int r;
  stack_t s,os;

  void* base_altstack = tbx_malloc(as_head);

  tfprintf(stderr,"Altstack memory alloc [%p, %p[", base_altstack,
       base_altstack+AS_STACK_SIZE);

  s.ss_sp = base_altstack;
  s.ss_size = AS_STACK_SIZE;
  s.ss_flags = 0;

#if defined(IRIX_SYS)
  s.ss_sp += s.ss_size-sizeof(int*);
#endif

  current_stack = base_altstack;

  if ((r = sigaltstack(&s,&os))) {
    char tmp[1024];
		perror("--->");
    sprintf(tmp,"Glooops : sigaltstack return %d for base=%p sp=%p size=%d\n",
	    r,base_altstack, s.ss_sp, s.ss_size);
			
    FAILURE(tmp);
  }

  return os.ss_sp;
}

void dsm_as_check_altstack() {

  stack_t ss;
  int r;

  r = sigaltstack(NULL,&ss);
  
  DISP("Altstack setup at %p with size %d and flags %d\n",
       ss.ss_sp, ss.ss_size, ss.ss_flags);

}

void  dsm_as_init() {
  /* fisrt, allocate memory */
  tbx_malloc_init(&as_head, AS_STACK_SIZE, AS_NB_AREA);

  /* then, set up altstack */
  dsm_as_setup_altstack();
}

#endif
