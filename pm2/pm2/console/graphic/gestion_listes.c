
#include <xview/xview.h>
#include "structures.h"
#include "gestion_listes.h"

/* La macro suivante effectue l'allocation de la memoire
   pour la chaine 'x' et copie la chaine parametre 'x'
   dans le champ 'x' de la structure */
#define alloue_et_copie(x) pointeur->x = malloc (1+strlen(x)) ; strcpy(pointeur->x, x) ;

/* La macro suivante libere la memoire du champs 'x' */
#define libere(x) if (pointeur->x != NULL) {free(pointeur->x) ;}

/* threads */
p_thread new_thread (void)
{
  p_thread pointeur ;
  
  pointeur = (p_thread)malloc(sizeof (thread)) ;

  pointeur->thread_id  = (char *)NULL ;
  pointeur->num        = (char *)NULL ;
  pointeur->prio       = (char *)NULL ;
  pointeur->running    = (char *)NULL ;
  pointeur->stack      = (char *)NULL ;
  pointeur->migratable = (char *)NULL ;
  pointeur->service    = (char *)NULL ;

  pointeur->suivant    = (p_thread)NULL ;

  return pointeur ;
}


p_thread new_thread_with_args (char *thread_id,
			       char *num,
			       char *prio,
			       char *running,
			       char *stack,
			       char *migratable,
			       char *service
			       )
{
  p_thread pointeur ;

  pointeur = (p_thread)malloc(sizeof (thread)) ;

  alloue_et_copie (thread_id)
  alloue_et_copie (num)
  alloue_et_copie (prio)
  alloue_et_copie (running)
  alloue_et_copie (stack)
  alloue_et_copie (migratable)
  alloue_et_copie (service)

  pointeur->suivant = (p_thread) NULL ;

  return pointeur ;
}


p_thread delete_thread (p_thread pointeur)
{  
  libere (thread_id)
  libere (num)
  libere (prio)
  libere (running)
  libere (stack)
  libere (migratable)
  libere (service)

  {
    p_thread suivant ;
    suivant = pointeur->suivant ;
    free (pointeur) ;

    return suivant ;
  }
}


int numero_thread (p_thread pt_debut, p_thread pt)
{
  p_thread temp_pt ;
  int compteur ;
  
  temp_pt = pt_debut ;
  compteur = 0 ;

  while ((temp_pt != (p_thread)NULL) && (temp_pt != pt))
    {
      compteur ++ ;
      temp_pt = temp_pt->suivant ;
    }

  if (temp_pt == (p_thread)NULL)
    {
      return -1 ;
    }
  else
    {
      return compteur ;
    }
}


int compte_threads (p_thread pt)
{
  int compteur ;

  compteur = 0 ;

  while (pt != (p_thread)NULL)
    {
      compteur++ ;

      pt = pt->suivant ;
    }

  return compteur ;
}


p_thread renvoie_thread (p_thread pt, int n)
{
  if (n < 0)
    {
      return (p_thread)NULL ;
    }
  
  while (n > 0)
    {
      n-- ;
      
      if (pt == (p_thread)NULL)
	{
	  return (p_thread)NULL ;
	}
      else
	{
	  pt = pt->suivant ;
	}
    }

  return pt ;
}

  
/* Taches */
p_pvm_task
new_pvm_task (void)
{
  p_pvm_task pointeur ;

  pointeur = (p_pvm_task)malloc(sizeof(pvm_task)) ;

  pointeur->host = (char *)NULL ;
  pointeur->tid = (char *)NULL ;
  pointeur->command = (char *)NULL ;

  pointeur->nb_threads = 0 ;
  pointeur->selection_thread = (p_thread)NULL ;

  pointeur->xv_objects.thread_frame  = XV_NULL ;
  pointeur->xv_objects.canvas_thread = XV_NULL ;
  pointeur->xv_objects.conf_thread = XV_NULL ;
  pointeur->xv_objects.pan_thread = (Panel *)malloc(7 * sizeof(Panel)) ;

  {
    int i ;

    for (i = 0 ; i < 7 ; i++)
      {
	pointeur->xv_objects.pan_thread[i] = XV_NULL ;
      }
  }

  pointeur->xv_objects.thread_panel_cache = 1 ;
  pointeur->xv_objects.thread_frame_cache = 1 ;

  pointeur->xv_objects.dpy_thread = (Display *)NULL ;
  pointeur->xv_objects.xwin_thread = -1 ;

  /* pas d'initialisation pour les contextes graphiques */

  pointeur->old_selected_thread = (char *)NULL ;
  pointeur->threads = (p_thread)NULL ;

  pointeur->suivant = (p_pvm_task)NULL ;

  return pointeur ;
}


