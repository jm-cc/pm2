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

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/scrollbar.h>
#include <xview/xv_xrect.h>
#include <xview/textsw.h>
#include <xview/tty.h>
#include <xview/termsw.h>
#include <xview/notify.h>
#include <xview/notice.h>

#include "structures.h"     /* structures de donnees de l'application */
#include "gestion_listes.h" /* fonctions de gestion des listes chainees */
#include "lire_console.h"   /* fonctions d'acces a la console PM2 */


/* macrocommandes */
#define min(a, b)	((a) < (b) ? (a) : (b))


/* fin des macrocommandes */



/* constantes */
enum
{
  S_INITIAL_STATE,
  S_ANY,
  S_ADD,
  S_CONF,
  S_DELETE,
  S_HALT,
  S_QUIT,
  S_PS, 
  TH,
  S_KILL,
  S_MIGR,
  S_FREEZE,
  USER
};

#define MAX_VIEWABLE_HOST	32
#define MAX_VIEWABLE_TASK	10
#define MAX_VIEWABLE_THREAD	112

#define MAX_USER_COMMANDS	16


/* fin des constantes */



/* types */
typedef char* (*f_host_view_func)(p_host);
typedef char* (*f_task_view_func)(p_pvm_task);
typedef char* (*f_thread_view_func)(p_thread);

typedef char serpent_de_panier[32];

struct
{
  char archi[32];
  char task_id[32];
  int nb;
  serpent_de_panier thread[50];
} panier;



/* fin des types */



/* Prototypes de fonctions */
static void redessiner_thread(Canvas,
			      Xv_Window,
			      Display *,
			      Window,
			      Xv_xrectlist *) ;

static void ma_procedure_thread(Xv_Window,
				Event *,
				Notify_arg) ;

char *host_by_name(p_host);
char *host_by_dtid(p_host);
char *host_by_arch(p_host);

char *task_by_name(p_pvm_task);
char *task_by_tid(p_pvm_task);

char *thread_by_pid(p_thread);
char *thread_by_service(p_thread);
char *thread_by_prio(p_thread);

void exec_command(int,char *);


/* fin des prototypes de fonctions */



/* Variables statiques de l'application */

/* structure de donnee principale */
int nb_hosts = 0 ; /* nombre d'hosts */
p_host hosts = (p_host)NULL ; /* liste chainee contenant les hosts */
p_host selection_host = (p_host)NULL ; /* host selectionne */

/* Tube de communication */
static int pipe_io[2][2];

/* icones */
static char icon_host[128];
static char icon_task[128];
static char icon_thread[128];
static char icon_boulet[128];
static char icon_endormi[128];
static char icon_panier[128];
static char icon_panier_ouvert[128];

static char *my_argv[] = { "pm2", NULL };
static int longueur = 0, etat;

/* etat des panneaux d'information */
static int fenetre_cache_host = 1;
static int fenetre_cache_pvm = 1;
static int fenetre_cache_panier = 1;

/* variables de gestion X11 */
static int     gcCreated = False;
static Display *dpy1;
static Window  xwin1;

/* Objets d'interface XView */
static Panel controle ;
static Panel mes2 ;
static Panel controle1 ;
static Panel controle2 ;
static Panel controle3 ; 
static Panel controle4 ;
static Panel text_saisi ;
static Panel Pan_Host[4] ;
static Panel Pan_Pvm[6] ; 

static Panel_item refresh_item ;
static Panel_item panier_count_item ;
static Panel_item panier_archi_item ;
static Panel_item host_count_item ;
static Panel_item task_count_item ;
static Panel_item thread_count_item ;
static Panel_item user_text ;
static Panel_item user_button;

static Frame racine ;
static Frame xv_add_host ;
static Frame conf_host = 0 ;
static Frame conf_pvm = 0 ;
static Frame a_propos ;
static Frame panier_frame = 0;

static Menu menu ;
static Menu menu_host ;
static Menu menu_pvm ;
static Menu menu_thread ;
static Menu user_menu;

static Xv_opaque window1_image;

static unsigned short window1_bits[] = {
  /* Format_version=1, Width=64, Height=64, Depth=1, Valid_bits_per_item=16
   */
  0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x0000,	0x0000,	0x0000,	0x0000,
  0x0000,	0x0000,	0x0000,	0x0000,
  0x0000,	0x0000,	0x0000,	0x0000,
  0x0000,	0x0000,	0x0000,	0x0000,
  0xFC01,	0xF800,	0x0000,	0x0000,
  0x7E03,	0xF000,	0x0000,	0x0000,
  0x7E03,	0xF000,	0x0000,	0x0018,
  0x3F07,	0xE000,	0x0000,	0x003C,
  0x3F07,	0xE000,	0x0000,	0x0066,
  0x1F8F,	0xC000,	0x0000,	0x00C6,
  0x1F8F,	0xC000,	0x0000,	0x0006,
  0x0FDF,	0x8000,	0x0000,	0x000C,
  0x0FDF,	0x8000,	0x0000,	0x0038,
  0x07FF,	0x0000,	0x0000,	0x0060,
  0x07FF,	0x0000,	0x0000,	0x00C0,
  0x03FE,	0x03FE,	0x07C0,	0x01FC,
  0x03FE,	0x0201,	0x8470,	0x63F8,
  0x01FC,	0x0200,	0x444D,	0x9800,
  0x01FC,	0x0200,	0x2402,	0x0400,
  0x01FC,	0x023C,	0x2400,	0x0200,
  0x03FE,	0x0222,	0x1400,	0x0100,
  0x03FE,	0x0221,	0x1430,	0x6100,
  0x07FF,	0x0221,	0x1448,	0x9100,
  0x07FF,	0x0221,	0x1448,	0x9100,
  0x0FDF,	0x8221,	0x1448,	0x9100,
  0x0FDF,	0x8221,	0x1448,	0x9100,
  0x1F8F,	0xC221,	0x1448,	0x9100,
  0x1F8F,	0xC222,	0x1448,	0x9100,
  0x3F07,	0xE23C,	0x2448,	0x9100,
  0x3F07,	0xE200,	0x2448,	0x9100,
  0x7E03,	0xF200,	0x4448,	0x9100,
  0x7E03,	0xF201,	0x8448,	0x9100,
  0xFC01,	0xFA3E,	0x07CF,	0x9F00,
  0x0000,	0x0220,	0x0000,	0x0000,
  0x0000,	0x0220,	0x0000,	0x0000,
  0x0000,	0x0220,	0x0000,	0x0000,
  0x8000,	0x0220,	0x0000,	0x0001,
  0x8000,	0x0220,	0x0000,	0x0001,
  0x8000,	0x0220,	0x0000,	0x0001,
  0x8000,	0x0220,	0x0000,	0x0001,
  0x8000,	0x0220,	0x0000,	0x0001,
  0x8000,	0x0220,	0x0000,	0x0001,
  0x8000,	0x0220,	0x0000,	0x0001,
  0x8000,	0x0220,	0x0000,	0x0001,
  0x8000,	0x0220,	0x0000,	0x0001,
  0x8000,	0x0220,	0x0000,	0x0001,
  0x8000,	0x03E0,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0x8000,	0x0000,	0x0000,	0x0001,
  0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
};


static Canvas canvas ;
static Canvas canvas_pvm ;
static Canvas canvas_panier ;

static Tty ttysw;
static int tty_fd;

static Display *dpy_host ;
static Display *dpy_pvm ;
static Display *dpy_panier;

static Window xwin_host ;
static Window xwin_pvm ;
static Window xwin_panier;

static GC gc ;
static GC gc_rectangle ;
static GC gc_rect_pvm ;
static GC gc_pvm ; 
static GC gc_panier;

static XGCValues gcv;

static unsigned long gcmask;
 
static int largeur ;
static int hauteur;

static Pixmap img_host[4] ;
static Pixmap img_pvm_task ;
static Pixmap img_thread ;
static Pixmap img_boulet ;
static Pixmap img_endormi ;
static Pixmap img_panier ;
static Pixmap img_panier_ouvert;

static char *old_selected_host = NULL ;

static int verbose_mode = 0;

static struct itimerval refresh_timer ;

/* refreshing period in seconds : */
#define REFRESH_INTERVAL	5

static int automatic_refresh = FALSE ;

static f_host_view_func host_view_func	= &host_by_name ;
static f_task_view_func task_view_func	= &task_by_tid ;
static f_thread_view_func thread_view_func = &thread_by_pid ;

/* buffer */
static unsigned long MAX_BUF = 65000;
static char *buf = (char *)NULL;
static int offset = 0;


/* fin des variables statiques */




/* definition des fonctions */
/*==========================*/

/* gestion du Timer de rafraichissement */

void stop_timer()
{
  automatic_refresh = FALSE;

  notify_set_itimer_func
    (racine,
     NOTIFY_FUNC_NULL,
     ITIMER_REAL, NULL,
     NULL);

  xv_set(refresh_item, PANEL_VALUE, 0, NULL);
}



/* Fonctions de visualisation */

char *host_by_name(p_host ph)
{
  return ph->nom ;
}

char *host_by_dtid(p_host ph)
{
  return ph->dtid ;
}

char *host_by_arch(p_host ph)
{
  return ph->archi;
}


char *task_by_name(p_pvm_task ppt)
{
  return ppt->command;
}

char *task_by_tid(p_pvm_task ppt)
{
  return ppt->tid;
}


char *thread_by_pid(p_thread pt)
{
  return pt->thread_id;
}

char *thread_by_service(p_thread pt)
{
  return pt->service;
}

char *thread_by_prio(p_thread pt)
{
  return pt->prio;
}



void by_hname_proc()
{
  host_view_func = host_by_name;
  xv_set(racine, FRAME_BUSY, TRUE, NULL);
  exec_command(S_CONF, NULL);
}

void by_dtid_proc()
{
  host_view_func = host_by_dtid;
  xv_set(racine, FRAME_BUSY, TRUE, NULL);
  exec_command(S_CONF, NULL);
}

void by_arch_proc()
{
  host_view_func = host_by_arch;
  xv_set(racine, FRAME_BUSY, TRUE, NULL);
  exec_command(S_CONF, NULL);
}


void by_name_proc()
{
  task_view_func = task_by_name;
  xv_set(racine, FRAME_BUSY, TRUE, NULL);
  exec_command(S_CONF, NULL);
}

void by_tid_proc()
{
  task_view_func = task_by_tid;
  xv_set(racine, FRAME_BUSY, TRUE, NULL);
  exec_command(S_CONF, NULL);
}


void by_pid_proc()
{
  thread_view_func = thread_by_pid;
  xv_set(racine, FRAME_BUSY, TRUE, NULL);
  exec_command(S_CONF, NULL);
}

void by_service_proc()
{
  thread_view_func = thread_by_service;
  xv_set(racine, FRAME_BUSY, TRUE, NULL);
  exec_command(S_CONF, NULL);
}

void by_prio_proc()
{
  thread_view_func = thread_by_prio;
  xv_set(racine, FRAME_BUSY, TRUE, NULL);
  exec_command(S_CONF, NULL);
}


void maj_host_count(void)
{
  char message[64];

  sprintf(message, "%d host(s)", nb_hosts);
  xv_set(host_count_item, PANEL_LABEL_STRING, message, NULL);
}

void maj_task_count(p_host ph)
{
  char message[64];

  if (ph == (p_host)NULL)
    {
      sprintf(message, "%d task", 0) ;
    }
  else
    {
      sprintf(message, "%d task(s)", ph->nb_pvms) ;
    }
  
  xv_set(task_count_item, PANEL_LABEL_STRING, message, NULL);
}




/* Mise a jour du panneau  Host */
/*-----------------------====-*/
void mettre_a_jour_host(p_host ph)
{
  char message[128];

  if(fenetre_cache_host == 0)
    {

      if(ph == (p_host)NULL)
	{
	  int i;

	  xv_set(Pan_Host[0], PANEL_LABEL_STRING, "*** No Host selected ***", NULL);
	  
	  for(i = 1 ; i < 3 ; i++)
	    {
	      xv_set(Pan_Host[i], PANEL_LABEL_STRING, "", NULL);
	    }
	}
      else
	{
	  sprintf(message,"Host Name : %s", ph->nom);
	  xv_set(Pan_Host[0],
		 PANEL_LABEL_STRING, message,
		 NULL);

	  sprintf(message,"DTID : %s", ph->dtid);
	  xv_set(Pan_Host[1],
		 PANEL_LABEL_STRING, message,
		 NULL);

	  sprintf(message,"Archi : %s", ph->archi);
	  xv_set(Pan_Host[2],
		 PANEL_LABEL_STRING, message,
		 NULL);
	}
    }
}


