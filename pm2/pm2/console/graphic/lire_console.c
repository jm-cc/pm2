
#include "structures.h"
#include "gestion_listes.h"
#include "lire_console.h"

void extract_word(char *buf, char nom[128], int *i)
{
  int j = *i ;
  int k = 0;

	
  while((buf[j] == ' ') || (buf[j] == '\t'))
    {
      j++;
    }
  
  while((buf[j] != ' ') && (buf[j] != '\n') && (buf[j] != 0))
    {
      nom[k] = buf[j];
      j++;
      k++;
    }
  
  if (buf[j] == '\n')
    {
      j++;
    }
  
  nom[k] = 0; 
  *i = j;
}

/* extract_conf retourne un pointeur sur le
   debut de la nouvelle liste d'host */
p_host extract_conf(char *buf, p_host previous_hosts)
{
  int i;
  
  p_host new_hosts ;
  p_host *position ;
  
  char chaine[128];

  new_hosts = (p_host)NULL ;
  position = &(new_hosts) ;
  
  i=0; 

  do
    {
      extract_word(buf, chaine, &i);
    }
  while(strcmp(chaine, "SPEED") != 0) ;
  
  do
    {
      extract_word(buf, chaine, &i);

      if (strcmp (chaine, "pm2>") != 0)
	{
	  p_host current_host ;
	  char *nom  ;
	  char *dtid ;
	  char *archi ;
	  char *speed ;
	  
	  nom = (char *)malloc(1 + strlen(chaine)) ;
	  strcpy(nom, chaine);
	  
	  extract_word(buf, chaine, &i);
	  dtid = (char *)malloc(1 + strlen(chaine)) ;
	  strcpy(dtid, chaine);
	  
	  extract_word(buf, chaine, &i);
	  archi = (char *)malloc(1 + strlen(chaine)) ;
	  strcpy(archi, chaine);
	  
	  extract_word(buf, chaine, &i);
	  speed = (char *)malloc(1 + strlen(chaine)) ;
	  strcpy(speed, chaine);

	  /* Recherche de l'host dans l'ancienne liste */
	  {
	    p_host *p_ph_temp ;
	    p_host ph_temp ;

	    p_ph_temp = &previous_hosts ;
	    ph_temp = previous_hosts ;
	    current_host = (p_host) NULL ;

	    while (ph_temp != (p_host)NULL)
	      {
		if (strcmp(ph_temp->nom, nom) == 0)
		  {
		    current_host = ph_temp ;

		    /* enleve l'host de l'ancienne liste */
		    *p_ph_temp = ph_temp->suivant ;
			
		    break ;
		  }
		else
		  {
		    p_ph_temp = &(ph_temp->suivant);
		    ph_temp = ph_temp->suivant;
		  }
	      }
	  }

	  if (current_host == (p_host)NULL)
	    {
	      current_host = new_host_with_args (nom, dtid, archi, speed) ;
	    }

	  free (nom);
	  free (dtid);
	  free (archi);
	  free (speed);
	      
	  *position = current_host ;
	  position = &(current_host->suivant) ;
	}
    } 
  while(strcmp(chaine, "pm2>") != 0) ;
  
  /* efface les host restants de l'ancienne liste */
  {
    p_host ph_temp ;

    ph_temp = previous_hosts ;
    while (ph_temp != (p_host)NULL)
      {
	/* delete_host renvoie le champ 'suivant'
	   de la structure, ce qui est pratique ici */
	ph_temp = delete_host (ph_temp) ;
      }
  }

  /* retourne la nouvelle liste */
  return new_hosts ;
}

p_pvm_task extract_ps(char *buf, p_host ph, p_pvm_task previous_tasks)
{
  int i ;

  p_pvm_task new_pvm_tasks ;
  p_pvm_task *position ;
  
  char chaine[128] ;
  
  i=0;

  new_pvm_tasks = (p_pvm_task)NULL ;
  position = &(new_pvm_tasks) ;
  
  do
    {
      extract_word(buf, chaine, &i);
    }
  while(strcmp(chaine, "COMMAND") != 0);

  do
    {
      extract_word(buf, chaine, &i);
      
      if (strcmp(chaine, "pm2>") != 0)
	{
	  p_pvm_task current_pvm_task ;
	  char *host ;
	  char *tid ;
	  char *command ;
	  
	  if (strcmp(ph->nom, chaine) != 0)
	    {
	      /* on ne lit que les tache du host 'ph',
		 les autres sont ignoree, d'où les deux 'extract_word'
		 suivant, pour sauter les mots inutiles */
	      extract_word(buf, chaine, &i);
	      extract_word(buf, chaine, &i);
	      extract_word(buf, chaine, &i);
	      continue ;
	    }

	  host = (char *)malloc(1 + strlen (chaine)) ;
	  strcpy(host, chaine);
	  
	  extract_word(buf, chaine, &i);
	  tid = (char *)malloc(1 + strlen (chaine)) ;
	  strcpy(tid, chaine);
	  
	  extract_word(buf, chaine, &i);
	  command = (char *)malloc(1 + strlen (chaine)) ;
	  strcpy(command, chaine);
	  
	  /* Lecture vide pour sauter les champs inutiles */
	  extract_word(buf, chaine, &i);

	  /* Recherche de la tache dans l'ancienne liste */
	  {
	    p_pvm_task *p_ppt_temp ;
	    p_pvm_task ppt_temp ;

	    p_ppt_temp = &previous_tasks ;
	    ppt_temp = previous_tasks ;
	    current_pvm_task = (p_pvm_task) NULL ;

	    while (ppt_temp != (p_pvm_task)NULL)
	      {
		if (strcmp(ppt_temp->tid, tid) == 0)
		  {
		    current_pvm_task = ppt_temp ;

		    /* enleve la tache de l'ancienne liste */
		    *p_ppt_temp = ppt_temp->suivant ;
			
		    break ;
		  }
		else
		  {
		    p_ppt_temp = &(ppt_temp->suivant);
		    ppt_temp = ppt_temp->suivant;
		  }
	      }
	  }

	  if (current_pvm_task == (p_pvm_task)NULL)
	    {
	      current_pvm_task = new_pvm_task_with_args (host, tid, command) ;
	    }

	  free (host);
	  free (tid);
	  free (command);
	      
	  *position = current_pvm_task ;
	  position = &(current_pvm_task->suivant) ;
	}
    }
  while(strcmp(chaine, "pm2>") != 0);
  
  {
    p_pvm_task ppt_temp ;

    ppt_temp = previous_tasks;
    
    while (ppt_temp != (p_pvm_task)NULL)
      {
	ppt_temp = delete_pvm_task (ppt_temp) ;
      }
  }

  return new_pvm_tasks ;
}


