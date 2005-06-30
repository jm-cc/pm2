
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