/* Mise a jour du panneau  PVM */
/*-----------------------===-*/
void mettre_a_jour_pvm(p_pvm_task ppt)
{
  char message[128];

  if(fenetre_cache_pvm == 0) {

    if(ppt == (p_pvm_task)NULL)
      {
	int i;

	xv_set(Pan_Pvm[0],
	       PANEL_LABEL_STRING, "*** No task selected ***",
	       NULL);
	
	for(i = 1 ; i < 3 ; i++)
	  {
	    xv_set(Pan_Pvm[i], PANEL_LABEL_STRING, "", NULL);
	  }
      }
    else
      {
	sprintf(message,"Host Name : %s", ppt->host);
	xv_set(Pan_Pvm[0],
	       PANEL_LABEL_STRING, message,
	       NULL) ;

	sprintf(message,"TID : %s", ppt->tid);
	xv_set(Pan_Pvm[1],
	       PANEL_LABEL_STRING, message,
	       NULL) ;

	sprintf(message,"COMMAND : %s", ppt->command);
	xv_set(Pan_Pvm[2],
	       PANEL_LABEL_STRING, message,
	       NULL);
      }
  }
}

/* Mise a jour du panneau  Thread(s) */
/*-----------------------===========-*/
void mettre_a_jour_thread(p_pvm_task ppt, p_thread pt)
{
  char message[128];

  if (ppt == (p_pvm_task)NULL)
    {
      return ;
    }
  
  if(ppt->xv_objects.thread_panel_cache == 0)
    {
      if(pt == (p_thread)NULL)
	{
	  int i;

	  xv_set(ppt->xv_objects.pan_thread[0],
		 PANEL_LABEL_STRING, "*** No thread selected ***",
		 NULL);
	  for(i = 1 ; i < 7 ; i++)
	    {
	      xv_set(ppt->xv_objects.pan_thread[i],
		     PANEL_LABEL_STRING, "",
		     NULL);
	    }
	}
      else
	{
	  sprintf(message, "Thread_ID : %s", pt->thread_id);
	  xv_set(ppt->xv_objects.pan_thread[0],
		 PANEL_LABEL_STRING, message,
		 NULL);

	  sprintf(message, "NUM : %s", pt->num);
	  xv_set(ppt->xv_objects.pan_thread[1],
		 PANEL_LABEL_STRING, message,
		 NULL);

	  sprintf(message, "PRIO : %s", pt->prio);
	  xv_set(ppt->xv_objects.pan_thread[2],
		 PANEL_LABEL_STRING, message,
		 NULL);

	  sprintf(message, "RUNNING : %s", pt->running);
	  xv_set(ppt->xv_objects.pan_thread[3],
		 PANEL_LABEL_STRING, message,
		 NULL);

	  sprintf(message, "STACK SIZE : %s", pt->stack);
	  xv_set(ppt->xv_objects.pan_thread[4],
		 PANEL_LABEL_STRING, message,
		 NULL);

	  sprintf(message, "MIGRATABLE : %s", pt->migratable);
	  xv_set(ppt->xv_objects.pan_thread[5],
		 PANEL_LABEL_STRING, message,
		 NULL);

	  sprintf(message, "SERVICE : %s", pt->service);
	  xv_set(ppt->xv_objects.pan_thread[6],
		 PANEL_LABEL_STRING, message,
		 NULL);
	}
    }
}

/* Decodage des commandes */
/*------------------------*/
void exec_command(int command, char *param)
{
  char comm[128];

  switch(command)
    {

    case S_ADD :
      {
	sprintf (comm, "add %s\n", param) ;
	etat = S_ADD;

	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }
	
	write(pipe_io[0][1], comm, strlen(comm));
      }
    break;

    case S_CONF :
      {
	sprintf (comm, "conf\n") ;
	etat = S_CONF;

	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }
	
	write(pipe_io[0][1], comm, strlen(comm));
      }
    break;

    case S_DELETE :
      {
	sprintf (comm, "delete %s\n", param) ;
	etat = S_DELETE;

	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }

	write(pipe_io[0][1],comm,strlen(comm));
      }
    break;
    
    case S_HALT :
      {
	sprintf (comm, "halt\n") ;
	etat = S_HALT;

	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }
	
	write(pipe_io[0][1], comm, strlen(comm));
      }
    break;

    case S_QUIT :
      {
	sprintf (comm, "quit\n") ;
	etat = S_QUIT;
	
	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }
	
	write(pipe_io[0][1], comm, strlen(comm));
      }
    break;
    
    case S_PS :
      {
	sprintf (comm, "ps a\n") ;
	etat = S_PS;

	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }

	write(pipe_io[0][1], comm, strlen(comm));
      }
    break;
    
    case TH :
      {
	sprintf (comm, "th %s\n", param) ;
	etat = TH;
	
	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }
	
	write(pipe_io[0][1], comm, strlen(comm));
      }
    break;
    
    case S_KILL :
      {
	sprintf (comm, "kill %s\n", param) ;
	etat = S_KILL;
	
	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }
	
	write(pipe_io[0][1], comm, strlen(comm));
      }
    break;
    
    case S_FREEZE :
      {
	sprintf (comm, "freeze %s\n", param) ;
	etat = S_FREEZE;
	
	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }
	
	write(pipe_io[0][1], comm, strlen(comm));
      }
    break;
    
    case S_MIGR :
      {
	sprintf (comm, "mig %s\n", param) ;
	etat = S_MIGR;
	
	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }
	
	write(pipe_io[0][1], comm, strlen(comm));
      }
    break;
    
    case USER :
      {
	sprintf (comm, "%s\n", param) ;
	etat = USER;
	
	if(verbose_mode)
	  {
	    write(tty_fd, comm, strlen(comm));
	  }
	
	write(pipe_io[0][1], comm, strlen(comm));
      }
    break;
    
    }
}


void user_results_done_proc(Frame subframe)
{
  Textsw t;

  t = (Textsw)xv_get(subframe, XV_KEY_DATA, 0);
  
  textsw_reset(t, 0, 0);

  xv_destroy_safe(subframe);
}



