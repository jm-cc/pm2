/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_INIT_H__
#define __MARCEL_INIT_H__


#include "marcel_config.h"
#include "asm/marcel_ctx.h"
#include "tbx_compiler.h"
#include "tbx_macros.h"
#include "tbx_types.h"


/** Public macros **/
/****************************************************************
 * MARCEL_SELF et MA_LWP_SELF deviennent utilisable
 * - Si on utilise la pile, celle-ci doit �tre correctement positionn�e
 * - On remplit les structures si ce n'est pas fait
 *   Avec MA__GLIBC, �a doit �tre fait au link (sauf %gs)
 * - La pr�emption est d�sactiv�e :
 *   - t�che courrante interdite de switch
 *   - les softirqs sont d�sactiv�es
 */
#define MA_INIT_SELF       0

/****************************************************************
 * TLS utilisable 
 * - uniquement MA__GLIBC
 * - (Pas encore impl�ment�)
 */
#define MA_INIT_TLS        1

/****************************************************************
 * Pr�paration au d�marrage du scheduler
 * - toujours un seul LWP
 * - possibilit� d'utiliser par la suite les softintr
 * - impossibilit� de cr�er des threads
 */
#define MA_INIT_MAIN_LWP   2

/****************************************************************
 * D�marrage du scheduler
 * - fin de la pr�emption forc�e (prio 'c')
 * - d�marrage des threads syst�mes sur le LWP principal
 * - possibilit� d'utiliser les softirq
 * - d�marrage �ventuel du timer
 * - possibilit� de cr�er des threads
 */
#define MA_INIT_SCHEDULER  3

/****************************************************************
 * D�marrage des LWPs
 * - lancement des �ventuels LWPs suppl�mentaires
 * - utiliser les callback pour LWP pour rajouter des actions par LWP
 */
#define MA_INIT_START_LWPS 4

#define MA_INIT_MAX_PARTS  MA_INIT_START_LWPS

/* print help menu item */
#define MARCEL_SHOW_OPTION(name, descr)  marcel_fprintf(stderr, "\t--%s\r\t\t\t\t\t\t%s\n", name, descr)

/* initialization */
#define marcel_init(argc, argv)	  marcel_initialize(argc, argv)


/** Public functions **/
/**
 * Return #tbx_true if @param header_hash matches the library's header hash.
 * @param header_hash The user-visible Marcel header hash, i.e.,
 * #MARCEL_HEADER_HASH.
 */
extern tbx_bool_t marcel_header_hash_matches_binary(const char *header_hash);

/**
 * Do nothing if @param header_hash matches the library's header hash,
 * otherwise exit with a non-zero exit code.
 * @param header_hash The user-visible Marcel header hash, i.e., #MARCEL_HEADER_HASH.
 */
extern void marcel_ensure_abi_compatibility(const char *header_hash);

int marcel_test_activity(void);

void marcel_purge_cmdline(void);

void marcel_initialize(int *argc, char *argv[]);

/*  When completed, calls to marcel_self() are ok, etc. */
/*  So do calls to the Unix Fork primitive. */
void marcel_init_data(int argc, char *argv[]);

/*  May start some internal threads. */
/*  When completed, fork calls are prohibited. */
void marcel_start_sched(void);

void marcel_init_section(int section);

void marcel_end(void);

#ifdef STANDARD_MAIN
#  define marcel_main main
#else				/* STANDARD_MAIN */
#  ifdef MARCEL_MAIN_AS_FUNC
int go_marcel_main(int (*main_func) (int, char *[]), int argc, char *argv[]);
#  else				/* MARCEL_MAIN_AS_FUNC */
#    ifdef __MARCEL_KERNEL__
extern int marcel_main(int argc, char *argv[]);
#    else
#      ifdef __GNUC__
/*
 * If compiler is GNU C, we can rename the application's 'main' into
 * marcel_main automatically, saving the user a conditional rename.
 */
int main(int argc, char *argv[]) __asm__(TBX_MACRO_TO_STR(TBX_REAL_SYMBOL_NAME(marcel_main)));
#      endif	/* __GNUC__ */
#    endif	/* __MARCEL_KERNEL__ */
#  endif	/* MARCEL_MAIN_AS_FUNC */
#endif		/* STANDARD_MAIN */


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define __ma_initfunc_prio_internal(_func, _section, _prio, _pdebug) \
  TBX_INTERNAL const __ma_init_info_t ma_init_info_##_func \
  TBX_ALIGNED = {.func=&_func, .section=_section, .prio=_prio, .debug=_pdebug, .file=__FILE__}
