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
*/

#ifndef __gestion_liste_h__
#define __gestion_liste_h__

#include "structures.h"

/* threads */
p_thread new_thread (void) ;

p_thread new_thread_with_args (char *thread_id,
			       char *num,
			       char *prio,
			       char *running,
			       char *stack,
			       char *migratable,
			       char *service
			       );

p_thread delete_thread (p_thread pointeur);

int numero_thread (p_thread pt_debut, p_thread pt);

int compte_threads (p_thread pt);

p_thread renvoie_thread (p_thread pt, int n);


/* Taches */
p_pvm_task new_pvm_task (void);

p_pvm_task new_pvm_task_with_args (char *host,
				   char *tid,
				   char *command
				   );

p_pvm_task delete_pvm_task (p_pvm_task pointeur);

int numero_pvm_task (p_pvm_task ppt_debut, p_pvm_task ppt);

int compte_pvm_tasks (p_pvm_task ppt);

p_pvm_task renvoie_pvm_task (p_pvm_task ppt, int n);


/* hosts */
p_host new_host(void) ;

p_host new_host_with_args(char *nom,
			  char *dtid,
			  char *archi,
			  char *speed);

p_host delete_host(p_host pointeur) ;

int numero_host (p_host ph_debut, p_host ph) ;

int compte_hosts (p_host ph);

p_host renvoie_host (p_host ph, int n);

#endif
