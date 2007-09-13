
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

#section macros
#ifdef MARCEL_EXCEPTIONS_ENABLED
#  define MARCEL_EXCEPTION_BEGIN { \
                         marcel_exception_block_t _excep_blk;				\
          	       marcel_t __cur = marcel_self();			\
                         _excep_blk.old_blk=__cur->cur_excep_blk; \
                         __cur->cur_excep_blk=&_excep_blk; \
                         if (marcel_ctx_setjmp(_excep_blk.ctx) == 0) {
#  define MARCEL_EXCEPTION   __cur->cur_excep_blk=_excep_blk.old_blk; } \
                             else { __cur->cur_excep_blk=_excep_blk.old_blk;	\
          		   if(0) {
#  define MARCEL_EXCEPTION_WHEN(ex)       } else if (__cur->cur_exception == ex) {
#  define MARCEL_EXCEPTION_WHEN2(ex1,ex2) } else if (__cur->cur_exception == ex1 || \
                                          __cur->cur_exception == ex2) {
#  define MARCEL_EXCEPTION_WHEN3(ex1,ex2,ex3) } else if (__cur->cur_exception == ex1 || \
                                              __cur->cur_exception == ex2 || \
                                              __cur->cur_exception == ex3) {
#  define MARCEL_EXCEPTION_WHEN_OTHERS    } else if(1) {
#  define MARCEL_EXCEPTION_END            } else _marcel_raise_exception(NULL); }}

#  define MARCEL_EXCEPTION_RAISE(ex) (SELF_GETMEM(exfile)=__FILE__,SELF_GETMEM(exline)=__LINE__,_marcel_raise_exception(ex))
#  define MARCEL_EXCEPTION_RRAISE    (SELF_GETMEM(exfile)=__FILE__,SELF_GETMEM(exline)=__LINE__,_marcel_raise_exception(NULL))

#  define MARCEL_LOOP(name)	{ marcel_exception_block_t _##name##_excep_blk; \
          int _##name##_val; \
          _##name##_excep_blk.old_blk=SELF_GETMEM(cur_excep_blk); \
          SELF_GETMEM(cur_excep_blk)=&_##name##_excep_blk; \
          if ((_##name##_val = marcel_ctx_setjmp(_##name##_excep_blk.ctx)) == 0) { \
          while(1) {

#  define MARCEL_EXIT_LOOP(name) marcel_ctx_longjmp(_##name##_excep_blk.ctx, 2)

#  define MARCEL_END_LOOP(name) } } \
	SELF_GETMEM(cur_excep_blk)=_##name##_excep_blk.old_blk; \
	if(_##name##_val == 1) _marcel_raise_exception(NULL); }

#else /* MARCEL_EXCEPTIONS_ENABLED */
#  define MARCEL_EXCEPTION_RAISE(ex) (abort())
#  define MARCEL_EXCEPTION_RRAISE    MARCEL_EXCEPTION_RAISE(NULL)
#endif /* MARCEL_EXCEPTIONS_ENABLED */

#section types
#ifdef MARCEL_EXCEPTIONS_ENABLED
typedef char *marcel_exception_t;
typedef struct marcel_exception_block marcel_exception_block_t;
#endif /* MARCEL_EXCEPTIONS_ENABLED */

#section structures
#depend "asm/marcel_ctx.h[structures]"

#ifdef MARCEL_EXCEPTIONS_ENABLED
struct marcel_exception_block {
	marcel_ctx_t ctx;
	struct marcel_exception_block *old_blk;
};
#endif /* MARCEL_EXCEPTIONS_ENABLED */

#section variables
#ifdef MARCEL_EXCEPTIONS_ENABLED
extern marcel_exception_t
  MARCEL_TASKING_ERROR,
  MARCEL_DEADLOCK_ERROR,
  MARCEL_STORAGE_ERROR,
  MARCEL_CONSTRAINT_ERROR,
  MARCEL_PROGRAM_ERROR,
  MARCEL_STACK_ERROR,
  MARCEL_TIME_OUT,
  MARCEL_NOT_IMPLEMENTED,
  MARCEL_USE_ERROR;
#else /* MARCEL_EXCEPTIONS_ENABLED */
#  define MARCEL_TASKING_ERROR		NULL
#  define MARCEL_DEADLOCK_ERROR		NULL
#  define MARCEL_STORAGE_ERROR		NULL
#  define MARCEL_CONSTRAINT_ERROR	NULL
#  define MARCEL_PROGRAM_ERROR		NULL
#  define MARCEL_STACK_ERROR		NULL
#  define MARCEL_TIME_OUT		NULL
#  define MARCEL_NOT_IMPLEMENTED	NULL
#  define MARCEL_USE_ERROR		NULL
#endif /* MARCEL_EXCEPTIONS_ENABLED */

#section functions
#include "tbx_compiler.h"
#ifdef MARCEL_EXCEPTIONS_ENABLED
int TBX_NORETURN _marcel_raise_exception(marcel_exception_t ex);
#endif /* MARCEL_EXCEPTIONS_ENABLED */


