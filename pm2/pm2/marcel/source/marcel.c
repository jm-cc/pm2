
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
#warning set_sp() should not be directly used
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

volatile unsigned long threads_created_in_cache = 0;

/* C'EST ICI QU'IL EST PRATIQUE DE METTRE UN POINT D'ARRET
   LORSQUE L'ON VEUT EXECUTER PAS A PAS... */
void breakpoint()
{
  /* Lieu ideal ;-) pour un point d'arret */
}

/* =========== specifs =========== */
int marcel_cancel(marcel_t pid);

marcel_t marcel_alloc_stack(unsigned size)
{
  marcel_t t;
  char *st;

  lock_task();

#ifdef PM2
  RAISE(PROGRAM_ERROR);
#endif

  st = marcel_slot_alloc();

  t = (marcel_t)(MAL_BOT((unsigned long)st + size) - MAL(sizeof(marcel_task_t)));

  //init_task_desc(t);
  t->stack_base = st;

#ifdef STACK_CHECKING_ALLOWED
  memset(t->stack_base, 0, size);
#endif

  unlock_task();

  return t;
}

unsigned long marcel_cachedthreads(void)
{
   return threads_created_in_cache;
}

unsigned long marcel_usablestack(void)
{
  return (unsigned long)get_sp() - (unsigned long)marcel_self()->stack_base;
}

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

void *marcel_malloc(unsigned size, char *file, unsigned line)
{
  void *p;

    lock_task();
    if((p = __TBX_MALLOC(size, file, line)) == NULL) {
      fprintf(stderr, "Storage error at %s:%d\n", file, line);
      RAISE(STORAGE_ERROR);
    } else {
      unlock_task();
      return p;
    }
}

void *marcel_realloc(void *ptr, unsigned size, char *file, unsigned line)
{
  void *p;

   lock_task();
   if((p = __TBX_REALLOC(ptr, size, file, line)) == NULL)
      RAISE(STORAGE_ERROR);
   unlock_task();
   return p;
}

void *marcel_calloc(unsigned nelem, unsigned elsize, char *file, unsigned line)
{
  void *p;

   if(nelem && elsize) {
      lock_task();
      if((p = __TBX_CALLOC(nelem, elsize, file, line)) == NULL)
         RAISE(STORAGE_ERROR);
      else {
         unlock_task();
         return p;
      }
   }
   return NULL;
}

void marcel_free(void *ptr, char *file, unsigned line)
{
   if(ptr) {
      lock_task();
      __TBX_FREE((char *)ptr, file, line);
      unlock_task();
   }
}

marcel_key_destructor_t marcel_key_destructor[MAX_KEY_SPECIFIC]={NULL};
int marcel_key_present[MAX_KEY_SPECIFIC]={0};
marcel_lock_t marcel_key_lock=MARCEL_LOCK_INIT;
unsigned marcel_nb_keys=1;
static unsigned marcel_last_key=0;
/* 
 * Hummm... Should be 0, but for obscure reasons,
 * 0 is a RESERVED value. DON'T CHANGE IT !!! 
*/

DEF_MARCEL_POSIX(int, key_create, (marcel_key_t *key, 
				   marcel_key_destructor_t func))
{ /* pour l'instant, le destructeur n'est pas utilise */

   lock_task();
   marcel_lock_acquire(&marcel_key_lock);
   while ((++marcel_last_key < MAX_KEY_SPECIFIC) &&
	  (marcel_key_present[marcel_last_key])) {
   }
   if(marcel_last_key == MAX_KEY_SPECIFIC) {
     /* sinon, il faudrait remettre à 0 toutes les valeurs spécifiques
	des threads existants */
      marcel_lock_release(&marcel_key_lock);
      unlock_task();
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
   unlock_task();
   return 0;
}
DEF_PTHREAD(key_create)
DEF___PTHREAD(key_create)

DEF_MARCEL_POSIX(int, key_delete, (marcel_key_t key))
{ /* pour l'instant, le destructeur n'est pas utilise */

   lock_task();
   marcel_lock_acquire(&marcel_key_lock);
   if (marcel_key_present[key]) {
      marcel_nb_keys--;
      marcel_key_present[key]=0;
      marcel_key_destructor[key]=NULL;
   }
   marcel_lock_release(&marcel_key_lock);
   unlock_task();
   return 0;
}
DEF_PTHREAD(key_delete)
//DEF___PTHREAD(key_delete)

DEFINLINE_MARCEL_POSIX(int, setspecific, (marcel_key_t key,
					  __const void* value))
DEF_PTHREAD(setspecific)
DEF___PTHREAD(setspecific)

DEFINLINE_MARCEL_POSIX(any_t, getspecific, (marcel_key_t key))
DEF_PTHREAD(getspecific)
DEF___PTHREAD(getspecific)


/* ================== Gestion des exceptions : ================== */

int _marcel_raise(marcel_exception_t ex)
{
  marcel_t cur = marcel_self();

   if(ex == NULL)
      ex = cur->cur_exception ;

   if(cur->cur_excep_blk == NULL) {
      ma_preempt_disable();
      fprintf(stderr, "\nUnhandled exception %s in task %ld on LWP(%d)"
	      "\nFILE : %s, LINE : %d\n",
	      ex, cur->number, cur->exfile, cur->exline, LWP_NUMBER(LWP_SELF));
      abort(); /* To generate a core file */
      exit(1);
   } else {
      cur->cur_exception = ex ;
      call_ST_FLUSH_WINDOWS();
      marcel_ctx_longjmp(cur->cur_excep_blk->ctx, 1);
   }
}

#ifndef MA__PTHREAD_FUNCTIONS
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
#endif /* MA__PTHREAD_FUNCTIONS */

