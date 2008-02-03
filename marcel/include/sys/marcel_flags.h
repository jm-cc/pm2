
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

#if ((MARCEL_MONO + MARCEL_SMP + MARCEL_NUMA) == 0)
#  error No MARCEL_... defined. Choose between MONO SMP and NUMA Marcel options in the flavor
//#define MARCEL_MONO 1
#endif

#if ((MARCEL_MONO + MARCEL_SMP + MARCEL_NUMA) > 1)
#  error You must define at most one of MARCEL_MONO, MARCEL_SMP or MARCEL_NUMA Marcel options in the flavor
#endif

/* MA__LWPS : indique que l'on a plusieurs entités ordonnancées par
 * le noyau en parallèle.
 * */
#ifdef MA__LWPS
#  undef MA__LWPS
#endif

/* MA__BIND_LWP_ON_PROCESSORS : indique qu'il faut utiliser attacher les LWPs
 * aux processeurs. (utile uniquement en SMP)
 * */
#ifdef MA__BIND_LWP_ON_PROCESSORS
#  undef MA__BIND_LWP_ON_PROCESSORS
#endif

/* MA__TIMER : indique qu'il faut utiliser le timer unix (signaux)
 * pour la préemption.
 * */
#ifdef MA__TIMER
#  undef MA__TIMER
#endif

/* MA__INTERRUPTS_FIX_LWP : indique que l'on doit tranférer le LWP de la
 * libpthread à l'aide des macros
 * MA_ARCH_(SWITCHTO|INTERRUPT_(ENTER|EXIT))_LWP_FIX
 * Suppose MA__LWPS && !MA__LIBPTHREAD
 * */
#ifdef MA__INTERRUPTS_FIX_LWP
#  undef MA__INTERRUPTS_FIX_LWP
#endif

/* MA__WORK : utilisation des files de travail à la fin des ma_preempt_enable()
 * */
#ifdef MA__WORK
#  undef MA__WORK
#endif

/* MA__IFACE_PMARCEL : définit les symboles/structures pmarcel_...
 * (compatible API avec la norme POSIX)
 * */
#ifdef MA__IFACE_PMARCEL
#  undef MA__IFACE_PMARCEL
#endif

/* MA__IFACE_LPT : définit les symboles/structures lpt_...
 * (compatible binaire avec la libpthread)
 * */
#ifdef MA__IFACE_LPT
#  undef MA__IFACE_LPT
#endif

/* MA__LIBPTHREAD : définit les fonctions pthread_...
 * et tout ce qu'il faut pour obtenir une libpthread
 * */
#ifdef MA__LIBPTHREAD
#  undef MA__LIBPTHREAD
#endif

/* MA__FUT_RECORD_TID : definit si on enregistre ou pas les tid dans les traces
 * */
#ifdef MA__FUT_RECORD_TID
#  undef MA__FUT_RECORD_TID
#endif

/* MA__PROVIDE_TLS : définit si on fournit le mécanisme de TLS pour l'application (et la glibc) */
#ifdef MA__PROVIDE_TLS
#  undef MA__PROVIDE_TLS
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

#if defined(MARCEL_NUMA)
#  define MA__NUMA
#endif

#ifdef MARCEL_POSIX
#  define MA__IFACE_PMARCEL
#endif

#ifdef MARCEL_LIBPTHREAD
#  define MA__IFACE_LPT
#  define MA__LIBPTHREAD
#  define MA__PROVIDE_TLS
#endif

#if defined(MA__LWPS) && !defined(MA__LIBPTHREAD)
#  define MA__INTERRUPTS_FIX_LWP
#endif

/* On peut ne pas vouloir de signaux quand même */
#if defined(MA_DO_NOT_LAUNCH_SIGNAL_TIMER) || defined(__MINGW32__)
#ifdef PM2_DEV
#warning NO SIGNAL TIMER ENABLE
#warning I hope you are debugging
#endif
#  undef MA__TIMER
#endif

/* Les tid sont en fait toujours enregistrés. On en a besoin pour détecter
 * les interruptions entre l'enregistrement du switch_to et le switch_to
 * effectif 
 * */
#define MA__FUT_RECORD_TID

/* Pour l'instant, toujours activer les bulles en NUMA */
#ifdef MA__NUMA
#  define MA__BUBBLES
#endif

#ifdef MA__HAS_SUBSECTION
#  undef MA__HAS_SUBSECTION
#endif

#if defined(LINUX_SYS) || defined(GNU_SYS)
#  define MA__HAS_SUBSECTION
#endif

#endif /* MARCEL_FLAGS_EST_DEF */

