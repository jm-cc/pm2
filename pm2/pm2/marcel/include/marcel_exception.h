
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
#define BEGIN          { marcel_exception_block_t _excep_blk; \
                       marcel_t __cur = marcel_self(); \
                       _excep_blk.old_blk=__cur->cur_excep_blk; \
                       __cur->cur_excep_blk=&_excep_blk; \
                       if (marcel_ctx_setjmp(_excep_blk.ctx) == 0) {
#define EXCEPTION      __cur->cur_excep_blk=_excep_blk.old_blk; } \
                       else { __cur->cur_excep_blk=_excep_blk.old_blk; \
                       if(0) {
#define WHEN(ex)       } else if (__cur->cur_exception == ex) {
#define WHEN2(ex1,ex2) } else if (__cur->cur_exception == ex1 || \
                                  __cur->cur_exception == ex2) {
#define WHEN3(ex1,ex2,ex3) } else if (__cur->cur_exception == ex1 || \
                                      __cur->cur_exception == ex2 || \
                                      __cur->cur_exception == ex3) {
#define WHEN_OTHERS    } else if(1) {
#define END            } else _marcel_raise(NULL); }}

#define RAISE(ex)      (marcel_self()->exfile=__FILE__,marcel_self()->exline=__LINE__,_marcel_raise(ex))
#define RRAISE         (marcel_self()->exfile=__FILE__,marcel_self()->exline=__LINE__,_marcel_raise(NULL))

/* =================== Quelques definitions utiles ==================== */

#define LOOP(name)	{ marcel_exception_block_t _##name##_excep_blk; \
	int _##name##_val; \
	_##name##_excep_blk.old_blk=marcel_self()->cur_excep_blk; \
	marcel_self()->cur_excep_blk=&_##name##_excep_blk; \
	if ((_##name##_val = marcel_ctx_setjmp(_##name##_excep_blk.ctx)) == 0) { \
	while(1) {

#define EXIT_LOOP(name) marcel_ctx_longjmp(_##name##_excep_blk.ctx, 2)

#define END_LOOP(name) } } \
	marcel_self()->cur_excep_blk=_##name##_excep_blk.old_blk; \
	if(_##name##_val == 1) _marcel_raise(NULL); }

#section types
typedef char *marcel_exception_t;
typedef struct marcel_exception_block marcel_exception_block_t;

#section structures
#depend "asm/marcel_ctx.h[]"

struct marcel_exception_block {
  marcel_ctx_t ctx;
  struct marcel_exception_block *old_blk;
};

#section variables
extern marcel_exception_t
  TASKING_ERROR,
  DEADLOCK_ERROR,
  STORAGE_ERROR,
  CONSTRAINT_ERROR,
  PROGRAM_ERROR,
  STACK_ERROR,
  TIME_OUT,
  NOT_IMPLEMENTED,
  USE_ERROR,
  LOCK_TASK_ERROR;

#section functions
int _marcel_raise(marcel_exception_t ex);


