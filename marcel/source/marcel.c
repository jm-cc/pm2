
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

#include "marcel.h"
#include "marcel_timing.h"
#include "marcel_alloc.h"
#include "sys/marcel_work.h"

#include "pm2_common.h"
#include "tbx.h"
#include "pm2_profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <signal.h>

#ifdef UNICOS_SYS
#include <sys/mman.h>
#endif


#ifdef RS6K_ARCH
int _jmg(int r)
{
  if(r!=0) 
    unlock_task();
  return r;
}

#undef longjmp
void LONGJMP(jmp_buf buf, int val)
{
  static jmp_buf _buf;
  static int _val;

  lock_task();
  memcpy(_buf, buf, sizeof(jmp_buf));
  _val = val;
#ifdef PM2_DEV
#warning set_sp() should not be directly used
#endif
  set_sp(SP_FIELD(buf)-256);
  longjmp(_buf, _val);
}
#define longjmp(buf, v)	LONGJMP(buf, v)
#endif

marcel_exception_t
   TASKING_ERROR = "TASKING_ERROR: A non-handled exception occurred in a task",
   DEADLOCK_ERROR = "DEADLOCK_ERROR: Global Blocking Situation Detected",
   STORAGE_ERROR = "STORAGE_ERROR: No space left on the heap",
   CONSTRAINT_ERROR = "CONSTRAINT_ERROR",
   PROGRAM_ERROR = "PROGRAM_ERROR",
   STACK_ERROR = "STACK_ERROR: Stack Overflow",
   TIME_OUT = "TIME OUT while being blocked on a semaphor",
   NOT_IMPLEMENTED = "NOT_IMPLEMENTED (sorry)",
   USE_ERROR = "USE_ERROR: Marcel was not compiled to enable this functionality",
   LOCK_TASK_ERROR = "LOCK_TASK_ERROR: All tasks blocked after bad use of lock_task()";

/* C'EST ICI QU'IL EST PRATIQUE DE METTRE UN POINT D'ARRET
   LORSQUE L'ON VEUT EXECUTER PAS A PAS... */
void breakpoint()
{
  /* Lieu ideal ;-) pour un point d'arret */
}

/* =========== specifs =========== */
int marcel_cancel(marcel_t pid);

/* returns the amount of mem between the base of the current thread stack and
   its stack pointer */
unsigned long marcel_usablestack(void)
{
  return (unsigned long)get_sp() - (unsigned long)marcel_self()->stack_base;
}

/* returns the amount of memory in the current thread stack that has never been
   modified, if stack checking is compiled in */
unsigned long marcel_unusedstack(void)
{
#ifdef STACK_CHECKING_ALLOWED
   char *repere = (char *)marcel_self()->stack_base;
   unsigned *ptr = (unsigned *)repere;

   while(!(*ptr++));
   return ((char *)ptr - repere);

#else
   RAISE(USE_ERROR);
   return 0;
#endif
}

/* marcel_malloc, marcel_calloc, marcel_free:
   avoid lock/unlock_task penalty on trivial requests */
void *marcel_malloc(unsigned size, char *file, unsigned line)
{
        void *p;

        if (size) {
                lock_task();
                p = __TBX_MALLOC(size, file, line);
                unlock_task();
                if(p == NULL)
                        RAISE(STORAGE_ERROR);
        } else {
                return NULL;
        }

        return p;
}

void *marcel_realloc(void *ptr, unsigned size, char * __restrict file, unsigned line)
{
        void *p;

        lock_task();
        p = __TBX_REALLOC(ptr, size, file, line);
        unlock_task();
        if(p == NULL)
                RAISE(STORAGE_ERROR);

        return p;
}

void *marcel_calloc(unsigned nelem, unsigned elsize, char *file, unsigned line)
{
        void *p;

        if (nelem && elsize) {
                lock_task();
                p = __TBX_CALLOC(nelem, elsize, file, line);
                unlock_task();
                if(p == NULL)
                        RAISE(STORAGE_ERROR);
        } else {
                return NULL;
        }

        return p;
}

void marcel_free(void *ptr, char * __restrict file, unsigned line)
{
        if(ptr) {
                lock_task();
                __TBX_FREE((char *)ptr, file, line);
                unlock_task();
        }
}