Notify_value read_it(Notify_client client, int fd)
{
  int bytes ;
  int i ;
  int x ;
  int x_rect ;
  int y ;
  int y_rect ;
  int ligne_courante ;
  int j ;
  int compte ;
  int res, retrouve;

  /* Etrange : il semble que cette instruction aille plutot
     dans le do..while suivant */
  if(offset == MAX_BUF)
    {
      MAX_BUF += 10000;
      buf = (char *)realloc(buf, MAX_BUF+1) ;
    }

  do
    {
      if ((i = read(fd, buf + offset, MAX_BUF - offset)) < 0)
	{
	  fprintf(stderr, "Error : read failed !\n");
	  exit(1);
	}

      offset += i;
    }
  while(i == MAX_BUF);


  buf[offset] = 0 ; /* Marque d'arret */

  if ((etat != S_QUIT) && (etat != S_HALT))
    {
      char *fin = buf + offset - 4;
      
      if ((offset < 4) || (strcmp(fin, "pm2>"))) /* Bizarre : ne faudrait-il pas
					       plutot strcmp (...) == 0 ? */
	{
	  return NOTIFY_DONE;
	}
    }

  if((etat == S_ANY) || (verbose_mode))
    {
      (void)write(tty_fd, buf, offset) ;
    }

  switch(etat)
    {

    case S_INITIAL_STATE :
      {
	xv_set(racine, FRAME_BUSY, TRUE, NULL);
	exec_command(S_CONF, NULL);
	
	if(verbose_mode)
	  {
	    char prompt[4]="pm2>";
	    write(tty_fd, prompt, strlen(prompt));
	  }
      }
    break ;
    
    case S_ANY :
      {
	/* instruction vide */
      }
    break ;
    
    case S_CONF :
      {
	if (old_selected_host != (char *)NULL)
	  {
	    free (old_selected_host) ;
	  }
	
	if(selection_host != (p_host)NULL)
	  {
	    old_selected_host = malloc (1 + strlen(selection_host->nom)) ;
	    strcpy(old_selected_host, selection_host->nom) ;
	  }
		
	selection_host = (p_host)NULL ;

	hosts = extract_conf(buf, hosts); /* extraction des donnees 'host' */
	XClearWindow(dpy_host, xwin_host);

	{
	  p_host ph ;
	  
	  for (i = 0, ph = hosts ;
	       (ph != (p_host)NULL) && (i < MAX_VIEWABLE_HOST) ;
	       i++, ph = ph->suivant)
	    {
	      x = (i * 64) + ((i + 1) * 10);
	      
	      XCopyPlane(dpy_host,
			 img_host[0],
			 xwin_host,
			 gc,
			 0, 0,
			 64, 64,
			 x, 10,
			 1);
	      
	      XDrawString(dpy_host,
			  xwin_host,
			  gc,
			  x, 95,
			  host_view_func(ph),
			  strlen(host_view_func(ph)));
		    
	      if (    (old_selected_host != (char *)NULL)
		   && (strcmp(ph->nom, old_selected_host) == 0))
		{
		  selection_host = ph ;
		}
	    }
	}

	nb_hosts = compte_hosts (hosts);
	
	maj_host_count();

	mettre_a_jour_host(selection_host);
		
	if(selection_host == (p_host) NULL)
	  {
	    XClearWindow(dpy_pvm, xwin_pvm);
	    
	    maj_task_count(selection_host);
	    mettre_a_jour_pvm((p_pvm_task)NULL);
	    
	    xv_set(racine, FRAME_BUSY, FALSE, NULL);
	    etat = S_ANY;
	  }
	else
	  {
	    int num ;

	    num = numero_host (hosts, selection_host) ;
	    x_rect = (num * 64) + (num * 10) + 7 ;

	    XDrawRectangle(dpy_host,
			   xwin_host,
			   gc_rectangle,
			   x_rect, 5,
			   70, 72);
	    
	    exec_command(S_PS, NULL);
	  }
      }
    /* fin S_CONF */
    break;
      
    case S_DELETE :
      {
	exec_command(S_CONF, NULL);
      }
    break;
    
    case S_ADD :
      {
	exec_command(S_CONF, NULL);
      }
    break;

		
    case S_PS : /* Mise a jour des Taches */
      {
	if (selection_host == (p_host)NULL)
	  {
	    break ;
	  }

	
	if (selection_host->old_selected_task != (char *)NULL)
	  {
	    free (selection_host->old_selected_task) ;
	  }
	
	if(selection_host->selection_pvm != (p_pvm_task)NULL)
	  {
	    selection_host->old_selected_task =
	      malloc (1 + strlen(selection_host->selection_pvm->tid)) ;
	    strcpy(selection_host->old_selected_task,
		   selection_host->selection_pvm->tid) ;
	  }
		
	selection_host->selection_pvm = (p_pvm_task)NULL;

	selection_host->pvm_tasks = extract_ps(buf,
					       selection_host,
					       selection_host->pvm_tasks);
	XClearWindow(dpy_pvm, xwin_pvm);

	compte = 0;

	{
	  p_pvm_task ppt ;	  
	  
	  for (i = 0, ppt = selection_host->pvm_tasks ;
	       (ppt != (p_pvm_task)NULL) && (i < MAX_VIEWABLE_TASK) ;
	       i++, ppt = ppt->suivant)
	  {
	    x = (i * 64) + ((i + 1) * 10);
		
	    XCopyPlane
	      (dpy_pvm,
	       img_pvm_task,
	       xwin_pvm,
	       gc,
	       0, 0,
	       64, 64,
	       x, 10,
	       1);
		
	    XDrawString
	      (dpy_pvm,
	       xwin_pvm,
	       gc,
	       x+10, 95,
	       task_view_func(ppt),
	       strlen(task_view_func(ppt)));
		
	    if (    (selection_host->old_selected_task != (char *)NULL)
		 && (strcmp(ppt->tid, selection_host->old_selected_task) == 0))
	      {
		selection_host->selection_pvm = ppt ;
	      }
	  }
	}
	
	selection_host->nb_pvms = compte_pvm_tasks (selection_host->pvm_tasks);
	maj_task_count(selection_host);
	mettre_a_jour_pvm(selection_host->selection_pvm);
	    
	if(selection_host->selection_pvm == (p_pvm_task)NULL)
	  {
	    xv_set(racine,
		   FRAME_BUSY, FALSE,
		   NULL);
	    etat = S_ANY;
	  }
	else
	  {
	    int num ;

	    num = numero_pvm_task (selection_host->pvm_tasks,
				   selection_host->selection_pvm) ;
	    
	    x = (num * 64) + (num * 10)+7;

	    XDrawRectangle
	      (dpy_pvm,
	       xwin_pvm,
	       gc_rect_pvm,
	       x, 5,
	       72, 73);

	    exec_command(TH, selection_host->selection_pvm->tid);

		
	  }
      } /* fin S_PS */
      break;

		
    case TH : /* Mise a jour du cadre Thread(s) */
      {
	p_pvm_task selection_pvm ;

	if (selection_host == (p_host)NULL)
	  {
	    /* si aucun host n'est selectionne,
	       pas la peine d'aller plus loin */
	    break ;
	  }
	
	selection_pvm = selection_host->selection_pvm ;

	if (selection_pvm == (p_pvm_task)NULL)
	  {
	    break ;
	  }

	if (selection_pvm->old_selected_thread != (char *)NULL)
	  {
	    free (selection_pvm->old_selected_thread) ;
	  }
	
	if(selection_pvm->selection_thread != (p_thread)NULL)
	  {
	    selection_pvm->old_selected_thread =
	      malloc (1 + strlen(selection_pvm->selection_thread->thread_id));
	    strcpy(selection_pvm->old_selected_thread,
		   selection_pvm->selection_thread->thread_id) ;
	  }
		
	selection_pvm->selection_thread = (p_thread)NULL ;

	selection_pvm->threads = extract_th(buf,
					   selection_pvm,
					   selection_pvm->threads);
	
	if ((selection_pvm->xv_objects.thread_frame != XV_NULL)
	    && (selection_pvm->xv_objects.thread_frame_cache == 0))
	  {
	    p_thread pt ;
	    p_threads_xview_objects pxv ;

	    pxv = &(selection_pvm->xv_objects) ;
	    
	    XClearWindow(pxv->dpy_thread, pxv->xwin_thread);
	    
	    /* boucle d'affichage des icones */
	    for (compte = 0, i = 0, pt = selection_pvm->threads ;
		 (pt != (p_thread)NULL) && (i < MAX_VIEWABLE_THREAD) ;
		 compte++, i++, pt = pt->suivant) 
	      {
		if (compte == 8)
		  {
		    compte = 0 ;
		  }
		
		x = (compte * 64) + ((compte + 1) * 10);
		y = (i / 8) * 95 + 10 ;

		if(pt->running[0] == 'n')
		  {
		    XCopyPlane
		      (pxv->dpy_thread,
		       img_endormi,
		       pxv->xwin_thread,
		       gc,
		       0, 0,
		       64, 64,
		       x, y,
		       1);
		  }
		else if(pt->migratable[0] == 'y')
		  {
		    XCopyPlane
		      (pxv->dpy_thread,
		       img_thread,
		       pxv->xwin_thread,
		       gc,
		       0, 0,
		       64, 64,
		       x, y,
		       1);
		  }
		else
		  {
		    XCopyPlane
		      (pxv->dpy_thread,
		       img_boulet,
		       pxv->xwin_thread,
		       gc,
		       0, 0,
		       64, 64,
		       x, y,
		       1);
		  }
			
		XDrawString
		  (pxv->dpy_thread,
		   pxv->xwin_thread,
		   gc,
		   x, y+85,
		   thread_view_func(pt),
		   strlen(thread_view_func(pt)));
			
		if (   (selection_pvm->old_selected_thread != (char *)NULL)
		    && (strcmp (pt->thread_id,
				selection_pvm->old_selected_thread) == 0))
		  {
		    selection_pvm->selection_thread = pt ;
		  }
	      }

	    selection_pvm->nb_threads =
	      compte_threads (selection_pvm->threads) ;
	    /* maj_thread_count(); */
	    mettre_a_jour_thread(selection_pvm,
				 selection_pvm->selection_thread);
	    
	    if(selection_pvm->selection_thread != (p_thread)NULL)
	      {
		int num ;

		num = numero_thread(selection_pvm->threads,
				    selection_pvm->selection_thread) ;
		
		y = (num / 8); 
		    
		if (y > 0)
		  {
		    x = num - (y * 8);
		  }
		else
		  {
		    x = num ;
		  }
			
		x_rect = (x * 64) + (x * 10) + 7;
		y_rect = y * 95;

		XDrawRectangle
		  (pxv->dpy_thread,
		   pxv->xwin_thread,
		   pxv->gc_rect_thread,
		   x_rect, y_rect+5,
		   72, 73) ;
	      }
	  }
	    
	if(selection_pvm->threads == (p_thread)NULL)
	  { /* Pas courant hein ? */
	    exec_command(S_PS, NULL);
	  }
	else
	  {
	    xv_set(racine,
		   FRAME_BUSY, FALSE,
		   NULL);
	    etat = S_ANY;
	  }
      } /* fin TH */
    break;

    case S_HALT :
      {
	xv_set(racine,
	       FRAME_BUSY, FALSE,
	       NULL);
	etat = S_ANY;
      }
    break;
    
    case S_QUIT :
      {
	xv_set(racine,
	       FRAME_BUSY, FALSE,
	       NULL);
	etat = S_ANY;
      }
    break;
    
    case S_KILL :
      {
	exec_command(S_PS,NULL);
      }
    break;
    
    case S_MIGR :
      {
	if(selection_host->selection_pvm == (p_pvm_task)NULL)
	  {
	    xv_set(racine,
		   FRAME_BUSY, FALSE,
		   NULL);
	    etat = S_ANY;
	  }
	else
	  {
	    /* Pause pour laisser le temps a la migration de s'effectuer */
	    sleep (1) ; 
		
	    exec_command(TH, selection_host->selection_pvm->tid);
	  }
      }
    break;
    
    case S_FREEZE :
      {
	if(selection_host->selection_pvm == (p_pvm_task)NULL)
	  {
	    xv_set(racine,
		   FRAME_BUSY, FALSE,
		   NULL);
	    etat = S_ANY;
	  }
	else
	  {
	    exec_command(TH, selection_host->selection_pvm->tid);
	  }
	
	mettre_a_jour_thread(selection_host->selection_pvm,
			     selection_host->selection_pvm->selection_thread) ;
      }
    break;
    
    case USER :
      {
	buf[offset-4] = 0;

	if(offset > 4) { 
	  Frame tmp_frame;
	  Textsw tmp_textsw;

	  tmp_frame = (Frame)xv_create(racine, FRAME_CMD,
				       FRAME_LABEL,	"User command results",
				       FRAME_CMD_PUSHPIN_IN,	TRUE,
				       FRAME_DONE_PROC,	user_results_done_proc,
				       XV_SHOW,		TRUE,
				       XV_WIDTH,		500,
				       XV_HEIGHT,		200,
				       NULL);

	  tmp_textsw = (Textsw)xv_create(tmp_frame, TEXTSW,
					 TEXTSW_READ_ONLY,	TRUE,
					 XV_X,			0,
					 XV_Y,			0,
					 XV_WIDTH,		500,
					 XV_HEIGHT,		200,
					 NULL);

	  xv_set(tmp_textsw, TEXTSW_CONTENTS, buf, NULL);

	  xv_set(tmp_textsw, TEXTSW_FIRST, 0, NULL);

	  xv_set(tmp_textsw, TEXTSW_READ_ONLY, TRUE, NULL);

	  textsw_normalize_view(tmp_textsw, 0);

	  xv_set(tmp_frame, XV_KEY_DATA, 0, tmp_textsw, NULL);
	}
	xv_set(racine, FRAME_BUSY, FALSE, NULL);
	etat = S_ANY;
      } /* fin USER */
    break;
    
    } /* fin du switch { ... } */
  offset = 0;
  return NOTIFY_DONE;
}



/* Gestion du panier */
/*===================*/

void mettre_a_jour_panier()
{
  char message[128];

  if(fenetre_cache_panier == 0)
    {
      if(panier.nb)
	{
	  XCopyPlane(dpy_panier,
		     img_panier_ouvert,
		     xwin_panier,
		     gc,
		     0, 0,
		     64, 64,
		     0, 0,
		     1);
	}
      else
	{
	  XCopyPlane(dpy_panier,
		     img_panier,
		     xwin_panier,
		     gc,
		     0, 0,
		     64, 64,
		     0, 0,
		     1);
	}

      sprintf(message, "(%d threads)", panier.nb);
      xv_set(panier_count_item, PANEL_LABEL_STRING, message, NULL);
  
      if(panier.nb)
	{
	  sprintf(message, "(Archi = %s)", panier.archi);
	}
      else
	{
	  sprintf(message, "");
	}
  
      xv_set(panier_archi_item, PANEL_LABEL_STRING, message, NULL);
    }
}

/* Ajout dans panier */
void mi_aller_dans_panier(Menu_item item, Menu_generate op)
{
  char message[64] ;
  char args[128];
  int result ;
  int x ;
  int y ;
  int x_rect ;
  int y_rect ;
  
  p_pvm_task selection_pvm ;

  selection_pvm = selection_host->selection_pvm ;

  if(selection_pvm->selection_thread == (p_thread)NULL)
    {
      result = notice_prompt
	(canvas_paint_window(selection_pvm->xv_objects.canvas_thread),
	 NULL,
	 NOTICE_FOCUS_XY, 300, 100,
	 NOTICE_MESSAGE_STRINGS,
	 "No thread selected", NULL,
	 NOTICE_BUTTON, "Ok", 0,	
       NULL);
    }
  else
    {
      int num ;
      p_thread selection_thread ;

      selection_thread = selection_pvm->selection_thread ;
      num = numero_thread (selection_pvm->threads,
			   selection_thread);
      
      y = (num / 8);
      if (y > 0)
	{
	  x = num - (y * 8);
	}
      else
	{
	  x = num ;
	}
	    
      x_rect = (x * 64) + (x * 10) + 39 ;
      y_rect = y * 95 + 32 ;

      if(selection_thread->migratable[0] != 'y')
	{
	  result = notice_prompt
	    (canvas_paint_window(selection_pvm->xv_objects.canvas_thread),
	     NULL,
	     NOTICE_FOCUS_XY, x_rect, y_rect,
	     NOTICE_MESSAGE_STRINGS,
	     "Thread is not migratable", NULL,
	     NOTICE_BUTTON, "Ok", 0,	
	     NULL);
	}
      else if(   (panier.nb != 0)
	      && (strcmp(panier.task_id, selection_pvm->tid) != 0))
	{
	  sprintf(message, "Thread must come from task %s", panier.task_id);
	  result = notice_prompt
	    (canvas_paint_window(selection_pvm->xv_objects.canvas_thread),
	     NULL,
	     NOTICE_FOCUS_XY, x_rect, y_rect,
	     NOTICE_MESSAGE_STRINGS,
	     message, NULL,
	     NOTICE_BUTTON, "Ok", 0,	
	     NULL);
	}
      else
	{
	  if(panier.nb == 0) 
	    {
	      strcpy(panier.archi, selection_host->archi);
	      strcpy(panier.task_id, selection_pvm->tid);
	    }
		
	  strcpy(panier.thread[panier.nb], selection_thread->thread_id) ;
	  panier.nb++ ;

	  sprintf (args,
		   "%s %s",
		   selection_pvm->tid,
		   selection_thread->thread_id
		   );

	  mettre_a_jour_panier();

	  xv_set(racine, FRAME_BUSY, TRUE, NULL);
	  exec_command(S_FREEZE, args);
	}
    }
}

void mi_prendre_du_panier(Menu_item item, Menu_generate op)
{
  int i ;
  int result ;
  int x ;
  char message[64] ;
  char args[1024] ;
  p_pvm_task selection_pvm ;

  selection_pvm = selection_host->selection_pvm ;
  
  if(panier.nb == 0)
    {
      result = notice_prompt (canvas_paint_window(selection_pvm->xv_objects.canvas_thread),
			      NULL,
			      NOTICE_FOCUS_XY, 300, 100,
			      NOTICE_MESSAGE_STRINGS,
			      "Threads basket is empty",
			      NULL,
			      NOTICE_BUTTON, "Ok", 0,
			      NULL);
    }
  else if(selection_pvm == (p_pvm_task)NULL)
    {
      result = notice_prompt (canvas_paint_window(canvas_pvm),
			      NULL,
			      NOTICE_FOCUS_XY,	150,50,
			      NOTICE_MESSAGE_STRINGS,
			      "No task selected", NULL,
			      NOTICE_BUTTON, "Ok", 0,
			      NULL);
    }
  else if(strcmp(panier.archi, selection_host->archi) != 0)
    {
      int num ;

      num = numero_host (hosts, selection_host);
      
      x = (num * 64) + (num * 10) + 39 ;

    sprintf(message,
	    "In the basket, threads come from a %s architecture",
	    panier.archi);

    result = notice_prompt (canvas_paint_window(canvas),
			    NULL,
			    NOTICE_FOCUS_XY,	x, 50,
			    NOTICE_MESSAGE_STRINGS,	message, NULL,
			    NOTICE_BUTTON, "Ok", 0,
			    NULL);
    } 
  else
    {
      sprintf(args, "%s %s ", panier.task_id, selection_pvm->tid);
      for(i = 0 ; i < panier.nb ; i++)
	{
	  strcat(args, panier.thread[i]);
	  strcat(args, " ");
	}
      
      panier.nb = 0;
      mettre_a_jour_panier();
	    
      xv_set(racine, FRAME_BUSY, TRUE, NULL);
      exec_command(S_MIGR, args);
	    
      mettre_a_jour_thread(selection_pvm, selection_pvm->selection_thread) ;
    }
}

