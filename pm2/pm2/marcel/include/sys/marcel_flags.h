/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: marcel_flags.h,v $
Revision 1.9  2000/05/25 00:23:51  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.8  2000/05/09 14:37:31  vdanjean
minor bugs fixes

Revision 1.7  2000/04/21 11:19:26  vdanjean
fixes for actsmp

Revision 1.6  2000/04/17 16:09:38  vdanjean
clean up : remove __ACT__ flags and use of MA__ACTIVATION instead of MA__ACT when needed

Revision 1.5  2000/04/17 08:31:14  rnamyst
Changed DEBUG into MA__DEBUG.

Revision 1.4  2000/04/14 14:01:18  rnamyst
Minor modifs.

Revision 1.3  2000/04/14 11:41:47  vdanjean
move SCHED_DATA(lwp).sched_task to lwp->idle_task

Revision 1.2  2000/04/11 09:07:17  rnamyst
Merged the "reorganisation" development branch.

Revision 1.1.2.9  2000/04/08 15:09:12  vdanjean
few bugs fixed

Revision 1.1.2.8  2000/03/29 17:21:10  rnamyst
Defined the MA__TIMER flag in MONO & SMP modes.

Revision 1.1.2.7  2000/03/29 14:24:39  rnamyst
Added the marcel_stdio.h that provides the marcel_printf functions.
=> MTRACE now uses marcel_fprintf.

Revision 1.1.2.6  2000/03/29 09:46:18  vdanjean
*** empty log message ***

Revision 1.1.2.5  2000/03/27 09:24:14  vdanjean
bug fix

Revision 1.1.2.4  2000/03/24 17:55:14  vdanjean
fixes

Revision 1.1.2.3  2000/03/17 20:09:50  vdanjean
*** empty log message ***

Revision 1.1.2.2  2000/03/15 15:54:54  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.1.2.1  2000/03/15 15:41:16  vdanjean
réorganisation de marcel. branche de développement

Revision 1.1  2000/03/06 14:56:04  rnamyst
Modified to include "marcel_flags.h".

______________________________________________________________________________
*/

#ifndef MARCEL_FLAGS_EST_DEF
#define MARCEL_FLAGS_EST_DEF

#ifdef MARCEL_DEBUG
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

/* MA__LWPS : indique que l'on a plusieurs entités ordonnancées par
 * le noyau en parallèle.
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
 * marcel en exécution dans une queue
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
 * pour la préemption. (ne pas valider avec les activations)
 * */
#ifdef MA__TIMER
#undef MA__TIMER
#endif

/* MA__WORK : utilisation des files de travail à la fin des unlock_task()
 * */
#ifdef MA__WORK
#undef MA__WORK
#endif

#ifdef MARCEL_MONO /* Marcel Mono */
#define MA__ONE_QUEUE
#define MA__TIMER
#endif /* Fin Marcel Mono */

#ifdef MARCEL_SMP /* Marcel SMP */
#define SMP
#define MA__SMP /* Utilisation des pthreads pour les lwp */
#define MA__LWPS
#define MA__TIMER
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