marcel_key_destructor_t marcel_key_destructor[MAX_KEY_SPECIFIC]={NULL};
int marcel_key_present[MAX_KEY_SPECIFIC]={0};
static marcel_lock_t marcel_key_lock=MARCEL_LOCK_INIT;
unsigned marcel_nb_keys=1;
static unsigned marcel_last_key=0;
/* 
 * Hummm... Should be 0, but for obscure reasons,
 * 0 is a RESERVED value. DON'T CHANGE IT !!! 
*/

DEF_MARCEL_POSIX(int, key_create, (marcel_key_t *key, 
				   marcel_key_destructor_t func), (key, func))
{ /* pour l'instant, le destructeur n'est pas utilise */

   //lock_task();
   marcel_lock_acquire(&marcel_key_lock);
   while ((++marcel_last_key < MAX_KEY_SPECIFIC) &&
	  (marcel_key_present[marcel_last_key])) {
   }
   if(marcel_last_key == MAX_KEY_SPECIFIC) {
     /* sinon, il faudrait remettre à 0 toutes les valeurs spécifiques
	des threads existants */
      marcel_lock_release(&marcel_key_lock);
      //unlock_task();
      RAISE(CONSTRAINT_ERROR);
/*        marcel_last_key=0; */
/*        while ((++marcel_last_key < MAX_KEY_SPECIFIC) && */
/*  	     (marcel_key_present[marcel_last_key])) { */
/*        } */
/*        if(new_key == MAX_KEY_SPECIFIC) { */
/*  	 marcel_lock_release(&marcel_key_lock); */
/*  	 unlock_task(); */
/*  	 RAISE(CONSTRAINT_ERROR); */
/*        } */
   }
   *key = marcel_last_key;
   marcel_nb_keys++;
   marcel_key_present[marcel_last_key]=1;
   marcel_key_destructor[marcel_last_key]=func;
   marcel_lock_release(&marcel_key_lock);
   //unlock_task();
   return 0;
}
DEF_PTHREAD(int, key_create, (pthread_key_t *key, 
				   void (*func)(void *)), (key, func))
DEF___PTHREAD(int, key_create, (pthread_key_t *key, 
				   void (*func)(void *)), (key, func))

DEF_MARCEL_POSIX(int, key_delete, (marcel_key_t key), (key))
{ /* pour l'instant, le destructeur n'est pas utilise */

   //lock_task();
   marcel_lock_acquire(&marcel_key_lock);
   if (marcel_key_present[key]) {
      marcel_nb_keys--;
      marcel_key_present[key]=0;
      marcel_key_destructor[key]=NULL;
   }
   marcel_lock_release(&marcel_key_lock);
   //unlock_task();
   return 0;
}
DEF_PTHREAD(int, key_delete, (pthread_key_t key), (key))
//DEF___PTHREAD(int, key_delete, (pthread_key_t key), (key))

/* ================== Gestion des exceptions : ================== */

int _marcel_raise(marcel_exception_t ex)
{
  marcel_t cur = marcel_self();

   if(ex == NULL)
      ex = cur->cur_exception ;

   if(cur->cur_excep_blk == NULL) {
      ma_preempt_disable();
      fprintf(stderr, "\nUnhandled exception %s in task %d on LWP(%d)"
	      "\nFILE : %s, LINE : %d\n",
	      ex, cur->number, LWP_NUMBER(LWP_SELF), cur->exfile, cur->exline);
      abort(); /* To generate a core file */
      exit(1);
   } else {
      cur->cur_exception = ex ;
      call_ST_FLUSH_WINDOWS();
      marcel_ctx_longjmp(cur->cur_excep_blk->ctx, 1);
   }
}

#ifndef MA__LIBPTHREAD
/* if MA__LIBPTHREAD is defined, marcel_extlib_protect/unprotect are
   defined as (empty) macros instead since they are useless in that case */
int marcel_extlib_protect(void)
{
	ma_local_bh_disable();
	return 0;
}

int marcel_extlib_unprotect(void)
{
	ma_local_bh_enable();
	return 0;
}
#endif /* MA__LIBPTHREAD */