done_panier(Frame subframe)
{
  fenetre_cache_panier = 1;
  xv_set(subframe,
	 XV_SHOW, FALSE,
	 NULL);
}

void redessiner_panier(Canvas canvas,
		       Xv_Window fenetre,
		       Display *dpy,
		       Window xwin,
		       Xv_xrectlist *xrects)
{
  int hx ;
  int hy ;
  int larg ;
  int haut ;
  int i ;
  int x ;
  int y ;
  int x_rect ;
  int y_rect ;
  int compte;
  
  unsigned long blanc ;
  unsigned long noir;

  gc_panier = DefaultGC(dpy, DefaultScreen(dpy));

  dpy_panier = dpy;
  xwin_panier = xwin;

  XReadBitmapFile(dpy,
		  xwin,
		  icon_panier,
		  &larg, &haut,
		  &img_panier,
		  &hx, &hy);

  XReadBitmapFile(dpy,
		  xwin,
		  icon_panier_ouvert,
		  &larg, &haut,
		  &img_panier_ouvert,
		  &hx, &hy);

  if(panier.nb)
    {
      XCopyPlane(dpy,
		 img_panier_ouvert,
		 xwin,
		 gc,
		 0, 0,
		 64, 64,
		 0, 0,
		 1);
    }
  else
    {
      XCopyPlane(dpy,
		 img_panier,
		 xwin,
		 gc,
		 0, 0,
		 64, 64,
		 0, 0,
		 1);
    }
}

Menu_item mi_voir_panier(Menu menu, Menu_item item)
{
  char message[128];
  Panel panier_panel;

  if(panier_frame == XV_NULL)
    {
      panier_frame = xv_create(racine, FRAME_CMD,
			       XV_WIDTH, 200,
			       XV_HEIGHT, 130,
			       FRAME_LABEL, "Threads Basket",
			       FRAME_DONE_PROC, done_panier,
			       FRAME_CMD_PUSHPIN_IN, TRUE,
			       NULL);

      panier_panel = xv_get(panier_frame, FRAME_CMD_PANEL, NULL);

      canvas_panier=(Canvas)xv_create(panier_frame, CANVAS,
				      CANVAS_AUTO_EXPAND, FALSE,
				      CANVAS_AUTO_SHRINK, FALSE,
				      CANVAS_X_PAINT_WINDOW, TRUE,
				      CANVAS_WIDTH, 64,
				      CANVAS_HEIGHT, 64,
				      CANVAS_REPAINT_PROC, redessiner_panier,
				      XV_X, 68,
				      XV_Y, 10,
				      XV_WIDTH, 64,
				      XV_HEIGHT, 64,
				      NULL);

      sprintf(message, "%d thread(s)", panier.nb);

      panier_count_item = (Panel_item)xv_create(panier_panel, PANEL_MESSAGE,
						PANEL_LABEL_STRING, message,
						XV_X, 50,
						XV_Y, 85,
						NULL);

      if(panier.nb != 0)
	{
	  sprintf(message, "(Archi = %s)", panier.archi);
	}
      else
	{
	  sprintf(message, "");
	}

      panier_archi_item = (Panel_item)xv_create(panier_panel, PANEL_MESSAGE,
						PANEL_LABEL_STRING, message,
						XV_X, 30,
						XV_Y, 110,
						NULL);

      fenetre_cache_panier = 0;

  xv_set(panier_frame,
	 XV_SHOW, TRUE,
	 NULL);
    }
  else
    {
      fenetre_cache_panier = 0;

      xv_set(panier_frame,
	     XV_SHOW, TRUE,
	     NULL);
      mettre_a_jour_panier();
    }
}



/* Gestion des panneaux d'information */
/*====================================*/


/* fonctions d'acces aux donnees d'une tache */

p_pvm_task tache_associee_au_cadre (Frame subframe, p_host ph)
{
  if (subframe == XV_NULL)
    {
      return (p_pvm_task)NULL ;
    }
  else
    {
      p_pvm_task ppt ;

      ppt = ph->pvm_tasks ;

      while (ppt != (p_pvm_task)NULL)
	{
	  if (ppt->xv_objects.thread_frame == subframe)
	    {
	      return ppt ;
	    }
	  ppt = ppt->suivant ;
	}
      
      return (p_pvm_task)NULL ; /* Non trouve */
    }
}


p_pvm_task tache_associee_au_panneau (Frame subframe, p_host ph)
{
  if (subframe == XV_NULL)
    {
      return (p_pvm_task)NULL ;
    }
  else
    {
      p_pvm_task ppt ;

      ppt = ph->pvm_tasks ;

      while (ppt != (p_pvm_task)NULL)
	{
	  if (ppt->xv_objects.conf_thread == subframe)
	    {
	      return ppt ;
	    }
	  ppt = ppt->suivant ;
	}
      
      return (p_pvm_task)NULL ; /* Non trouve */
    }
}

p_pvm_task tache_associee_a_la_fenetre (Xv_Window win, p_host ph)
{
  if (win == XV_NULL)
    {
      return (p_pvm_task)NULL ;
    }
  else
    {
      p_pvm_task ppt ;

      ppt = ph->pvm_tasks ;

      while (ppt != (p_pvm_task)NULL)
	{
	  if (   (ppt->xv_objects.thread_frame != XV_NULL)
	      && (canvas_paint_window(ppt->xv_objects.canvas_thread) == win))
	    {
	      return ppt ;
	    }
	  ppt = ppt->suivant ;
	}

      return (p_pvm_task) NULL ; /* Non trouve */
    }  
}

/* fermeture du panneau d'information de thread */
/*----------------------------------------------*/
done_proc_thread(Frame subframe)
{
  p_host ph ;
  p_pvm_task ppt ;

  ppt = tache_associee_au_panneau(subframe, selection_host);
  
  if (ppt = (p_pvm_task)NULL)
    {
      ph = hosts ;

      while (ph != (p_host)NULL)
	{
	  if (ph != selection_host)
	    {
	      ppt = tache_associee_au_panneau(subframe, ph);
	      if (ppt != (p_pvm_task)NULL)
		{
		  break ;
		}
	    }
	  ph = ph->suivant ;
	}
    }
  else
    {
      ph = selection_host ;
    }
  

  if ((ph != (p_host)NULL) && (ppt != (p_pvm_task)NULL))
    {
      ppt->xv_objects.thread_panel_cache = 1 ;
      xv_set(subframe,
	     XV_SHOW, FALSE,
	     NULL);
    }
  else
    {
      /* si on arrive dans cette branche
	 on peut considerer que le sous-cadre
	 'subframe' n'est plus associe à une donnée.
	 Il vaut donc mieux le detruire.
	 */
      xv_destroy(subframe) ;
    }
}

/* creation et affichage du panneau d'information de thread */
/*----------------------------------------------------------*/
Menu_item mi_info_thread(Menu menu, Menu_item item)
{
  Panel panel_conf;
  p_pvm_task selection_pvm ;
  p_threads_xview_objects pxv;
  
  selection_pvm = selection_host->selection_pvm ;
  pxv = &(selection_pvm->xv_objects);
  
  if(pxv->conf_thread == XV_NULL)
    { /* Si le panneau n'existe pas, on le cree */
      pxv->conf_thread = xv_create
	(racine, FRAME_CMD,
	 XV_WIDTH,		200,
	 XV_HEIGHT,		180,
	 FRAME_LABEL,		"Thread Info",
	 FRAME_DONE_PROC,	done_proc_thread,
	 FRAME_CMD_PUSHPIN_IN,	TRUE,
	 NULL);

      panel_conf = xv_get(pxv->conf_thread, FRAME_CMD_PANEL, NULL);

      pxv->pan_thread[0] = xv_create
	(panel_conf,
	 PANEL_MESSAGE,
	 PANEL_LABEL_STRING,	"",
	 XV_X,			0,
	 XV_Y,			5,
	 NULL);

      pxv->pan_thread[1] = xv_create
	(panel_conf,
	 PANEL_MESSAGE,
	 PANEL_LABEL_STRING,	"",
	 XV_X,			0,
	 XV_Y,			30,
	 NULL);

      pxv->pan_thread[2] = xv_create
	(panel_conf,
	 PANEL_MESSAGE,
	 PANEL_LABEL_STRING,	"",
	 XV_X,			0,
	 XV_Y,			55,
	 NULL);

      pxv->pan_thread[3] = xv_create
	(panel_conf,
	 PANEL_MESSAGE,
	 PANEL_LABEL_STRING,	"",
	 XV_X,			0,
	 XV_Y,			80,
	 NULL);

      pxv->pan_thread[4] = xv_create
	(panel_conf,
	 PANEL_MESSAGE,
	 PANEL_LABEL_STRING,	"",
	 XV_X,			0,
	 XV_Y,			105,
	 NULL);

      pxv->pan_thread[5] = xv_create
	(panel_conf,
	 PANEL_MESSAGE,
	 PANEL_LABEL_STRING,	"",
	 XV_X,			0,
	 XV_Y,			130,
	 NULL);

      pxv->pan_thread[6] = xv_create
	(panel_conf,
	 PANEL_MESSAGE,
	 PANEL_LABEL_STRING,	"",
	 XV_X,			0,
	 XV_Y,			155,
	 NULL);

      pxv->thread_panel_cache = 0;
      mettre_a_jour_thread(selection_pvm, selection_pvm->selection_thread);
      
      xv_set(pxv->conf_thread,
	     XV_SHOW, TRUE,
	     NULL);
    }
  else
    {
      pxv->thread_panel_cache = 0;
      mettre_a_jour_thread(selection_pvm, selection_pvm->selection_thread);
      
      xv_set(pxv->conf_thread,
	     XV_SHOW, TRUE,
	     NULL);
    }
}


/* fermeture du panneau d'information de tache */
/*---------------------------------------------*/
done_proc_pvm(Frame subframe)
{
  fenetre_cache_pvm = 1;
  xv_set(subframe,
	 XV_SHOW, FALSE,
	 NULL);
}


/* creation et affichage du panneau d'information de tache */
/*---------------------------------------------------=====--*/
Menu_item mi_info_pvm(Menu menu, Menu_item item)
{
  Panel panel_conf;

  if(conf_pvm == 0)
    {
      conf_pvm = xv_create(racine, FRAME_CMD,
			   XV_WIDTH, 200,
			   XV_HEIGHT, 80,
			   FRAME_LABEL, "Task Info",
			   FRAME_DONE_PROC, done_proc_pvm,
			   FRAME_CMD_PUSHPIN_IN, TRUE,
			   NULL);

      panel_conf = xv_get(conf_pvm, FRAME_CMD_PANEL, NULL) ;

      Pan_Pvm[0] = xv_create(panel_conf, PANEL_MESSAGE,
			     PANEL_LABEL_STRING, "",
			     XV_X, 0,
			     XV_Y, 5,
			     NULL);

      Pan_Pvm[1] = xv_create(panel_conf, PANEL_MESSAGE,
			     PANEL_LABEL_STRING, "",
			     XV_X, 0,
			     XV_Y, 30,
			     NULL);

      Pan_Pvm[2] = xv_create(panel_conf, PANEL_MESSAGE,
			     PANEL_LABEL_STRING, "",
			     XV_X, 0,
			     XV_Y, 55,
			     NULL);

      fenetre_cache_pvm = 0;
      mettre_a_jour_pvm(selection_host->selection_pvm);
      
      xv_set(conf_pvm,
	     XV_SHOW, TRUE,
	     NULL);

    }
  else
    {
      fenetre_cache_pvm = 0;
      mettre_a_jour_pvm(selection_host->selection_pvm);
      
      xv_set(conf_pvm,
	     XV_SHOW, TRUE,
	     NULL);
    }
}

