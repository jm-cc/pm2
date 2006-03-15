
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

#include <sys/types.h>

#include "tbx_compiler.h"
#include "tbx_macros.h"
#include "marcel_alloc___macros.h"
#include "sys/marcel_flags.h"
#include "marcel_valgrind.h"

#ifdef MARCEL_KERNEL
#define MARCEL_PROTECTED TBX_PROTECTED
#else
#define MARCEL_PROTECTED
#endif

__TBX_BEGIN_DECLS

//#include "asm/marcel-master___compiler.h"
#include "marcel-master___compiler.h"
//#include "scheduler/marcel-master___compiler.h"

#ifdef MA__LIBPTHREAD
//#  include "marcel_pthread.h"
#endif

#ifdef MA__LIBPTHREAD
#  include "pthread_libc-symbols.h"
#endif

#include "asm/marcel-master___macros.h"
#include "marcel-master___macros.h"
#include "scheduler/marcel-master___macros.h"

#include "asm/marcel-master___types.h"
#include "marcel-master___types.h"
#include "scheduler/marcel-master___types.h"

#include "asm/marcel-master___structures.h"
#include "marcel-master___structures.h"
#include "scheduler/marcel-master___structures.h"

#ifdef MA__IFACE_PMARCEL
#  include "marcel_pmarcel.h"
#endif

TBX_VISIBILITY_PUSH_DEFAULT
//#include "asm/marcel-master___functions.h"
#include "marcel-master___functions.h"
#include "scheduler/marcel-master___functions.h"
TBX_VISIBILITY_POP

//#include "asm/marcel-master___variables.h"
#include "marcel-master___variables.h"
#include "scheduler/marcel-master___variables.h"

#ifdef MARCEL_COMPILE_INLINE_FUNCTIONS
#  define MARCEL_INLINE
#else
#  define MARCEL_INLINE inline
#endif

//#include "asm/marcel-master___inline.h"
#include "marcel-master___inline.h"
#include "scheduler/marcel-master___inline.h"

#if defined(MARCEL_KERNEL) || defined(PM2_KERNEL) || defined(DSM_KERNEL) || defined (MARCEL_INTERNAL_INCLUDE)
TBX_VISIBILITY_PUSH_INTERNAL
//#  include "asm/marcel-master___marcel_compiler.h"
#  include "marcel-master___marcel_compiler.h"
//#  include "scheduler/marcel-master___marcel_compiler.h"

#  include "asm/marcel-master___marcel_macros.h"
#  include "marcel-master___marcel_macros.h"
#  include "scheduler/marcel-master___marcel_macros.h"

#  include "asm/marcel-master___marcel_types.h"
#  include "marcel-master___marcel_types.h"
#  include "scheduler/marcel-master___marcel_types.h"

//#  include "asm/marcel-master___marcel_structures.h"
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
TBX_VISIBILITY_POP

//#  include "asm/marcel-master___all.h"
//#  include "marcel-master___all.h"
//#  include "scheduler/marcel-master___all.h"
#endif

__TBX_END_DECLS

//#include "marcel_alias.h"

//#ifdef AIX_SYS
//#include <time.h>
//#endif

#include "tbx_debug.h"

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

#include "marcel_alloc___functions.h"
#include "tbx.h"
#include "pm2_profile.h"

#include "pm2_common.h"

/* ========== once objects ============ */

#define marcel_once_init { 0 }

/* ============== stack ============= */

TBX_FMALLOC
marcel_t marcel_alloc_stack (unsigned size) __tbx_deprecated__;

unsigned long marcel_usablestack(void);

unsigned long marcel_unusedstack(void);

static __tbx_inline__ char *marcel_stackbase(marcel_t pid) TBX_UNUSED;
static __tbx_inline__ char *marcel_stackbase(marcel_t pid)
{
  return (char *)pid->stack_base;
}


/* ============= miscellaneous ============ */

int  marcel_test_activity(void);

void marcel_start_playing(void);

#if defined(LINUX_SYS) || defined(GNU_SYS)
long marcel_random(void);
#endif

#endif // MARCEL_EST_DEF
