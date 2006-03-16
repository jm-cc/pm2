
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

#section common
#include "tbx_compiler.h"

#section functions

/* = initialization & termination == */
#define marcel_init(argc, argv) common_init(argc, argv, NULL)
#define marcel_end() common_exit(NULL)

// When completed, calls to marcel_self() are ok, etc.
// So do calls to the Unix Fork primitive.
void marcel_init_data(int *argc, char *argv[]);

// May start some internal threads or activations.
// When completed, fork calls are prohibited.
void marcel_start_sched(int *argc, char *argv[]);

void marcel_init_section(int section);

void marcel_purge_cmdline(int *argc, char *argv[]);

void marcel_finish(void);

void marcel_strip_cmdline(int *argc, char *argv[]);

#section functions
#ifdef STANDARD_MAIN
#define marcel_main main
#else
#ifdef MARCEL_MAIN_AS_FUNC
int go_marcel_main(int argc, char *argv[]);
#endif
#ifdef MARCEL_KERNEL
extern int marcel_main(int argc, char *argv[]);
#endif // MARCEL_KERNEL
#endif

#section marcel_macros
#ifdef OSF_SYS
#define __marcel_init
#else
#define __marcel_init TBX_TEXTSECTION(".ma.initfunc")
#endif

#section marcel_types
typedef void (*__ma_initfunc_t)(void);
typedef struct __ma_init_info {
	__ma_initfunc_t func;
        int section;
	char prio;
	const char **debug;
	char *file;
} TBX_ALIGNED __ma_init_info_t;

#section marcel_macros
#define __ma_initfunc_prio_internal(_func, _section, _prio, _pdebug) \
  TBX_INTERNAL const __ma_init_info_t ma_init_info_##_func \
    TBX_ALIGNED = {.func=&_func, .section=_section, .prio=_prio, .debug=_pdebug, .file=__FILE__};
#define __ma_initfunc_prio__(_func, _section, _prio, _debug) \
  static const char *ma_init_info_##_func##_help=_debug; \
  __ma_initfunc_prio_internal(_func, _section, _prio, \
                              &ma_init_info_##_func##_help)
#define __ma_initfunc_prio(_func, _section, _prio, _debug) \
  __ma_initfunc_prio__(_func, _section, _prio, _debug)
#define __ma_initfunc(_func, _section, _debug) \
  __ma_initfunc_prio(_func, _section, MA_INIT_PRIO_BASE, _debug)

/* Proc�dure de lancement de marcel */

/****************************************************************
 * MARCEL_SELF et LWP_SELF deviennent utilisable
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


/* A besoin de MARCEL_SELF et LWP_SELF */
#define MA_INIT_PRIO_BASE                          5

#define MA_INIT_TOPOLOGY              MA_INIT_SELF
#define MA_INIT_THREADS_MAIN          MA_INIT_SELF
#define MA_INIT_FAULT_CATCHER         MA_INIT_MAIN_LWP
#define MA_INIT_FAULT_CATCHER_PRIO                 0
#define MA_INIT_INT_CATCHER           MA_INIT_MAIN_LWP
#define MA_INIT_INT_CATCHER_PRIO                   0
#define MA_INIT_DEBUG                 MA_INIT_MAIN_LWP
#define MA_INIT_DEBUG_PRIO                         1
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
#define MA_INIT_IO                    MA_INIT_MAIN_LWP
#define MA_INIT_GENSCHED_IDLE         MA_INIT_MAIN_LWP
#define MA_INIT_TIMER_SIG_DATA        MA_INIT_MAIN_LWP
#define MA_INIT_TIMER                 MA_INIT_MAIN_LWP
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
#define MA_INIT_UPCALL_TASK_NEW       MA_INIT_SCHEDULER
#define MA_INIT_GENSCHED_START_LWPS   MA_INIT_START_LWPS
#define MA_INIT_UPCALL_START	      MA_INIT_START_LWPS
#define MA_INIT_UPCALL_START_PRIO		    6