Menu_item mi_delete_pvm(Menu menu, Menu_item item)
{
  int result ;
  int x ;


  if(    (selection_host == (p_host)NULL)
      || (selection_host->selection_pvm  == (p_pvm_task)NULL))
    {
      result = notice_prompt(canvas_paint_window(canvas_pvm), NULL,
			     NOTICE_FOCUS_XY, 150, 50,
			     NOTICE_MESSAGE_STRINGS,
			     "No task selected",
			     NULL,
			     NOTICE_BUTTON, "Ok", 0,
			     NULL);

    }
  else
    {
      int num ;
      
      num = numero_pvm_task (selection_host->pvm_tasks,
			     selection_host->selection_pvm) ;
      
      x = (num * 64)+(num * 10) + 39 ;

      result = notice_prompt(canvas_paint_window(canvas_pvm), NULL,
			     NOTICE_FOCUS_XY, x, 50,
			     NOTICE_MESSAGE_STRINGS,
			     "Do you really want to kill this task ?",
			     NULL,
			     NOTICE_BUTTON_YES,	"Yes",
			     NOTICE_BUTTON_NO, "No",
			     NULL);

    if(result == NOTICE_YES)
      {
	xv_set(racine,
	       FRAME_BUSY, TRUE,
	       NULL);
	
	exec_command(S_KILL, selection_host->selection_pvm->tid);
      }
  }
}

void validate_host(void)
{
  char machine[128];

  strcpy(machine, (char *)xv_get(text_saisi, PANEL_VALUE));
  xv_set(racine,
	 FRAME_BUSY, TRUE,
	 NULL);
  exec_command(S_ADD, machine);
  xv_destroy_safe(xv_add_host);
}

Menu_item mi_add_host(Menu menu, Menu_item item)
{
  char param[256];
  Panel panel_host;

  xv_add_host = xv_create
    (racine, FRAME_CMD,
     XV_WIDTH, 200,
     XV_HEIGHT,	60,
     FRAME_LABEL, "Add Host",
     NULL);

  panel_host = xv_get(xv_add_host, FRAME_CMD_PANEL, NULL);

  text_saisi = (Panel)xv_create
    (panel_host,
     PANEL_TEXT,
     PANEL_LABEL_STRING, "Host Name : ",
     PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
     PANEL_NOTIFY_STRING, "\n\r",
     PANEL_NOTIFY_PROC,	validate_host,
     NULL);

  (void)xv_create
    (panel_host,
     PANEL_BUTTON,
     PANEL_LABEL_STRING, "Ok",
     PANEL_NOTIFY_PROC,	validate_host,
     XV_X, 80,
     NULL);

  xv_set(xv_add_host,
	 XV_SHOW, TRUE,
	 NULL);

}

void validate_info_host(void)
{
  xv_destroy_safe(conf_host);
}

/* fermeture du panneau d'information Host */
/*------------------------------------====-*/
done_proc_host(Frame subframe)
{
  fenetre_cache_host = 1;
  xv_set(subframe,
	 XV_SHOW, FALSE,
	 NULL);
}


/* creation et affichage du panneau d'information Host */
/*------------------------------------------------====-*/
Menu_item mi_info_host(Menu menu, Menu_item item)
{
  Panel panel_conf;

  if(conf_host == 0)
    {
      conf_host = xv_create(racine, FRAME_CMD,
			    XV_WIDTH, 200,
			    XV_HEIGHT, 80,
			    FRAME_LABEL, "Host Info",
			    FRAME_DONE_PROC, done_proc_host,
			    FRAME_CMD_PUSHPIN_IN, TRUE,
			    NULL);

      panel_conf = xv_get(conf_host, FRAME_CMD_PANEL, NULL);

      Pan_Host[0] = xv_create(panel_conf,PANEL_MESSAGE,
			      PANEL_LABEL_STRING, "",
			      XV_X, 0,
			      XV_Y, 5,
			      NULL);

      Pan_Host[1] = xv_create(panel_conf,PANEL_MESSAGE,
			      PANEL_LABEL_STRING, "",
			      XV_X, 0,
			      XV_Y, 30,
			      NULL);

      Pan_Host[2] = xv_create(panel_conf,PANEL_MESSAGE,
			      PANEL_LABEL_STRING, "",
			      XV_X, 0,
			      XV_Y, 55,
			      NULL);

      fenetre_cache_host = 0;
      mettre_a_jour_host(selection_host);

      xv_set(conf_host,
	     XV_SHOW, TRUE,
	     NULL);
    }
  else
    {
      fenetre_cache_host = 0;
      mettre_a_jour_host(selection_host);

      xv_set(conf_host,
	     XV_SHOW, TRUE,
	     NULL);
    }
}

Menu_item mi_delete_host(Menu menu, Menu_item item)
{
  int result ;
  int x;

  if(selection_host == (p_host)NULL)
    {
      result = notice_prompt(canvas_paint_window(canvas), NULL,
			     NOTICE_FOCUS_XY, 150, 50,
			     NOTICE_MESSAGE_STRINGS, "No host selected", NULL,
			     NOTICE_BUTTON, "Ok", 0,
			     NULL);
    }
  else
    {
      int num ;

      num = numero_host (hosts, selection_host);
      
      x = (num * 64) + (num * 10) + 39 ;

      result = notice_prompt(canvas_paint_window(canvas), NULL,
			     NOTICE_FOCUS_XY, x, 50,
			     NOTICE_MESSAGE_STRINGS,
			     "Do you really want to delete this host ?",
			     NULL,
			     NOTICE_BUTTON_YES,	"Yes",
			     NOTICE_BUTTON_NO, "No",
			     NULL);

    if(result == NOTICE_YES)
      {
	xv_set(racine,
	       FRAME_BUSY, TRUE,
	       NULL);
	exec_command(S_DELETE, selection_host->nom);
      }
    }
}



/* Gestion des cadres d'affichage de Threads de chaque tache */
/*===========================================================*/


/* fermeture d'un cadre de threads */
/*-------------------------=======-*/

done_proc_thread_frame(Frame subframe)
{
  p_host ph ;
  p_pvm_task ppt ;

  ppt = tache_associee_au_cadre(subframe, selection_host);
  
  if (ppt == (p_pvm_task)NULL)
    {
      ph = hosts ;

      while (ph != (p_host)NULL)
	{
	  if (ph != selection_host)
	    {
	      ppt = tache_associee_au_cadre(subframe, ph);
	      if (ppt != (p_pvm_task)NULL)
		{
		  break ;
		}
	    }
	  ph = ph->suivant ;
	}
    }
  else
    {
      ph = selection_host ;
    }
  

  if ((ph != (p_host)NULL) && (ppt != (p_pvm_task)NULL))
    {
      ppt->xv_objects.thread_frame_cache = 1 ;
      xv_set(subframe,
	     XV_SHOW, FALSE,
	     NULL);
    }
  else
    {
      /* si on arrive dans cette branche
	 on peut considerer que le sous-cadre
	 'subframe' n'est plus associe à une donnée.
	 Il vaut donc mieux le detruire.
	 */
      xv_destroy(subframe) ;
    }
}


/* ouverture d'un cadre de threads */
/*-------------------------=======-*/

Menu_item mi_thread_frame(Menu menu, Menu_item item)
{
  if (    (selection_host != (p_host)NULL) 
       && (selection_host->selection_pvm != (p_pvm_task)NULL))
    {
      p_pvm_task selection_pvm ;
      p_threads_xview_objects pxv ;

      selection_pvm = selection_host->selection_pvm ;
      pxv = &(selection_pvm->xv_objects);
      
      if (pxv->thread_frame == XV_NULL)
	{
	  char titre[160] ;

	  sprintf (titre, "Task [%s] on %s",
		   selection_pvm->tid,
		   selection_pvm->host
		   ) ;

	  pxv->thread_frame = xv_create
	    (racine, FRAME_CMD,
	     XV_WIDTH, 600,
	     XV_HEIGHT, 200,
	     FRAME_LABEL, titre,
	     FRAME_DONE_PROC, done_proc_thread_frame,
	     FRAME_CMD_PUSHPIN_IN, TRUE,
	     NULL) ;
	  
	  pxv->canvas_thread = (Canvas)xv_create
	    (pxv->thread_frame, CANVAS,
	     CANVAS_AUTO_EXPAND, TRUE,
	     CANVAS_AUTO_SHRINK, FALSE,
	     CANVAS_X_PAINT_WINDOW, TRUE,
	     CANVAS_WIDTH, 600,
	     CANVAS_HEIGHT, (MAX_VIEWABLE_THREAD>>3)*95,
	     CANVAS_REPAINT_PROC, redessiner_thread,
	     XV_X, 0,
	     XV_Y, 0,
	     XV_WIDTH, 600,
	     XV_HEIGHT, 200,
	     NULL) ;
	  
	  xv_set(canvas_paint_window(pxv->canvas_thread),
	     WIN_EVENT_PROC, ma_procedure_thread,
	     WIN_CONSUME_EVENTS, WIN_MOUSE_BUTTONS, NULL,
	     NULL);
	  
	  (void)xv_create
	    (pxv->canvas_thread, SCROLLBAR,
	     SCROLLBAR_SPLITTABLE, TRUE,
	     SCROLLBAR_DIRECTION, SCROLLBAR_VERTICAL,
	     NULL);
	     
	  pxv->thread_frame_cache = 0;
	  
	  xv_set (pxv->thread_frame,
		  XV_SHOW, TRUE,
		  NULL) ;
	}
      else
	{
	  pxv->thread_frame_cache = 0;
	  
	  xv_set (pxv->thread_frame,
		  XV_SHOW, TRUE,
		  NULL) ;
	}
    }
}


/*====================*/
void au_sujet_de(void)
{
  Panel panel_conf;
  char message[128];

  a_propos = xv_create(racine, FRAME_CMD,
		       XV_WIDTH, 200,
		       XV_HEIGHT, 190,
		       FRAME_LABEL, "About Xpm2 designers",
		       NULL);

  panel_conf = xv_get(a_propos, FRAME_CMD_PANEL, NULL) ;
	
  sprintf(message, "X-Console For PM2");
  (void) xv_create(panel_conf, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, message,
		   PANEL_LABEL_BOLD, TRUE,
		   XV_X, 50,
		   XV_Y, 5,
		   NULL);

  sprintf(message,"Programmed By :");
  (void) xv_create(panel_conf, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, message,
		   PANEL_LABEL_BOLD, TRUE,
		   XV_X, 0,
		   XV_Y, 30,
		   NULL);

  sprintf(message,"Gilles CYMBALISTA");
  (void) xv_create(panel_conf, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, message,
		   XV_X, 10,
		   XV_Y, 50,
		   NULL);
	
  sprintf(message,"Yvon HUBERT");
  (void) xv_create(panel_conf, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, message,
		   XV_X, 10,
		   XV_Y, 70,
		   NULL);

  sprintf(message,"Version 2.0 Programmed By :");
  (void) xv_create(panel_conf, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, message,
		   PANEL_LABEL_BOLD, TRUE,
		   XV_X, 0,
		   XV_Y, 100,
		   NULL);

  sprintf(message,"Olivier AUMAGE");
  (void) xv_create(panel_conf, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, message,
		   XV_X, 10,
		   XV_Y, 120,
		   NULL);
	

  sprintf(message,"Technical Help : ");
  (void) xv_create(panel_conf, PANEL_MESSAGE,
		   PANEL_LABEL_STRING,	message,
		   PANEL_LABEL_BOLD,	TRUE,
		   XV_X, 0,
		   XV_Y, 150,
		   NULL);

  sprintf(message,"Raymond NAMYST");
  (void) xv_create(panel_conf, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, message,
		   XV_X, 10,
		   XV_Y, 170,
		   NULL);

  xv_set(a_propos, XV_SHOW, TRUE, NULL);
}

int refresh(void) /* appele par le timer */
{
  if((automatic_refresh) && (etat == S_ANY))
    {
      xv_set(racine,
	     FRAME_BUSY, TRUE,
	     NULL);
      exec_command(S_CONF, NULL);
    }
  
  return NOTIFY_DONE;
}

int force_refresh()	/* appele par le bouton refresh */
{
  xv_set(racine,
	 FRAME_BUSY, TRUE,
	 NULL);
  exec_command(S_CONF, NULL);
  
  return NOTIFY_DONE;
}

void quitter(Panel_item item, Event *event)
{
  int result;

  result = notice_prompt(racine, NULL,
			 NOTICE_FOCUS_XY, event_x(event), event_y(event),
			 NOTICE_MESSAGE_STRINGS, "Quit or Halt ?", NULL,
			 NOTICE_BUTTON_YES, "Quit",
			 NOTICE_BUTTON_NO, "Halt",
			 NULL);

  xv_set(racine,
	 FRAME_BUSY, TRUE,
	 NULL);
  
  stop_timer();
  
  if(result == NOTICE_YES)
    {
      exec_command(S_QUIT, NULL);
    }
  else
    {
      exec_command(S_HALT, NULL);
    }
}

