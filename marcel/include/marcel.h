
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

#ifdef __cplusplus
extern "C" {
#endif

#define MARCEL_VERSION 0x020002 /* Version 2.0002 */

#include <sys/types.h>

#include "tbx_compiler.h"
#include "tbx_macros.h"
#include "tbx_types.h"
#include "marcel_alloc___macros.h"
#include "sys/marcel_flags.h"
#include "sys/isomalloc_archdep.h"
#include "marcel_config.h"
#include "pm2_valgrind.h"

#include "marcel_abi.h"


#if (defined(MARCEL_KERNEL) || defined(PM2_KERNEL)) && !defined (MARCEL_INTERNAL_INCLUDE)
#define MARCEL_INTERNAL_INCLUDE
#endif

#if defined(MARCEL_KERNEL) && (__GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ < 3))
#error Your gcc is too old for Marcel, please upgrade to at least 3.3
#endif

/* pour les fonctions que l'on veut "extern inline" pour l'application, il
 * faut aussi fournir un symbole, dans marcel_extern.o */
#ifdef MARCEL_COMPILE_INLINE_FUNCTIONS
#  define MARCEL_INLINE TBX_EXTERN
#else
#  ifdef MARCEL_INTERNAL_INCLUDE
#    define MARCEL_INLINE TBX_EXTERN extern inline
#  else
#    define MARCEL_INLINE TBX_EXTERN extern
#  endif
#endif

/* ============ constants ============= */

#define NO_TIME_OUT		-1

#define NO_TIME_SLICE		0

#define DEFAULT_STACK		0


__TBX_BEGIN_DECLS

/*#include "asm/marcel-master___compiler.h"*/
/*#include "marcel-master___compiler.h"*/
/*#include "scheduler/marcel-master___compiler.h"*/

#ifdef MA__LIBPTHREAD
/*#  include "marcel_pthread.h"*/
#endif

#if defined(MA__LIBPTHREAD) || defined(MA__PROVIDE_TLS)
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
/*#include "asm/marcel-master___functions.h"*/
#include "marcel-master___functions.h"
#include "scheduler/marcel-master___functions.h"
TBX_VISIBILITY_POP

/*#include "asm/marcel-master___variables.h"*/
#include "marcel-master___variables.h"
#include "scheduler/marcel-master___variables.h"

/*#include "asm/marcel-master___inline.h"*/
#include "marcel-master___inline.h"
/*#include "scheduler/marcel-master___inline.h"*/

#ifdef MARCEL_INTERNAL_INCLUDE
/* pthread.h est inclu _ici_ pour qu'il soit inclut avec une visibilité normale par défaut, car sinon notre libpthread.so aurait les symboles pthread_* en interne seulement ! */
#if defined(MA__LIBPTHREAD) || defined(MA__IFACE_LPT) || !defined(MARCEL_DONT_USE_POSIX_THREADS)
#include <pthread.h>
#endif
/* TBX_VISIBILITY_PUSH_INTERNAL */
/*#  include "asm/marcel-master___marcel_compiler.h"*/
#  include "marcel-master___marcel_compiler.h"
/*#  include "scheduler/marcel-master___marcel_compiler.h"*/

#  include "asm/marcel-master___marcel_macros.h"
#  include "marcel-master___marcel_macros.h"
#  include "scheduler/marcel-master___marcel_macros.h"

#  include "asm/marcel-master___marcel_types.h"
#  include "marcel-master___marcel_types.h"
#  include "scheduler/marcel-master___marcel_types.h"

TBX_VISIBILITY_PUSH_DEFAULT
/*#  include "asm/marcel-master___marcel_structures.h"*/
#  include "marcel-master___marcel_structures.h"
#  include "scheduler/marcel-master___marcel_structures.h"
TBX_VISIBILITY_POP

#  include "asm/marcel-master___marcel_functions.h"
#  include "marcel-master___marcel_functions.h"
#  include "scheduler/marcel-master___marcel_functions.h"

#  include "asm/marcel-master___marcel_variables.h"
#  include "marcel-master___marcel_variables.h"
#  include "scheduler/marcel-master___marcel_variables.h"

#  include "asm/marcel-master___marcel_inline.h"
#  include "marcel-master___marcel_inline.h"
#  include "scheduler/marcel-master___marcel_inline.h"
/* TBX_VISIBILITY_POP */

/*#  include "asm/marcel-master___all.h"*/
/*#  include "marcel-master___all.h"*/
/*#  include "scheduler/marcel-master___all.h"*/
#endif

/*#include "marcel_alias.h"*/

#include "tbx_debug.h"

/* ================= included files ================= */

#include "sys/marcel_archsetjmp.h"

#include "marcel_stdio.h"

#include "marcel_alloc___functions.h"
#include "tbx.h"
#include "pm2_profile.h"

#include "pm2_common.h"

/* ========== once objects ============ */

#define MARCEL_ONCE_INITIALIZER { 0 }

/* ============== stack ============= */

TBX_FMALLOC
marcel_t marcel_alloc_stack (unsigned size) __tbx_deprecated__;

unsigned long marcel_usablestack(void);

/* ============= miscellaneous ============ */

void marcel_start_playing(void);

#if defined(LINUX_SYS) || defined(GNU_SYS)
long marcel_random(void);
#endif

DEC_MARCEL_POSIX(int,atfork,(void (*prepare)(void),void (*parent)(void),void (*child)(void)) __THROW);


__TBX_END_DECLS

#ifdef __cplusplus
}
#endif

#endif /* MARCEL_EST_DEF */
