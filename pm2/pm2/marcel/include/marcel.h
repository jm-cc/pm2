
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

#ifndef MARCEL_EST_DEF
#define MARCEL_EST_DEF

#define MARCEL_VERSION 0x020000 /* Version 2.0000 */

#include "tbx_compiler.h"
#include "tbx_macros.h"
#include "sys/marcel_flags.h"

__TBX_BEGIN_DECLS

#include "asm/marcel-master___compiler.h"
#include "marcel-master___compiler.h"
#include "scheduler/marcel-master___compiler.h"

#include "asm/marcel-master___macros.h"
#include "marcel-master___macros.h"
#include "scheduler/marcel-master___macros.h"

#include "asm/marcel-master___types.h"
#include "marcel-master___types.h"
#include "scheduler/marcel-master___types.h"

#include "asm/marcel-master___structures.h"
#include "marcel-master___structures.h"
#include "scheduler/marcel-master___structures.h"

#include "asm/marcel-master___functions.h"
#include "marcel-master___functions.h"
#include "scheduler/marcel-master___functions.h"

#include "asm/marcel-master___variables.h"
#include "marcel-master___variables.h"
#include "scheduler/marcel-master___variables.h"

#ifdef MARCEL_COMPILE_INLINE_FUNCTIONS
#  define MARCEL_INLINE
#else
#  define MARCEL_INLINE inline
#endif

#include "asm/marcel-master___inline.h"
#include "marcel-master___inline.h"
#include "scheduler/marcel-master___inline.h"

#if defined(MARCEL_KERNEL) || defined (MARCEL_INTERNAL_INCLUDE)
#  include "asm/marcel-master___marcel_compiler.h"
#  include "marcel-master___marcel_compiler.h"
#  include "scheduler/marcel-master___marcel_compiler.h"

#  include "asm/marcel-master___marcel_macros.h"
#  include "marcel-master___marcel_macros.h"
#  include "scheduler/marcel-master___marcel_macros.h"

#  include "asm/marcel-master___marcel_types.h"
#  include "marcel-master___marcel_types.h"
#  include "scheduler/marcel-master___marcel_types.h"

#  include "asm/marcel-master___marcel_structures.h"
#  include "marcel-master___marcel_structures.h"
#  include "scheduler/marcel-master___marcel_structures.h"

#  include "asm/marcel-master___marcel_functions.h"
#  include "marcel-master___marcel_functions.h"
#  include "scheduler/marcel-master___marcel_functions.h"

#  include "asm/marcel-master___marcel_variables.h"
#  include "marcel-master___marcel_variables.h"
#  include "scheduler/marcel-master___marcel_variables.h"

#  include "asm/marcel-master___marcel_inline.h"
#  include "marcel-master___marcel_inline.h"
#  include "scheduler/marcel-master___marcel_inline.h"

#  include "asm/marcel-master___all.h"
//#  include "marcel-master___all.h"
//#  include "scheduler/marcel-master___all.h"
#endif

__TBX_END_DECLS

//#include "marcel_pthread.h"

#ifdef MA__POSIX_FUNCTIONS_NAMES
#  include "marcel_pmarcel.h"
#endif 
//#include "marcel_alias.h"

//#ifdef AIX_SYS
//#include <time.h>
//#endif

#include "tbx_debug.h"

#ifdef MA__POSIX_FUNCTIONS_NAMES
#  include "pthread_libc-symbols.h"
#endif

/* ========== customization =========== */

#define MAX_KEY_SPECIFIC	20

#define MAX_STACK_CACHE		1024

#define THREAD_THRESHOLD_LOW    1

/* ============ constants ============= */

#define NO_TIME_OUT		-1

#define NO_TIME_SLICE		0

#define DEFAULT_STACK		0


/* ================= included files ================= */

#include "sys/marcel_archsetjmp.h"

#include "sys/marcel_privatedefs.h"
#include "marcel_timing.h"
#include "marcel_stdio.h"

#include "tbx.h"
#include "pm2_profile.h"

#include "pm2_common.h"

/* ========== once objects ============ */

#define marcel_once_init { 0 }

/* ============== stack ============= */

marcel_t marcel_alloc_stack (unsigned size) __attribute__((deprecated));

unsigned long marcel_usablestack(void);

unsigned long marcel_unusedstack(void);

static __inline__ char *marcel_stackbase(marcel_t pid) __attribute__ ((unused));
static __inline__ char *marcel_stackbase(marcel_t pid)
{
  return (char *)pid->stack_base;
}


/* ============= miscellaneous ============ */

unsigned long marcel_cachedthreads(void);

int  marcel_test_activity(void);
void marcel_set_activity(void);
void marcel_clear_activity(void);


/* ======= MT-Safe functions from standard library ======= */

#define tmalloc(size)          marcel_malloc(size, __FILE__, __LINE__)
#define trealloc(ptr, size)    marcel_realloc(ptr, size, __FILE__, __LINE__)
#define tcalloc(nelem, elsize) marcel_calloc(nelem, elsize, __FILE__, __LINE__)
#define tfree(ptr)             marcel_free(ptr, __FILE__, __LINE__)

void *marcel_malloc(unsigned size, char *file, unsigned line);
void *marcel_realloc(void *ptr, unsigned size, char *file, unsigned line);
void *marcel_calloc(unsigned nelem, unsigned elsize, char *file, unsigned line);
void marcel_free(void *ptr, char *file, unsigned line);


void marcel_restart_main_task(void);

#endif // MARCEL_EST_DEF