/* Gestion des evenements des cadres 'threads' */
void ma_procedure_thread(Xv_Window window, Event *event, Notify_arg arg)
{
  if ((event_action(event) == ACTION_MENU) && (event_is_down(event)))
    {
      p_host ph ;
      p_pvm_task ppt ;

      ppt = tache_associee_a_la_fenetre(window, selection_host);
  
      if (ppt == (p_pvm_task)NULL)
	{
	  ph = hosts ;

	  while (ph != (p_host)NULL)
	    {
	      if (ph != selection_host)
		{
		  ppt = tache_associee_a_la_fenetre(window, ph);
		  if (ppt != (p_pvm_task)NULL)
		    {
		      break ;
		    }
		}
	      ph = ph->suivant ;
	    }
	}
      else
	{
	  ph = selection_host ;
	}

      if (ph != selection_host)
	{
	  char *dtid ;
	  char *tid ;

	  dtid = (char *)malloc(1 + strlen(ph->dtid));
	  tid = (char *)malloc(1 + strlen(ppt->tid));

	  strcpy (dtid, ph->dtid);
	  strcpy (tid, ppt->tid);
	  
	  selection_host = ph ;
	  selection_host->selection_pvm = ppt ;
	  exec_command(S_CONF, NULL) ;
	  
	  if (   (selection_host != ph)
	      || (strcmp (selection_host->dtid, dtid) != 0))
	    {
	      /* l'host n'existe plus, on retourne donc tout de suite */
	      free (dtid);
	      free (tid);
	      
	      return ;
	    }
	  
	  free (dtid);
	  free (tid);
	}
      else if (ppt != selection_host->selection_pvm)
	{
	  char *tid ;

	  tid = (char *)malloc(1 + strlen(ppt->tid));

	  strcpy (tid, ppt->tid);	  
	  
	  selection_host->selection_pvm = ppt ;
	  exec_command(S_PS, NULL);
	  
	  if (    (selection_host->selection_pvm != ppt)
	       || (strcmp (selection_host->selection_pvm->tid, tid) !=0))
	    {
	      free (tid);
	      
	      return ;
	    }

	  free (tid);
	}
      
      menu_show(menu_thread, window, event, NULL);
    }
  else if((event_action(event) == ACTION_SELECT) && (event_is_down(event))) 
    {
      p_host ph ;
      p_pvm_task ppt ;

      ppt = tache_associee_a_la_fenetre(window, selection_host);
  
      if (ppt == (p_pvm_task)NULL)
	{
	  ph = hosts ;

	  while (ph != (p_host)NULL)
	    {
	      if (ph != selection_host)
		{
		  ppt = tache_associee_a_la_fenetre(window, ph);
		  if (ppt != (p_pvm_task)NULL)
		    {
		      break ;
		    }
		}
	      ph = ph->suivant ;
	    }
	}
      else
	{
	  ph = selection_host ;
	}

      if (ph != selection_host)
	{
	  char *dtid ;
	  char *tid ;

	  dtid = (char *)malloc(1 + strlen(ph->dtid));
	  tid = (char *)malloc(1 + strlen(ppt->tid));

	  strcpy (dtid, ph->dtid);
	  strcpy (tid, ppt->tid);
	  
	  selection_host = ph ;
	  selection_host->selection_pvm = ppt ;
	  exec_command(S_CONF, NULL) ;
	  
	  if (   (selection_host != ph)
	      || (strcmp (selection_host->dtid, dtid) != 0))
	    {
	      /* l'host n'existe plus, on retourne donc tout de suite */
	      free (dtid);
	      free (tid);
	      
	      return ;
	    }
	  
	  free (dtid);
	  free (tid);
	}
      else if (ppt != selection_host->selection_pvm)
	{
	  char *tid ;

	  tid = (char *)malloc(1 + strlen(ppt->tid));

	  strcpy (tid, ppt->tid);	  
	  
	  selection_host->selection_pvm = ppt ;
	  exec_command(S_PS, NULL);
	  
	  if (    (selection_host->selection_pvm != ppt)
	       || (strcmp (selection_host->selection_pvm->tid, tid) != 0))
	    {
	      free (tid);
	      
	      return ;
	    }

	  free (tid);
	}

      if(ppt->threads != (p_thread)NULL)
	{
	  p_threads_xview_objects pxv ;

	  int x ;
	  int y ;
	  int indice ;
	  int x_rect ;
	  int y_rect ;
	  int indice_x ;
	  int indice_y ;
	
	  pxv = &(ppt->xv_objects);
	  
	  x = event_x(event);
	  y = event_y(event);
	  
	  indice_x = x / 74;
	  indice_y = y / 95;
	  
	  indice = indice_x + (indice_y * 8);
		
	  if (indice > (ppt->nb_threads - 1))
	    {
	      indice = -1;
	    }
		
	  if(numero_thread (ppt->threads, ppt->selection_thread) != indice)
	    {
	      x = indice_x;
	      y = indice_y;

	      if(ppt->selection_thread != (p_thread)NULL)
		{
		  int num ;

		  num = numero_thread (ppt->threads, ppt->selection_thread);

		  indice_y = (num / 8) ;
				
		  if (indice_y > 0)
		    {
		      indice_x = num - (indice_y * 8) ;
		    }
		  else
		    {
		      indice_x = num ;
		    }
				
		  x_rect = (indice_x * 64) + (indice_x * 10) + 7;
		  y_rect = indice_y * 95;
			      
		  XDrawRectangle(pxv->dpy_thread,
				 pxv->xwin_thread,
				 pxv->gc_rect_thread,
				 x_rect, y_rect + 5,
				 72, 73);
		}
		    
	      if(indice != -1)
		{
		  x_rect = (x * 64) + (x * 10) + 7;
		  y_rect = y * 95;
		  XDrawRectangle
		    (pxv->dpy_thread,
		     pxv->xwin_thread,
		     pxv->gc_rect_thread,
		     x_rect, y_rect + 5,
		     72, 73);
		}
		    
	      ppt->selection_thread =
		renvoie_thread(ppt->threads, indice) ;

	      mettre_a_jour_thread(ppt, ppt->selection_thread);
	    }
	}	    
    }
}


/* Fonction de gestion des evenements dans le cadre PVM */
void ma_procedure_pvm (Xv_Window window, Event *event, Notify_arg arg)
{
  int x ;
  int x_rect ;
  int indice;

  if(event_action(event) == ACTION_MENU && event_is_down(event))
    {
      menu_show(menu_pvm, window, event, NULL);
    }
  else if (    (selection_host != (p_host)NULL)
	    && (selection_host->pvm_tasks != (p_pvm_task)NULL))
    {
      if(    (event_action(event) == ACTION_SELECT)
	  && (event_is_down(event)))
	{
	  x = event_x(event);
	  indice = (x / 74); /* calcul du numero du processus choisi */

	  if (indice > (selection_host->nb_pvms - 1))
	    { /* test de validite du numero */
	      indice = -1;
	    }

	  if(numero_pvm_task(selection_host->pvm_tasks,
			     selection_host->selection_pvm) != indice)
	    { /* si le numero est different de celui qui est actif
		 alors, mettre a jour le cadre Threads */

	      if(selection_host->selection_pvm != (p_pvm_task)NULL)
		{ /* effacement de l'ancien cadre de focus */
		  int num ;

		  num = numero_pvm_task(selection_host->pvm_tasks,
					selection_host->selection_pvm) ;
		  
		  x_rect = (num * 64)+(num * 10) + 7 ;
		  XDrawRectangle
		    (dpy_pvm,
		     xwin_pvm,
		     gc_rect_pvm,
		     x_rect, 5,
		     72, 73);
		}

	      if(indice != -1)
		{ /* affichage du nouveau cadre de focus */
		  x_rect = (indice * 64) + (indice * 10) + 7 ;
		  XDrawRectangle
		    (dpy_pvm,
		     xwin_pvm,
		     gc_rect_pvm,
		     x_rect, 5,
		     72, 73);
		}

	      selection_host->selection_pvm = renvoie_pvm_task
		(selection_host->pvm_tasks,
		 indice);
	      
	      mettre_a_jour_pvm(selection_host->selection_pvm);
	    }

	  if(selection_host->selection_pvm != (p_pvm_task)NULL)
	    {
	      xv_set(racine,
		     FRAME_BUSY, TRUE,
		     NULL);
	      exec_command(TH, selection_host->selection_pvm->tid);
	    }
	}
    }
}

void ma_procedure(Xv_Window window, Event *event, Notify_arg arg)
{
  int x, x_rect, indice;

  if ((event_action(event) == ACTION_MENU) && (event_is_down(event)))
    {
      menu_show(menu_host, window, event, NULL);
    }
  else if ((event_action(event) == ACTION_SELECT) && (event_is_down(event)))
    {
      x = event_x(event);
      
      indice = (x / 74);
      if(indice > nb_hosts - 1)
	{
	  indice = -1 ;
	  XClearWindow(dpy_pvm, xwin_pvm);
	}
      
      if (numero_host (hosts, selection_host) != indice)
      {
	if(selection_host != (p_host)NULL)
	  {
	    int num ;

	    num = numero_host (hosts, selection_host) ;
	    
	    x_rect = (num * 64) + (num * 10) + 7 ;
	    XDrawRectangle(dpy_host,
			   xwin_host,
			   gc_rectangle,
			   x_rect, 5,
			   70, 72);
	  }
	
	if(indice != -1)
	  {
	    x_rect = (indice * 64) + (indice * 10) + 7 ;
	    XDrawRectangle(dpy_host,
			   xwin_host,
			   gc_rectangle,
			   x_rect, 5,
			   70, 72);
	  }
	
	selection_host = renvoie_host(hosts, indice) ;
	mettre_a_jour_host(selection_host);
	if (selection_host != (p_host)NULL)
	  {
	    mettre_a_jour_pvm(selection_host->selection_pvm);
	    if (selection_host->selection_pvm != (p_pvm_task)NULL)
	      {
		mettre_a_jour_thread
		  (selection_host->selection_pvm,
		   selection_host->selection_pvm->selection_thread);
	      }
	  }
      }
      
      if(indice != -1)
	{
	  xv_set(racine,
		 FRAME_BUSY, TRUE,
		 NULL);
	  exec_command(S_PS, NULL);
	}
      else
	{
	  maj_task_count (selection_host) ;
	}
    }
}

void redessiner_thread(Canvas canvas,
		       Xv_Window fenetre,
		       Display *dpy,
		       Window xwin,
		       Xv_xrectlist *xrects)
{
  int hx ;
  int hy ;
  int larg ;
  int haut ;
  int i ;
  int x ;
  int y ;
  int x_rect ;
  int y_rect ;
  int compte;
  
  unsigned long blanc ;
  unsigned long noir;
  
  GC gc_thread ;
  GC gc_rect_thread ;
  Display *dpy_thread ;
  Window xwin_thread ;

  p_host ph ;
  p_pvm_task ppt ;
  p_threads_xview_objects pxv ;
  p_thread pt;
  
  ppt = tache_associee_a_la_fenetre(fenetre, selection_host);

  if (ppt == (p_pvm_task)NULL)
    {
      ph = hosts ;

      while (ph != (p_host)NULL)
	{
	  if (ph != selection_host)
	    {
	      ppt = tache_associee_a_la_fenetre(fenetre, selection_host);
  
	      if (ppt != (p_pvm_task)NULL)
		{
		  break ;
		}
	    }
	  ph = ph->suivant ;
	}
    }
  else
    {
      ph = selection_host ;
    }
  if ((ph == (p_host)NULL) || (ppt == (p_pvm_task)NULL))
    {
      xv_destroy(canvas) ;
      return ;
    }

  pxv = &(ppt->xv_objects);

  pxv->dpy_thread = dpy_thread = dpy;
  pxv->xwin_thread = xwin_thread = xwin ;
  
  pxv->gc_thread = gc_thread = DefaultGC(dpy, DefaultScreen(dpy));
	
  noir = BlackPixel(dpy, DefaultScreen(dpy));
  blanc = WhitePixel(dpy, DefaultScreen(dpy));

  gcv.function = GXinvert;
  gcv.foreground = noir;
  gcv.background = blanc;
  gcmask = GCForeground | GCBackground | GCFunction;

  pxv->gc_rect_thread = gc_rect_thread = XCreateGC(dpy, xwin, gcmask, &gcv);

  XReadBitmapFile(dpy,
		  xwin,
		  icon_thread,
		  &larg, &haut,
		  &img_thread,
		  &hx, &hy);

  XReadBitmapFile(dpy,
		  xwin,
		  icon_boulet,
		  &larg, &haut,
		  &img_boulet,
		  &hx, &hy);

  XReadBitmapFile(dpy,
		  xwin,
		  icon_endormi,
		  &larg, &haut,
		  &img_endormi,
		  &hx, &hy);
  
  for (compte = 0, i = 0, pt = ppt->threads ;
       (pt != (p_thread)NULL) && (i < MAX_VIEWABLE_THREAD) ;
       compte++, i++, pt = pt->suivant)
    {
      if (compte == 8)
	{
	  compte = 0;
	}
      
      x = (compte * 64) + ((compte + 1) * 10) ;
      y = (i / 8) * 95 + 10 ;

      if(pt->running[0] == 'n')
	{
	  XCopyPlane(dpy_thread,
		     img_endormi,
		     xwin_thread,
		     gc_thread,
		     0, 0,
		     64, 64,
		     x, y,
		     1) ;
	}
      else if(pt->migratable[0] == 'y')
	{
	  XCopyPlane (dpy_thread,
		      img_thread,
		      xwin_thread,
		      gc_thread,
		      0, 0,
		      64, 64,
		      x, y,
		      1) ;
	}
      else
	{
	  XCopyPlane(dpy_thread,
		     img_boulet,
		     xwin_thread,
		     gc_thread,
		     0, 0,
		     64, 64,
		     x, y,
		     1) ;
	}

      XDrawString(dpy_thread,
		  xwin_thread,
		  gc_thread,
		  x, y + 85,
		  thread_view_func(pt),
		  strlen(thread_view_func(pt))
	 );
    }
  
  if(ppt->selection_thread != (p_thread)NULL)
    {
      int num ;

      num = numero_thread (ppt->threads, ppt->selection_thread);
      
      y = (num / 8);
      
      if (y > 0)
	{
	  x = num - (y * 8);
	}
      else
	{
	  x = num ;
	}
      
      x_rect = (x * 64) + (x * 10) + 7;
      y_rect = y * 95;

      XDrawRectangle(dpy_thread,
		     xwin_thread,
		     gc_rect_thread,
		     x_rect, y_rect + 5,
		     72, 73);
    }
}


