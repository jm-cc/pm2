
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