p_pvm_task new_pvm_task_with_args (char *host,
				   char *tid,
				   char *command
				   )
{
  p_pvm_task pointeur ;

  pointeur = (p_pvm_task)malloc(sizeof(pvm_task)) ;

  alloue_et_copie(host)
  alloue_et_copie(tid)
  alloue_et_copie(command)

  pointeur->nb_threads = 0 ;
  pointeur->selection_thread = (p_thread)NULL ;

  pointeur->xv_objects.thread_frame  = XV_NULL ;
  pointeur->xv_objects.canvas_thread = XV_NULL ;
  pointeur->xv_objects.conf_thread = XV_NULL ;
  pointeur->xv_objects.pan_thread = (Panel *)malloc(7 * sizeof(Panel)) ;

  {
    int i ;

    for (i = 0 ; i < 7 ; i++)
      {
	pointeur->xv_objects.pan_thread[i] = XV_NULL ;
      }
  }

  pointeur->xv_objects.thread_panel_cache = 1 ;
  pointeur->xv_objects.thread_frame_cache = 1 ;

  pointeur->xv_objects.dpy_thread = (Display *)NULL ;
  pointeur->xv_objects.xwin_thread = -1 ;

  /* pas d'initialisation pour les contextes graphiques */

  pointeur->old_selected_thread = (char *)NULL ;
  pointeur->threads = (p_thread)NULL ;

  pointeur->suivant = (p_pvm_task)NULL ;

  return pointeur ;
}


p_pvm_task delete_pvm_task (p_pvm_task pointeur)
{
  libere(host)
  libere(tid)
  libere(command)

  {
    int i ;

    for (i = 0 ; i < 7 ; i++)
      {
	if (pointeur->xv_objects.pan_thread[i] != XV_NULL)
	  {
	    xv_destroy (pointeur->xv_objects.pan_thread[i]) ;
	  }
      }
  }
  
  libere(xv_objects.pan_thread)
  
  if (pointeur->xv_objects.conf_thread != XV_NULL)
    {
      xv_destroy (pointeur->xv_objects.conf_thread) ;
    }

  if (pointeur->xv_objects.canvas_thread != XV_NULL)
    {
      xv_destroy (pointeur->xv_objects.canvas_thread) ;
    }

  if (pointeur->xv_objects.thread_frame != XV_NULL)
    {
      xv_destroy (pointeur->xv_objects.thread_frame) ;
    }

  libere(old_selected_thread) ;

  {
    p_thread thread_liste ;

    thread_liste = pointeur->threads ;

    while (thread_liste != NULL)
      {
	thread_liste = delete_thread (thread_liste) ;
      }
  }

  {
    p_pvm_task suivant ;

    suivant = pointeur->suivant ;

    free (pointeur) ; 

    return suivant ;
  }
}


int numero_pvm_task (p_pvm_task ppt_debut, p_pvm_task ppt)
{
  p_pvm_task temp_ppt ;
  int compteur ;
  
  temp_ppt = ppt_debut ;
  compteur = 0 ;

  while ((temp_ppt != (p_pvm_task)NULL) && (temp_ppt != ppt))
    {
      compteur ++ ;
      temp_ppt = temp_ppt->suivant ;
    }

  if (temp_ppt == (p_pvm_task)NULL)
    {
      return -1 ;
    }
  else
    {
      return compteur ;
    }
}