void redessiner_pvm(Canvas canvas,
		    Xv_Window fenetre,
		    Display *dpy,
		    Window xwin,
		    Xv_xrectlist *xrects)
{
  int hx ;
  int hy ;
  int larg ;
  int haut ;
  int i ;
  int x ;
  
  unsigned long blanc ;
  unsigned long noir ;

  p_pvm_task ppt ;
  
  gc_pvm = DefaultGC(dpy, DefaultScreen(dpy));
  noir = BlackPixel(dpy, DefaultScreen(dpy));
  blanc = WhitePixel(dpy, DefaultScreen(dpy));

  gcv.function = GXinvert;
  gcv.foreground = noir;
  gcv.background = blanc;
  gcmask = GCForeground | GCBackground | GCFunction;
  gc_rect_pvm = XCreateGC(dpy, xwin, gcmask, &gcv);

  XReadBitmapFile(dpy,
		  xwin,
		  icon_task,
		  &larg, &haut,
		  &img_pvm_task,
		  &hx, &hy);

  dpy_pvm = dpy;
  xwin_pvm = xwin;

  if (selection_host != (p_host)NULL)
    {
      for (i = 0, ppt = selection_host->pvm_tasks ;
	   (ppt != (p_pvm_task)NULL) && (i < MAX_VIEWABLE_TASK) ;
	   i++, ppt = ppt->suivant)
	{
	  x = (i * 64) + ((i + 1) * 10) ;
      
	  XCopyPlane(dpy_pvm,
		     img_pvm_task,
		     xwin_pvm,
		     gc,
		     0, 0,
		     64, 64,
		     x, 10,
		     1);
      
	  XDrawString(dpy_pvm,
		      xwin_pvm,
		      gc,
		      x + 10, 95,
		      task_view_func(ppt),
		      strlen(task_view_func(ppt)));
	}
  
      if (selection_host->selection_pvm != (p_pvm_task)NULL)
	{
	  int num ;

	  num = numero_pvm_task (selection_host->pvm_tasks,
				 selection_host->selection_pvm
				 ) ;
      
	  x = (num * 64) + (num * 10) + 7 ;
      
	  XDrawRectangle(dpy_pvm,
			 xwin_pvm,
			 gc_rect_pvm,
			 x, 5,
			 72, 73);
	}
    }
}

void redessiner(Canvas canvas,
		Xv_window fenetre,
		Display *dpy,
		Window xwin,
		Xv_xrectlist *xrects)
{
  int hx ;
  int hy ;
  int larg ;
  int haut ;
  int i ;
  int x ;
  
  unsigned long blanc ;
  unsigned long noir ;

  p_host ph ;
  
  gc = DefaultGC(dpy, DefaultScreen(dpy)) ;
  noir = BlackPixel(dpy, DefaultScreen(dpy)) ;
  blanc = WhitePixel(dpy, DefaultScreen(dpy)) ;

  gcv.function = GXinvert;
  gcv.foreground = noir;
  gcv.background = blanc;
  gcmask = GCForeground | GCBackground | GCFunction;
  gc_rectangle = XCreateGC(dpy,xwin,gcmask,&gcv);

  XReadBitmapFile(dpy,
		  xwin,
		  icon_host,
		  &larg, &haut,
		  &img_host[0],
		  &hx, &hy);
  
  dpy_host = dpy;
  xwin_host = xwin;

  XClearWindow(dpy_host, xwin_host) ;

  for (i = 0, ph = hosts ;
       (ph != (p_host)NULL) && (i < MAX_VIEWABLE_HOST) ;
       i++, ph = ph->suivant)
    {
      x = (i * 64) + ((i + 1) * 10) ;
      
      XCopyPlane(dpy_host,
		 img_host[0],
		 xwin_host,
		 gc,
		 0, 0,
		 64, 64,
		 x, 10,
		 1);
      
      XDrawString(dpy_host,
		  xwin_host,
		  gc,
		  x, 95,
		  host_view_func(ph),
		  strlen(host_view_func(ph)));
    }
  
  if(selection_host != (p_host)NULL)
    {
      int num ;

      num = numero_host (hosts, selection_host);
      
      x = (num * 64) + (num * 10) + 7 ;
      XDrawRectangle(dpy_host,
		     xwin_host,
		     gc_rectangle,
		     x, 5,
		     70, 72);
    }
}

void tty_input()	/* on envoie la commande tapee vers la console */
{
  char buf[512];
  int n;

  stop_timer();
  
  n = read(tty_fd, buf, 512);
  write(pipe_io[0][1], buf, n);
}

void la_console_est_morte_paix_a_son_ame()
{
  xv_destroy_safe(racine);
}

void mode_console(Panel_item item, int value)
{
  verbose_mode = !(value) ;
}

void mode_refresh(Panel_item item, int value)
{
  if (value != 0)
    { /* Automatic refresh */
      automatic_refresh = TRUE ;
      timerclear(&refresh_timer.it_value) ;
      refresh_timer.it_value.tv_sec = REFRESH_INTERVAL ;
      refresh_timer.it_interval = refresh_timer.it_value ;
      notify_set_itimer_func(racine,
			     (Notify_func)refresh,
			     ITIMER_REAL, &refresh_timer,
			     NULL);
    }
  else
    {
      automatic_refresh = FALSE;
      notify_set_itimer_func(racine,
			     NOTIFY_FUNC_NULL,
			     ITIMER_REAL, NULL,
			     NULL);
    }
}

int execute_user_cmd(char *s)
{
  char cmd[128] ;
  char macro[32] ;
  char source[128] ;
  char tmp ;
  char *src = source ;
  char *dest = cmd;
  int x ;
  int y;

  strcpy(source, s) ;
  
  while(*src != '\0')
    {
      if(*src != '$')
	{
	  *dest = *src ;
	  src++ ;
	  dest++ ;
	}
      else if (*(src + 1) == '$')
	{
	  src++ ;
	  *dest = *src ;
	  src ++ ;
	  dest ++ ;
	}
      else
	{
	  char *deb = ++src ;

	  while (    (*src != '\0')
		  && (*src != ' ' )
		  && (*src != '\t'))
	    {
	      src++ ;
	    }
	  
	  tmp = *src ;
	  *src = '\0' ;
	  
	  if (strcmp(deb, "thread") == 0)
	    {
	      if(selection_host->selection_pvm->selection_thread == (p_thread)NULL)
		{
		  x = (int)xv_get(user_button, XV_X);
		  y = (int)xv_get(user_button, XV_Y);
		  (void)notice_prompt(racine, NULL,
				      NOTICE_FOCUS_XY, x + 40, y + 5,
				      NOTICE_MESSAGE_STRINGS,
				      "No thread selected",
				      NULL,
				      NOTICE_BUTTON, "Ok", 0,
				      NULL);
		  return FALSE;
		}
	      
	      strcpy
		(dest,
		 selection_host->selection_pvm->selection_thread->thread_id) ;
	      
	      dest += strlen
		(selection_host->selection_pvm->selection_thread->thread_id) ;
	    }
	  else if (strcmp(deb, "task") == 0)
	    {
	      x = (int)xv_get(user_button, XV_X);
	      y = (int)xv_get(user_button, XV_Y);
	      
	      if (selection_host->selection_pvm == (p_pvm_task)NULL)
		{
		  (void)notice_prompt(racine, NULL,
				      NOTICE_FOCUS_XY, x + 40, y + 5,
				      NOTICE_MESSAGE_STRINGS,
				      "No task selected",
				      NULL,
				      NOTICE_BUTTON, "Ok", 0,
				      NULL);
		  return FALSE;
		}
	      
	      strcpy(dest, selection_host->selection_pvm->tid);
	      dest += strlen(selection_host->selection_pvm->tid);
	    }
	  else if (strcmp(deb, "host") == 0)
	    {
	      x = (int)xv_get(user_button, XV_X);
	      y = (int)xv_get(user_button, XV_Y);
	      
	      if (selection_host == (p_host)NULL)
		{
		  (void)notice_prompt(racine, NULL,
				      NOTICE_FOCUS_XY, x + 40, y + 5,
				      NOTICE_MESSAGE_STRINGS,
				      "No host selected",
				      NULL,
				      NOTICE_BUTTON, "Ok", 0,
				      NULL);
		  return FALSE ;
		}
	      
	      strcpy(dest, selection_host->dtid);
	      dest += strlen(selection_host->dtid);
	    }
	  else
	    {
	      x = (int)xv_get(user_button, XV_X);
	      y = (int)xv_get(user_button, XV_Y);
	      (void)notice_prompt(racine, NULL,
				  NOTICE_FOCUS_XY, x + 40, y + 5,
				  NOTICE_MESSAGE_STRINGS,
				  "unknown macro",
				  NULL,
				  NOTICE_BUTTON, "Ok", 0,
				  NULL);
	      return FALSE;
	    }
	  *src = tmp;
	}
    }
  *dest = *src; /* pour le '\0' */

  xv_set(racine,
	 FRAME_BUSY, TRUE,
	 NULL);
  exec_command(USER, cmd);
  
  return TRUE;
}

void user_menu_proc(Menu menu, Menu_item item)
{
  execute_user_cmd((char *)xv_get(item, MENU_STRING)) ;
}

int user_button_proc(Panel_item item, Event *event)
{
  return XV_OK;
}

Panel_setting user_text_proc(Panel_item item, Event event)
{
  char *text ;
  char *text_copy ;
  int n;

  text = (char *)xv_get(item, PANEL_VALUE) ;
  
  text_copy = (char *)malloc(1 + strlen(text)) ;
  
  strcpy(text_copy, text) ;

  if (strcmp(text, "") != 0)
    {
      if (execute_user_cmd(text))
	{
	  xv_set(user_text, PANEL_VALUE, "", NULL);
	  xv_set(user_menu, MENU_INSERT, 0, xv_create((Panel)NULL, MENUITEM,
						      MENU_STRING, text_copy,
						      MENU_RELEASE,
						      NULL),
		 NULL);

	  n = (int)xv_get(user_menu, MENU_NITEMS);
    
	  if (n > MAX_USER_COMMANDS)
	    {
	      xv_set(user_menu, MENU_REMOVE, n, NULL);
	    }
	}
    }

  return PANEL_NONE;
}


