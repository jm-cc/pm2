
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
#define MA__DEBUG
#endif

#ifdef __ACT__
#error use of __ACT__ depreciated. Define MARCEL_ACT instead
#undef __ACT__
#define MARCEL_ACT
#endif

#ifdef SMP
#error use of SMP depreciated. Define MARCEL_SMP instead
#undef SMP
#define MARCEL_SMP
#endif

#ifdef MARCEL_SMP
#undef MARCEL_SMP
#define MARCEL_SMP 1
#endif

#ifdef MARCEL_ACT
#undef MARCEL_ACT
#define MARCEL_ACT 1
#endif

#ifdef MARCEL_ACTSMP
#undef MARCEL_ACTSMP
#define MARCEL_ACTSMP 1
#endif

#ifdef MARCEL_MONO
#undef MARCEL_MONO
#define MARCEL_MONO 1
#endif

#if ((MARCEL_SMP + MARCEL_ACT + MARCEL_ACTSMP + MARCEL_MONO) == 0)
#error No MARCEL_... defined. Choose between MONO SMP ACT and ACTSMP
//#define MARCEL_MONO 1
#endif

#if ((MARCEL_SMP + MARCEL_ACT + MARCEL_ACTSMP + MARCEL_MONO) > 1)
#error You must define at most one of MARCEL_SMP, MARCEL_ACT, MARCEL_ACTSMP or MARCEL_MONO
#endif

/* MA__LWPS : indique que l'on a plusieurs entit�s ordonnanc�es par
 * le noyau en parall�le.
 *
 *  Cela arrive en SMP ou en Activation sur multiprocesseur 
 * */
#ifdef MA__LWPS
#undef MA__LWPS
#endif

/* MA__ONE_QUEUE : indique qu'il n'y a qu'une seule queue pour les threads
 * marcels pour tous les lwps
 *
 * C'est le cas tout le temps sauf en SMP
 * */
#ifdef MA__ONE_QUEUE
#undef MA__ONE_QUEUE
#endif

/* MA__MULTIPLE_RUNNING : indique qu'il peut y avoir plusieurs threads
 * marcel en ex�cution dans une queue
 *
 * C'est le cas avec les Activations
 * */
#ifdef MA__MULTIPLE_RUNNING
#undef MA__MULTIPLE_RUNNING
#endif

/* MA__ACTIVATION : indique qu'on utilise les activations (mono ou
 * smp). On peut donc utiliser les constantes de act.h
 * */
#ifdef MA__ACTIVATION
#undef MA__ACTIVATION
#endif

/* MA__TIMER : indique qu'il faut utiliser le timer unix (signaux)
 * pour la pr�emption. (ne pas valider avec les activations)
 * */
#ifdef MA__TIMER
#undef MA__TIMER
#endif

/* MA__WORK : utilisation des files de travail � la fin des unlock_task()
 * */
#ifdef MA__WORK
#undef MA__WORK
#endif

#ifdef MARCEL_MONO /* Marcel Mono */
#define MA__MONO
#define MA__ONE_QUEUE
#define MA__TIMER
#endif /* Fin Marcel Mono */

#ifdef MARCEL_SMP /* Marcel SMP */
#define MA__SMP /* Utilisation des pthreads pour les lwp */
#define MA__LWPS
#define MA__TIMER
#define MA__WORK
#ifndef SMP_MULTIPLE_QUEUES
#define MA__ONE_QUEUE
#define MA__MULTIPLE_RUNNING
#endif

#endif /* Fin Marcel SMP */

#if defined(MARCEL_ACT) || defined(MARCEL_ACTSMP) /* Marcel Activation */
#define MA__ONE_QUEUE
#define MA__MULTIPLE_RUNNING
#define MA__ACTIVATION
#define ACTIVATION
#define MA__WORK
#endif /* Fin Marcel Activation */

#ifdef MARCEL_ACT /* Marcel Activation Mono */
#define MA__ACT
#undef CONFIG_SMP
#endif /* Fin Marcel Activation Mono */

#ifdef MARCEL_ACTSMP /* Marcel Activation SMP */
#define MA__ACTSMP
#define CONFIG_SMP
#define __SMP__
#define MA__LWPS
#endif /* Fin Marcel Activation SMP */

#endif

