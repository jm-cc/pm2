
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

#ifndef MARCEL_FLAGS_EST_DEF
#define MARCEL_FLAGS_EST_DEF

#ifdef PM2DEBUG
#  define MA__DEBUG
#endif

#ifdef __ACT__
#  error use of __ACT__ depreciated. Define MARCEL_ACT instead
#  undef __ACT__
#  define MARCEL_ACT
#endif

#ifdef SMP
#  error use of SMP depreciated. Define MARCEL_SMP instead
#  undef SMP
#  define MARCEL_SMP
#endif

#ifdef MARCEL_MONO
#  undef MARCEL_MONO
#  define MARCEL_MONO 1
#endif

#ifdef MARCEL_SMP
#  undef MARCEL_SMP
#  define MARCEL_SMP 1
#endif

#ifdef MARCEL_NUMA
#  undef MARCEL_NUMA
#  define MARCEL_NUMA 1
#endif

#ifdef MARCEL_ACT
#  undef MARCEL_ACT
#  define MARCEL_ACT 1
#endif

#ifdef MARCEL_ACTSMP
#  undef MARCEL_ACTSMP
#  define MARCEL_ACTSMP 1
#endif

#ifdef MARCEL_ACTNUMA
#  undef MARCEL_ACTNUMA
#  define MARCEL_ACTNUMA 1
#endif

#if ((MARCEL_MONO + MARCEL_SMP + MARCEL_NUMA + MARCEL_ACT + MARCEL_ACTSMP + MARCEL_ACTNUMA) == 0)
#  error No MARCEL_... defined. Choose between MONO SMP NUMA ACT ACTSMP and ACTNUMA
//#define MARCEL_MONO 1
#endif

#if ((MARCEL_MONO + MARCEL_SMP + MARCEL_NUMA + MARCEL_ACT + MARCEL_ACTSMP + MARCEL_ACTNUMA) > 1)
#  error You must define at most one of MARCEL_MONO, MARCEL_SMP, MARCEL_NUMA, MARCEL_ACT, MARCEL_ACTSMP or MARCEL_ACTNUMA
#endif

/* MA__LWPS : indique que l'on a plusieurs entités ordonnancées par
 * le noyau en parallèle.
 *
 *  Cela arrive en SMP ou en Activation sur multiprocesseur 
 * */
#ifdef MA__LWPS
#  undef MA__LWPS
#endif

/* MA__ACTIVATION : indique qu'on utilise les activations (mono ou
 * smp). On peut donc utiliser les constantes de act.h
 * */
#ifdef MA__ACTIVATION
#  undef MA__ACTIVATION
#endif

/* MA__BIND_LWP_ON_PROCESSORS : indique qu'il faut utiliser attacher les LWPs
 * aux processeurs. (utile uniquement en SMP)
 * */
#ifdef MA__BIND_LWP_ON_PROCESSORS
#  undef MA__BIND_LWP_ON_PROCESSORS
#endif

/* MA__TIMER : indique qu'il faut utiliser le timer unix (signaux)
 * pour la préemption. (ne pas valider avec les activations)
 * */
#ifdef MA__TIMER
#  undef MA__TIMER
#endif

/* MA__INTERRUPTS_USE_SIGINFO : indique que les interruptions utilisent
 * siginfo. Les macros MA_ARCH_(SWITCHTO|INTERRUPT_(ENTER|EXIT))_LWP_FIX
 * peuvent être adaptées pour tranférer le LWP de la libpthread
 * Suppose MA__LWPS && !MA__PTHREAD_FUNCTIONS
 * */
#ifdef MA__INTERRUPTS_USE_SIGINFO
#  undef MA__INTERRUPTS_USE_SIGINFO
#endif

/* MA__WORK : utilisation des files de travail à la fin des unlock_task()
 * */
#ifdef MA__WORK
#  undef MA__WORK
#endif

/* MA__POSIX_FUNCTIONS_NAMES : définit les fonctions pthread_...
 * */
#ifdef MA__POSIX_FUNCTIONS_NAMES
#  undef MA__POSIX_FUNCTIONS_NAMES
#endif

/* MA__POSIX_BEHAVIOUR : définit les fonctions pmarcel_ compatibles avec
 * les propriétées (et structures de données) de la libpthread
 * */
#ifdef MA__POSIX_BEHAVIOUR
#  undef MA__POSIX_BEHAVIOUR
#endif

/* MA__PTHREAD_FUNCTIONS : definit les fonctions nécessaires pour construire
 * une libpthread (ABI)
 * */
#ifdef MA__PTHREAD_FUNCTIONS
#  undef MA__PTHREAD_FUNCTIONS
#endif

/* MA__FUT_RECORD_TID : definit si on enregistre ou pas les tid dans les traces
 * */
#ifdef MA__FUT_RECORD_TID
#  undef MA__FUT_RECORD_TID
#endif

#ifdef MARCEL_MONO /* Marcel Mono */
#  define MA__MONO
#  define MA__TIMER
#  define MA__WORK
#endif /* Fin Marcel Mono */

#if defined(MARCEL_SMP) || defined(MARCEL_NUMA) /* Marcel SMP */
#  define MA__SMP /* Utilisation des pthreads pour les lwp */
#  define MA__LWPS
#  define MA__TIMER
#  define MA__WORK
#  ifdef MARCEL_BIND_LWP_ON_PROCESSORS
#    define MA__BIND_LWP_ON_PROCESSORS
#  endif
#endif /* Fin Marcel SMP */

#if defined(MARCEL_ACT) || defined(MARCEL_ACTSMP) || defined(MARCEL_ACTNUMA) /* Marcel Activation */
#  define MA__ACTIVATION
#  define ACTIVATION
#  define MA__WORK
#endif /* Fin Marcel Activation */

#ifdef MARCEL_ACT /* Marcel Activation Mono */
#  define MA__ACT
#  undef CONFIG_SMP
#endif /* Fin Marcel Activation Mono */

#if defined(MARCEL_ACTSMP) || defined(MARCEL_ACTNUMA) /* Marcel Activation SMP */
#  define MA__ACTSMP
#  define CONFIG_SMP
#  define __SMP__
#  define MA__LWPS
#endif /* Fin Marcel Activation SMP */

#if defined(MARCEL_NUMA) || defined(MARCEL_ACTNUMA)
#  define MA__NUMA
#endif

#if defined(MA__LWPS) && defined(MA__TIMER)
#  define MA_PROTECT_LOCK_TASK_FROM_SIG
#endif

#ifdef MARCEL_POSIX
#  define MA__POSIX_FUNCTIONS_NAMES
#  define MA__POSIX_BEHAVIOUR
#  define MA__PTHREAD_FUNCTIONS
#endif

#if defined(MA__LWPS) && !defined(MA__PTHREAD_FUNCTIONS)
#  define MA__INTERRUPTS_USE_SIGINFO
#endif

/* Les tid sont en fait toujours enregistrés. On en a besoin pour détecter
 * les interruptions entre l'enregistrement du switch_to et le switch_to
 * effectif 
 * */
#define MA__FUT_RECORD_TID

#endif /* MARCEL_FLAGS_EST_DEF */