/* Fonction  main  */
/*-----------====--*/
int main(int argc, char *argv[])
{ 
  int pid ;
  int i;
	
  if(getenv("PM2_ROOT") == NULL)
    {
      fprintf(stderr,
	      "Fatal error : PM2_ROOT environment variable not set.\n");
      exit(1);
    }

  etat = S_INITIAL_STATE;
  
  pipe(pipe_io[0]);
  pipe(pipe_io[1]);
  
  switch (pid = fork())
    {
      
    case -1 :
      {
	close(pipe_io[0][0]);
	close(pipe_io[0][1]);
	close(pipe_io[1][0]);
	close(pipe_io[1][1]);
	
	perror("fork failed");
	exit(1);
      }
    break ; /* le break n'est bien sur pas indispensable ici */
    
    case 0 :
      {
	dup2(pipe_io[0][0],0);
	dup2(pipe_io[1][1],1);
	dup2(pipe_io[1][1],2);
	
	for(i = 0 ; i < NSIG ; i++)
	  {
	    (void)signal(i, SIG_DFL);
	  }
	
	execvp(*my_argv, my_argv);

	if (errno == ENOENT)
	  {
	    printf("s : command not found\n", *argv);
	  }
	else
	  {
	    perror(*argv);
	  }
	
	perror("execvp");
	_exit(1);
      }
    break ;
    
    default :
      {
	close(pipe_io[0][0]);
	close(pipe_io[1][1]);
      }
    break ;
    
    }
 
  strcpy(icon_host, getenv("PM2_ROOT")) ;
  strcpy(icon_task, icon_host) ;
  strcpy(icon_thread, icon_host) ;
  strcpy(icon_boulet, icon_host) ;
  strcpy(icon_endormi, icon_host) ;
  strcpy(icon_panier, icon_host) ;
  strcpy(icon_panier_ouvert, icon_host) ;

  strcat(icon_host, "/console/icons/hutte.xbm") ;
  strcat(icon_task, "/console/icons/task.xbm") ;
  strcat(icon_thread, "/console/icons/thread.xbm") ;
  strcat(icon_boulet, "/console/icons/boulet.xbm") ;
  strcat(icon_endormi, "/console/icons/sleeping.xbm") ;
  strcat(icon_panier, "/console/icons/panier.xbm") ;
  strcat(icon_panier_ouvert, "/console/icons/panier_ouvert.xbm") ;

  selection_host = (p_host)NULL ;

  buf = (char *)malloc(MAX_BUF + 1) ;

  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL) ;

  window1_image = xv_create(XV_NULL, SERVER_IMAGE,
			    SERVER_IMAGE_DEPTH,	1,
			    SERVER_IMAGE_BITS, window1_bits,
			    XV_WIDTH, 64,
			    XV_HEIGHT, 64,
			    NULL);

  racine = (Frame)xv_create(XV_NULL, FRAME,
			    FRAME_LABEL, "Xpm2 v2.0",
			    XV_HEIGHT, 460,
			    XV_WIDTH, 600,
			    FRAME_SHOW_RESIZE_CORNER, FALSE,
			    FRAME_ICON, xv_create(XV_NULL, ICON,
						  ICON_IMAGE, window1_image,
						  ICON_TRANSPARENT, TRUE,
						  NULL),
			    NULL);

  notify_set_input_func(racine, read_it, pipe_io[1][0]);

  controle = (Panel)xv_create(racine, PANEL,
			      PANEL_LAYOUT, PANEL_HORIZONTAL,
			      XV_HEIGHT, 80,
			      XV_WIDTH, 600,
			      NULL);

  /* Definition du menu */

  menu_host = (Menu)xv_create
    ((Frame)NULL,MENU,
     MENU_ACTION_ITEM, "Info", mi_info_host,
     MENU_ACTION_ITEM, "Add", mi_add_host,
     MENU_ACTION_ITEM, "Delete", mi_delete_host,
     MENU_PULLRIGHT_ITEM, "View", xv_create
     ((Frame)NULL, MENU,
      MENU_ACTION_ITEM, "by NAME", by_hname_proc,
      MENU_ACTION_ITEM, "by HOST_ID", by_dtid_proc,
      MENU_ACTION_ITEM,	"by ARCH.", by_arch_proc,
      NULL),
     NULL);

  menu_pvm = (Menu)xv_create
    ((Frame)NULL,MENU,
     MENU_ACTION_ITEM, "Info", mi_info_pvm,
     MENU_ACTION_ITEM, "Delete", mi_delete_pvm,
     MENU_ACTION_ITEM, "Thread(s)", mi_thread_frame,
     MENU_PULLRIGHT_ITEM, "View", xv_create
     ((Frame)NULL, MENU,
      MENU_ACTION_ITEM, "by TASK_ID", by_tid_proc,
      MENU_ACTION_ITEM,	"by NAME", by_name_proc,
      NULL),
     NULL);

  menu_thread = (Menu)xv_create
    ((Frame)NULL, MENU,
     MENU_ACTION_ITEM, "Info", mi_info_thread,
     MENU_PULLRIGHT_ITEM, "Basket", xv_create
     ((Frame)NULL, MENU,
      MENU_ACTION_ITEM,	"Show", mi_voir_panier,
      MENU_ACTION_ITEM,	"Put into", mi_aller_dans_panier,
      MENU_ACTION_ITEM,	"Take from", mi_prendre_du_panier,
      NULL),
     MENU_PULLRIGHT_ITEM, "View", xv_create
     ((Frame)NULL, MENU,
      MENU_ACTION_ITEM,	"by PTHREAD_ID", by_pid_proc,
      MENU_ACTION_ITEM,	"by SERVICE", by_service_proc,
      MENU_ACTION_ITEM,	"by PRIORITY", by_prio_proc,
      NULL),
     NULL);

  (void)xv_create(controle, PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Hosts",
		  PANEL_ITEM_MENU, menu_host,
		  NULL);

  (void)xv_create(controle, PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Tasks",
		  PANEL_ITEM_MENU, menu_pvm,
		  NULL);

  (void)xv_create(controle, PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Threads",
		  PANEL_ITEM_MENU, menu_thread,
		  NULL);

  (void)xv_create(controle, PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Refresh",
		  PANEL_NOTIFY_PROC, force_refresh,
		  NULL);

  user_menu = (Menu)xv_create
    ((Panel)NULL, MENU,
     MENU_NOTIFY_PROC, user_menu_proc,
     MENU_STRINGS,
     "user -t $task",
     "ping $task",
     "user -t $task -enable_mig $thread",
     "user -t $task -disable_mig $thread",
     "user -t $task -save /tmp/toto",
     "user -t $task -load /tmp/toto",
     NULL,
     NULL);

  user_button = (Panel_item)xv_create(controle, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "User cmd",
				      PANEL_NOTIFY_PROC, user_button_proc,
				      PANEL_ITEM_MENU, user_menu,
				      NULL);

  user_text = (Panel_item)xv_create(controle, PANEL_TEXT,
				    PANEL_LABEL_STRING, "",
				    PANEL_VALUE_DISPLAY_LENGTH,	16,
				    PANEL_VALUE_STORED_LENGTH, 128,
				    PANEL_VALUE, "",
				    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
				    PANEL_NOTIFY_STRING, "\n\r",
				    PANEL_NOTIFY_PROC, user_text_proc,
				    NULL);

  (void)xv_create(controle, PANEL_CHOICE,
		  PANEL_LABEL_STRING, "Mode",
		  PANEL_CHOICE_STRINGS,	"Verbose", "Silent", NULL,
		  PANEL_NOTIFY_PROC, mode_console,
		  PANEL_VALUE, 1,
		  NULL);

  refresh_item = (Panel_item)xv_create
    (controle, PANEL_CHECK_BOX,
     PANEL_LABEL_STRING, "   ",
     PANEL_CHOICE_STRINGS, "Automatic refresh", NULL,
     PANEL_NOTIFY_PROC,	mode_refresh,
     PANEL_VALUE, 0,
     NULL);

  (void)xv_create(controle, PANEL_BUTTON,
		  PANEL_LABEL_STRING, "About ...",
		  PANEL_NOTIFY_PROC, au_sujet_de,
		  NULL);

  (void)xv_create(controle, PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Quit",
		  PANEL_NOTIFY_PROC, quitter,
		  NULL);

  controle1 = (Panel)xv_create(racine, PANEL,
			       PANEL_LAYOUT, PANEL_HORIZONTAL,
			       XV_X, 0,
			       XV_Y, 80,
			       XV_HEIGHT, 20,
			       XV_WIDTH, 305,
			       NULL);

  host_count_item = (Panel_item)xv_create(controle1, PANEL_MESSAGE,
					  PANEL_LABEL_STRING, "0 host(s)",
					  NULL);

  /* bouche trou */
  (void)xv_create(racine, PANEL,
		  XV_X, 295,
		  XV_Y, 100,
		  XV_WIDTH, 10,
		  XV_HEIGHT, 120,
		  NULL);

  canvas = (Canvas)xv_create(racine,CANVAS,
			     CANVAS_AUTO_EXPAND, TRUE,
			     CANVAS_AUTO_SHRINK, FALSE,
			     CANVAS_X_PAINT_WINDOW, TRUE,
			     CANVAS_WIDTH, MAX_VIEWABLE_HOST * 74,
			     CANVAS_HEIGHT, 120,
			     CANVAS_REPAINT_PROC, redessiner,
			     XV_X, 0,
			     XV_Y, 100,
			     XV_WIDTH, 295,
			     XV_HEIGHT, 120,
			     NULL);

  xv_set(canvas_paint_window(canvas),
	 WIN_EVENT_PROC, ma_procedure,
	 WIN_CONSUME_EVENTS, WIN_MOUSE_BUTTONS, NULL,
	 NULL);

  controle2 = (Panel)xv_create(racine, PANEL,
			       PANEL_LAYOUT, PANEL_HORIZONTAL,
			       XV_X, 305,
			       XV_Y, 80,
			       XV_HEIGHT, 20,
			       XV_WIDTH, 295,
			       NULL);

  task_count_item = (Panel_item)xv_create(controle2, PANEL_MESSAGE,
					  PANEL_LABEL_STRING, "0 task(s)",
					  NULL);

  canvas_pvm = (Canvas)xv_create(racine, CANVAS,
				 CANVAS_AUTO_EXPAND, TRUE,
				 CANVAS_AUTO_SHRINK, FALSE,
				 CANVAS_X_PAINT_WINDOW, TRUE,
				 CANVAS_WIDTH, MAX_VIEWABLE_TASK * 74,
				 CANVAS_HEIGHT, 120,
				 CANVAS_REPAINT_PROC, redessiner_pvm,
				 XV_X, 305,
				 XV_Y, 100,
				 XV_WIDTH, 295,
				 XV_HEIGHT, 120,
				 NULL);

  xv_set(canvas_paint_window(canvas_pvm),
	 WIN_EVENT_PROC, ma_procedure_pvm,
	 WIN_CONSUME_EVENTS, WIN_MOUSE_BUTTONS, NULL,
	 NULL);

  (void)xv_create(canvas, SCROLLBAR,
		  SCROLLBAR_SPLITTABLE,	TRUE,
		  SCROLLBAR_DIRECTION, SCROLLBAR_HORIZONTAL,
		  NULL);

  (void)xv_create(canvas_pvm, SCROLLBAR,
		  SCROLLBAR_SPLITTABLE,	TRUE,
		  SCROLLBAR_DIRECTION, SCROLLBAR_HORIZONTAL,
		  NULL);

  controle4 = (Panel)xv_create(racine, PANEL,
			       PANEL_LAYOUT, PANEL_HORIZONTAL,
			       XV_X, 0,
			       XV_Y, 220,
			       XV_HEIGHT, 20,
			       XV_WIDTH, 600,
			       NULL);

  (void)xv_create(controle4,
		  PANEL_MESSAGE,
		  PANEL_LABEL_STRING, "Text console : ",
		  NULL);

  ttysw = (Tty)xv_create(racine,
			 TERMSW,
			 XV_X, 0,
			 XV_Y, 240,
			 XV_HEIGHT, 240,
			 XV_WIDTH, 600,
			 TTY_ARGV, TTY_ARGV_DO_NOT_FORK,
			 NULL);

  tty_fd = (int)xv_get(ttysw, TTY_TTY_FD);

  notify_set_input_func(racine, (Notify_func)tty_input, tty_fd);

  notify_set_wait3_func
    (racine, (Notify_func)la_console_est_morte_paix_a_son_ame,
     pid);
	
  /* Necessaire pour contourner un bug de XView :
     les parametres du xv_create du cadre principal
     sont ignores par certaines versions de XView.
     Il faut donc les transmettre une seconde fois */
  xv_set
    (racine,
     FRAME_CLOSED, FALSE,
     XV_WIDTH, 600,
     XV_HEIGHT, 460,
     NULL);

  xv_main_loop(racine);
}