#define __ma_initfunc_prio__(_func, _section, _prio, _debug) \
  static const char *ma_init_info_##_func##_help=_debug; \
  __ma_initfunc_prio_internal(_func, _section, _prio, &ma_init_info_##_func##_help)
#define __ma_initfunc_prio(_func, _section, _prio, _debug) \
  __ma_initfunc_prio__(_func, _section, _prio, _debug)
#define __ma_initfunc(_func, _section, _debug) \
  __ma_initfunc_prio(_func, _section, MA_INIT_PRIO_BASE, _debug)

/* Proc�dure de lancement de marcel */
/* A besoin de MARCEL_SELF et MA_LWP_SELF */
#define MA_INIT_PRIO_BASE                          5
#define MA_INIT_TOPOLOGY              MA_INIT_SELF
#define MA_INIT_THREADS_MAIN          MA_INIT_SELF
#define MA_INIT_FAULT_CATCHER         MA_INIT_MAIN_LWP
#define MA_INIT_FAULT_CATCHER_PRIO                 0
#define MA_INIT_INT_CATCHER           MA_INIT_MAIN_LWP
#define MA_INIT_INT_CATCHER_PRIO                   0
#define MA_INIT_SLOT                  MA_INIT_MAIN_LWP
#define MA_INIT_SLOT_PRIO                          2
#define MA_INIT_LINUX_SCHED           MA_INIT_MAIN_LWP
#define MA_INIT_LINUX_SCHED_PRIO                   3
#define MA_INIT_LWP_MAIN_STRUCT       MA_INIT_MAIN_LWP
#define MA_INIT_LWP_MAIN_STRUCT_PRIO               4
#define MA_INIT_BUBBLE_SCHED          MA_INIT_MAIN_LWP
#define MA_INIT_BUBBLE_SCHED_PRIO                  5
#define MA_INIT_SOFTIRQ               MA_INIT_MAIN_LWP
#define MA_INIT_THREADS_DATA          MA_INIT_MAIN_LWP
#define MA_INIT_MARCEL_SCHED          MA_INIT_MAIN_LWP
#define MA_INIT_LWP                   MA_INIT_MAIN_LWP
#define MA_INIT_GENSCHED_IDLE         MA_INIT_MAIN_LWP
#define MA_INIT_TIMER_SIG_DATA        MA_INIT_MAIN_LWP
#define MA_INIT_TIMER                 MA_INIT_MAIN_LWP
#define MA_INIT_SEM_DATA              MA_INIT_MAIN_LWP
#define MA_INIT_LINUX_TIMER           MA_INIT_MAIN_LWP
#define MA_INIT_REGISTER_LWP_NOTIFIER MA_INIT_MAIN_LWP
#define MA_INIT_REGISTER_LWP_NOTIFIER_PRIO          7
#define MA_INIT_LWP_FINISHED          MA_INIT_MAIN_LWP
#define MA_INIT_LWP_FINISHED_PRIO                   9
#define MA_INIT_GENSCHED_PREEMPT      MA_INIT_SCHEDULER
#define MA_INIT_GENSCHED_PREEMPT_PRIO               2
#define MA_INIT_THREADS_THREAD        MA_INIT_SCHEDULER
#define MA_INIT_TIMER_SIG             MA_INIT_SCHEDULER
#define MA_INIT_SOFTIRQ_KSOFTIRQD     MA_INIT_SCHEDULER
#define MA_INIT_FORKD                 MA_INIT_SCHEDULER
#define MA_INIT_UPCALL_TASK_NEW       MA_INIT_SCHEDULER
#define MA_INIT_GENSCHED_START_LWPS   MA_INIT_START_LWPS
#define MA_INIT_UPCALL_START	      MA_INIT_START_LWPS
#define MA_INIT_UPCALL_START_PRIO		    6


/** Internal data types **/
typedef void (*__ma_initfunc_t) (void);
typedef struct __ma_init_info {
	__ma_initfunc_t func;
	int section;
	char prio;
	const char **debug;
	const char *file;
} TBX_ALIGNED __ma_init_info_t;


/** Internal global variables **/
/* children after fork(2) must not call marcel_end() see pthread_ptfork: cleanup_child_after_fork */
extern marcel_ctx_t __ma_initial_main_ctx;
extern volatile int __marcel_main_ret;
extern int ma_init_done[MA_INIT_MAX_PARTS + 1];


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_INIT_H__ **/