int compte_pvm_tasks (p_pvm_task ppt)
{
  int compteur ;

  compteur = 0 ;

  while (ppt != (p_pvm_task)NULL)
    {
      compteur++ ;

      ppt = ppt -> suivant ;
    }

  return compteur ;
}


p_pvm_task renvoie_pvm_task (p_pvm_task ppt, int n)
{
  if (n < 0)
    {
      return (p_pvm_task)NULL ;
    }
  
  while (n > 0)
    {
      n-- ;
      
      if (ppt == (p_pvm_task)NULL)
	{
	  return (p_pvm_task)NULL ;
	}
      else
	{
	  ppt = ppt->suivant ;
	}
    }

  return ppt ;
}


/* hosts */

p_host new_host(void)
{
  p_host pointeur ;

  pointeur = (p_host)malloc(sizeof(host)) ;

  pointeur->nom = (char *)NULL ;
  pointeur->dtid = (char *)NULL ;
  pointeur->archi = (char *)NULL ;
  pointeur->speed = (char *)NULL ;

  pointeur->nb_pvms = 0 ;
  pointeur->selection_pvm = (p_pvm_task)NULL ;
  pointeur->old_selected_task = (char *) NULL ;
  pointeur->pvm_tasks = (p_pvm_task)NULL ;

  pointeur->suivant = (p_host)NULL ;

  return pointeur ;
}


p_host new_host_with_args(char *nom,
			  char *dtid,
			  char *archi,
			  char *speed)
{
  p_host pointeur ;

  pointeur = (p_host)malloc(sizeof(host)) ;
  
  alloue_et_copie (nom)
  alloue_et_copie (dtid)
  alloue_et_copie (archi)
  alloue_et_copie (speed)
    
  pointeur->nb_pvms = 0 ;
  pointeur->selection_pvm = (p_pvm_task)NULL ;
  pointeur->old_selected_task = (char *)NULL ;
  pointeur->pvm_tasks = (p_pvm_task)NULL ;

  pointeur->suivant = (p_host)NULL ;

  return pointeur ;
}


p_host delete_host(p_host pointeur)
{
  libere (nom)
  libere (dtid)
  libere (archi)
  libere (speed)
  libere (old_selected_task)

  {
    p_pvm_task liste_tache ;

    liste_tache = pointeur->pvm_tasks ;

    while (liste_tache != NULL)
      {
	liste_tache = delete_pvm_task (liste_tache) ;
      }
  }

  {
    p_host suivant ;

    suivant = pointeur->suivant ;

    free(pointeur) ;

    return suivant ;
  }
}


int numero_host (p_host ph_debut, p_host ph)
{
  p_host temp_ph ;
  int compteur ;
  
  temp_ph = ph_debut ;
  compteur = 0 ;

  while ((temp_ph != (p_host)NULL) && (temp_ph != ph))
    {
      compteur ++ ;
      temp_ph = temp_ph->suivant ;
    }

  if (temp_ph == (p_host)NULL)
    {
      return -1 ;
    }
  else
    {
      return compteur ;
    }
}


int compte_hosts (p_host ph)
{
  int compteur ;

  compteur = 0 ;

  while (ph != (p_host)NULL)
    {
      compteur++ ;
      ph = ph->suivant ;
    }

  return compteur ;
}


p_host renvoie_host (p_host ph, int n)
{
  if (n < 0)
    {
      return (p_host)NULL ;
    }
  while (n > 0)
    {
      n-- ;
      
      if (ph == (p_host)NULL)
	{
	  return (p_host)NULL ;
	}
      else
	{
	  ph = ph->suivant ;
	}
    }

  return ph ;
}


