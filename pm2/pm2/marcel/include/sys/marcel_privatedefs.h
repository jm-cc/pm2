
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

#ifndef PRIVATEDEFS_EST_DEF
#define PRIVATEDEFS_EST_DEF

#define MARCEL_ALIGN    64L
#define MAL(X)          (((X)+(MARCEL_ALIGN-1)) & ~(MARCEL_ALIGN-1))
#define MAL_BOT(X)      ((X) & ~(MARCEL_ALIGN-1))

#define THREAD_GETMEM(thread_desc, field)   ((thread_desc)->field)
#define THREAD_SETMEM(thread_desc, field, value)   ((thread_desc)->field)=(value)

//typedef marcel_t marcel_descr;

extern marcel_task_t __main_thread_struct;
extern char __security_stack[];


#define SECUR_TASK_DESC(lwp) \
   ((marcel_t)((((unsigned long)(lwp)->__security_stack + 2 * THREAD_SLOT_SIZE) \
	        & ~(THREAD_SLOT_SIZE-1)) - MAL(sizeof(marcel_task_t))))
#define SECUR_STACK_TOP(lwp) \
   ((unsigned long)SECUR_TASK_DESC(lwp) - MAL(1) - TOP_STACK_FREE_AREA)
#define SECUR_STACK_BOTTOM(lwp) \
   ((unsigned long)(lwp)->__security_stack) 


#ifdef ENABLE_STACK_JUMPING
static __inline__ void marcel_prepare_stack_jump(void *stack)
{
  *(marcel_t *)(stack + THREAD_SLOT_SIZE - sizeof(void *)) = __marcel_self();
}

static __inline__ void marcel_set_stack_jump(marcel_t m)
{
  register unsigned long sp = get_sp();

  *(marcel_t *)((sp & ~(THREAD_SLOT_SIZE-1)) + THREAD_SLOT_SIZE - sizeof(void *)) = m;
}
#endif



#endif
