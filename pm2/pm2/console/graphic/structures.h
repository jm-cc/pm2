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


#ifndef __structures_h__
#define __structures_h__

#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/canvas.h>

/* structures de memorisation des objets de base de Xpm2 : hotes, taches et threads */

/* Definition de la liste de Threads */
struct s_thread
{
  char *thread_id ; /* identificateur */
  char *num ; /* numero */
  char *prio ; /* priorite d'execution */
  char *running ; /* etat : actif ou non */
  char *stack ; /* taille de la pile */
  char *migratable ; /* etat : migrable ou non */
  char *service ; /* nom du service */ 
  
  struct s_thread *suivant ;
} ;

typedef struct s_thread thread, *p_thread ;

/* Definition de la liste des Taches PVM */

struct s_threads_xview_objects
{
  Frame thread_frame ; /* cadre XView contenant les icones Threads de la tache */
  Canvas canvas_thread ; /* canevas d'affichage des threads */
  Frame conf_thread ; /* panneau d'information sur la thread selectionnee */
  Panel *pan_thread; /* messages affiches dans le panneau d'information */

  int thread_panel_cache ; /* indique si le panneau d'informations est affiche */
  int thread_frame_cache ; /* indique si le cadre est actuellement affiche ou non */

  Display *dpy_thread ; /* display de la fenetre des threads */
  Window xwin_thread ; /* fenetre des threads */
  GC gc_thread, gc_rect_thread ; /* contextes graphiques */
} ;

typedef struct s_threads_xview_objects threads_xview_objects, *p_threads_xview_objects ;

struct s_pvm_task
{
  char *host ; /* hote associe */
  char *tid ;  /* identificateur */
  char *command ; /* commande */
  
  int nb_threads ; /* nombre de threads dans la tache */
  p_thread selection_thread ; /* Numero de la thread selectionnee */

  threads_xview_objects xv_objects ;
  
  char *old_selected_thread; /* thread selectionnee precedemment */
  p_thread threads ; /* liste chainee des threads */

  struct s_pvm_task *suivant ;
} ;

typedef struct s_pvm_task pvm_task, *p_pvm_task;


/* Definition de la liste des Hosts  */

/* liste chainee des 'hosts' */
struct s_host
{
  /* donnees du noeud host */
  /* Ne pas oublier d'allouer la memoire pour les chaines de donnees dorenavant */
  
  char *nom ;   /* nom du 'host'  */
  char *dtid ;  /* identificateur */
  char *archi ; /* architecture   */
  char *speed ; /* vitesse        */


  int nb_pvms ; /* nombre de taches dans l'host */
  p_pvm_task selection_pvm ; /* tache pvm selectionnee dans l'host */
  p_pvm_task pvm_tasks ; /* Liste chainee des taches pvm */

  char *old_selected_task ;

  struct s_host *suivant ;
} ;

typedef struct s_host host, *p_host ;

#endif
