
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
#define __init __attribute__((section(".ma.initfunc")))

#section marcel_types
typedef void (*__ma_initfunc_t)(void);
typedef struct __ma_init_info {
	__ma_initfunc_t func;
        int section;
	char prio;
	char *debug;
	char *file;
} __attribute__((aligned)) __ma_init_info_t;

#section marcel_macros
#define __MA_INIT_SECTION ".ma.init."

#define __ma_initfunc_prio__(_func, _section, _prio, _debug) \
  const __ma_init_info_t ma_init_info_##_func \
    __attribute__((aligned, \
		section (__MA_INIT_SECTION "info." #_section "." #_prio))) \
    = {.func=&_func, .section=_section, .prio=_prio, .debug=_debug, .file=__FILE__};
#define __ma_initfunc_prio(_func, _section, _prio, _debug) \
  __ma_initfunc_prio__(_func, _section, _prio, _debug)
#define __ma_initfunc(_func, _section, _debug) \
  __ma_initfunc_prio(_func, _section, MA_INIT_PRIO_BASE, _debug)

/* Procédure de lancement de marcel */

/****************************************************************
 * MARCEL_SELF et LWP_SELF deviennent utilisable
 * - Si on utilise la pile, celle-ci doit être correctement positionnée
 * - On remplit les structures si ce n'est pas fait
 *   Avec MA__GLIBC, ça doit être fait au link (sauf %gs)
 * - La préemption est désactivée :
 *   - tâche courrante interdite de switch
 *   - les softirqs sont désactivées
 */
#define MA_INIT_SELF       0

/****************************************************************
 * TLS utilisable 
 * - uniquement MA__GLIBC
 * - (Pas encore implémenté)
 */
#define MA_INIT_TLS        1

/****************************************************************
 * Préparation au démarrage du scheduler
 * - toujours un seul LWP
 * - possibilité d'utiliser par la suite les softintr
 * - impossibilité de créer des threads
 */
#define MA_INIT_MAIN_LWP   2

/****************************************************************
 * Démarrage du scheduler
 * - fin de la préemption forcée (prio 'c')
 * - démarrage des threads systèmes sur le LWP principal
 * - possibilité d'utiliser les softirq
 * - démarrage éventuel du timer
 * - possibilité de créer des threads
 */
#define MA_INIT_SCHEDULER  3

/****************************************************************
 * Démarrage des LWPs
 * - lancement des éventuels LWPs supplémentaires
 * - utiliser les callback pour LWP pour rajouter des actions par LWP
 */
#define MA_INIT_START_LWPS 4

#define MA_INIT_MAX_PARTS  MA_INIT_START_LWPS


/* A besoin de MARCEL_SELF et LWP_SELF */
#define MA_INIT_PRIO_BASE                          'H'

#define MA_INIT_THREADS_MAIN          MA_INIT_SELF
#define MA_INIT_FAULT_CATCHER         MA_INIT_MAIN_LWP
#define MA_INIT_FAULT_CATCHER_PRIO                 'A'
#define MA_INIT_DEBUG                 MA_INIT_MAIN_LWP
#define MA_INIT_DEBUG_PRIO                         'B'
#define MA_INIT_SLOT                  MA_INIT_MAIN_LWP
#define MA_INIT_SLOT_PRIO                          'C'
#define MA_INIT_LWP_MAIN_STRUCT       MA_INIT_MAIN_LWP
#define MA_INIT_LWP_MAIN_STRUCT_PRIO               'F'
#define MA_INIT_SOFTIRQ               MA_INIT_MAIN_LWP
#define MA_INIT_THREADS_DATA          MA_INIT_MAIN_LWP
#define MA_INIT_MARCEL_SCHED          MA_INIT_MAIN_LWP
#define MA_INIT_LWP                   MA_INIT_MAIN_LWP
#define MA_INIT_IO                    MA_INIT_MAIN_LWP
#define MA_INIT_LINUX_SCHED           MA_INIT_MAIN_LWP
#define MA_INIT_GENSCHED_IDLE         MA_INIT_MAIN_LWP
#define MA_INIT_TIMER_SIG_DATA        MA_INIT_MAIN_LWP
#define MA_INIT_TIMER                 MA_INIT_MAIN_LWP
#define MA_INIT_LINUX_TIMER           MA_INIT_MAIN_LWP
#define MA_INIT_LWP_FINISHED          MA_INIT_MAIN_LWP
#define MA_INIT_LWP_FINISHED_PRIO                   'Z'
#define MA_INIT_GENSCHED_PREEMPT      MA_INIT_SCHEDULER
#define MA_INIT_GENSCHED_PREEMPT_PRIO               'C'
#define MA_INIT_THREADS_THREAD        MA_INIT_SCHEDULER
#define MA_INIT_TIMER_SIG             MA_INIT_SCHEDULER
#define MA_INIT_SOFTIRQ_KSOFTIRQD     MA_INIT_SCHEDULER
#define MA_INIT_GENSCHED_START_LWPS   MA_INIT_START_LWPS