p_thread extract_th(char *buf, p_pvm_task ppt, p_thread previous_threads)
{
  int i ;
  char chaine[128];

  p_thread new_threads ;
  p_thread *position ;  
  
  i=0;
  new_threads = NULL ;
  position = &(new_threads) ;

  do
    {
      extract_word(buf, chaine, &i);
    }
  while(strcmp(chaine, "SERVICE")) ;
	
  extract_word(buf, chaine, &i);

  if(strcmp(chaine, "pm2>") == 0)
    {
      /* efface la liste des threads */
      {
	p_thread pt_temp ;

	pt_temp = previous_threads ;

	while (pt_temp != (p_thread)NULL)
	  {
	    pt_temp = delete_thread (pt_temp);
	  }
      }
      
      return (p_thread)NULL ;
    }

  while(strcmp(chaine, ppt->tid) != 0) /* un peu dangereux si un tid est egal
					  à un thread_id, il me semble */
    {
      extract_word(buf, chaine, &i);
    }
	
  extract_word(buf, chaine, &i); /* ? */

  while(strcmp(chaine, "pm2>") != 0)
    {
      extract_word(buf, chaine, &i);
      
      if (strcmp(chaine, "pm2>") != 0)
	{
	  p_thread current_thread ;
	  char *thread_id;
	  char *num;
	  char *prio;
	  char *running;
	  char *stack;
	  char *migratable;
	  char *service;
	  
	  thread_id = (char *)malloc(1 + strlen (chaine));
	  strcpy(thread_id, chaine);

	  extract_word(buf, chaine, &i);
	  num = (char *)malloc(1 + strlen (chaine));
	  strcpy(num, chaine);

	  extract_word(buf, chaine, &i);
	  prio = (char *)malloc(1 + strlen (chaine));
	  strcpy(prio, chaine);

	  extract_word(buf, chaine, &i);
	  running = (char *)malloc(1 + strlen (chaine));
	  strcpy(running, chaine);

	  extract_word(buf, chaine, &i);
	  stack = (char *)malloc(1 + strlen (chaine));
	  strcpy(stack, chaine);

	  extract_word(buf, chaine, &i);
	  migratable = (char *)malloc(1 + strlen (chaine));
	  strcpy(migratable, chaine);

	  extract_word(buf, chaine, &i);
	  service = (char *)malloc(1 + strlen (chaine));
	  strcpy(service, chaine);

	  /* Recherche de la thread dans l'ancienne liste */
	  {
	    p_thread *p_pt_temp ;
	    p_thread pt_temp ;

	    p_pt_temp = &previous_threads ;
	    pt_temp = previous_threads ;
	    current_thread = (p_thread) NULL ;

	    while (pt_temp != (p_thread)NULL)
	      {
		if (strcmp(pt_temp->thread_id, thread_id) == 0)
		  {
		    current_thread = pt_temp ;

		    /* enleve la thread de l'ancienne liste */
		    *p_pt_temp = pt_temp->suivant ;
			
		    break ;
		  }
		else
		  {
		    p_pt_temp = &(pt_temp->suivant);
		    pt_temp = pt_temp->suivant;
		  }
	      }
	  }

	  if (current_thread == (p_thread)NULL)
	    {
	      current_thread = new_thread_with_args (thread_id,
						     num,
						     prio,
						     running,
						     stack,
						     migratable,
						     service) ;
	    }

	  free (thread_id);
	  free (num);
	  free (prio);
	  free (running);
	  free (stack);
	  free (migratable);
	  free (service);
	  
	  *position = current_thread ;
	  position = &(current_thread->suivant) ;
	}
    }

  {
    p_thread pt_temp ;

    pt_temp = previous_threads;
    
    while (pt_temp != (p_thread)NULL)
      {
	pt_temp = delete_thread (pt_temp) ;
      }
  }

  return new_threads ;
}
