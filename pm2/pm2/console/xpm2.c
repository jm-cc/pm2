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


enum {	INITIAL_STATE, S_ANY, S_ADD, S_CONF, S_DELETE, S_HALT, S_QUIT, S_PS, 
	TH, S_KILL, MIGR, FREEZE, USER };

#define MAX_HOST		50
#define MAX_TASK		10
#define MAX_THREAD		1000

#define MAX_VIEWABLE_HOST	32
#define MAX_VIEWABLE_TASK	10
#define MAX_VIEWABLE_THREAD	112

#define MAX_USER_COMMANDS	16

#include "old-lire_console.c"

#define min(a, b)	((a) < (b) ? (a) : (b))

int pipe_io[2][2];

char icon_host[128], icon_task[128], icon_thread[128];
char icon_boulet[128], icon_endormi[128], icon_blocke[128];
char icon_panier[128], icon_panier_ouvert[128];

char *my_argv[] = { "pm2", NULL };
int longueur = 0, etat;
int fenetre_cache_host = 1;
int fenetre_cache_pvm = 1;
int fenetre_cache_thread = 1;
int fenetre_cache_panier = 1;

int		gcCreated = False;
Display         *dpy1;
Window          xwin1;

Panel		controle,mes2,controle1,controle2,controle3,controle4,text_saisi;
Panel		Pan_Host[4],Pan_Pvm[6],Pan_Thread[7];
Panel_item	refresh_item, panier_count_item, panier_archi_item,
		host_count_item, task_count_item, thread_count_item,
		user_text, user_button;
Frame		racine, xv_add_host,
		conf_host = 0, conf_pvm = 0, conf_thread = 0,
		a_propos, panier_frame = 0;
Menu		menu, menu_host, menu_pvm, menu_thread,
		user_menu;

Xv_opaque	window1_image;

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


Canvas	    canvas,canvas_pvm,canvas_thread,canvas_panier;

Tty	ttysw;
int	tty_fd;


Display	*dpy_host,*dpy_pvm,*dpy_thread,*dpy_panier;
Window	xwin_host,xwin_pvm,xwin_thread,xwin_panier;
GC	gc,gc_rectangle,gc_rect_pvm,gc_pvm,gc_thread,gc_rect_thread,gc_panier;
XGCValues gcv;
unsigned long gcmask;
 
int largeur,hauteur;
Pixmap host[4],pvm_task,img_thread,img_boulet,img_endormi,img_blocke,img_panier,img_panier_ouvert;

char old_selected_host[128];
char old_selected_task[50];
char old_selected_thread[50];

int verbose_mode = 0;

struct itimerval refresh_timer;

/* refreshing period in seconds : */
#define REFRESH_INTERVAL	5

int automatic_refresh = FALSE;

void stop_timer()
{
	automatic_refresh = FALSE;
	notify_set_itimer_func(racine, NOTIFY_FUNC_NULL,
		ITIMER_REAL,	NULL,
		NULL);
	xv_set(refresh_item, PANEL_VALUE, 0, NULL);
}

typedef char* (*view_func)(int n);

char *host_by_name(int n);
char *host_by_dtid(int n);
char *host_by_arch(int n);

char *task_by_name(int n);
char *task_by_tid(int n);

char *thread_by_pid(int n);
char *thread_by_service(int n);
char *thread_by_prio(int n);

view_func	host_view_func	=	&host_by_name,
		task_view_func	=	&task_by_tid,
		thread_view_func =	&thread_by_service;

char *host_by_name(int n) { return table_host[n].nom; }
char *host_by_dtid(int n) { return table_host[n].dtid; }
char *host_by_arch(int n) { return table_host[n].archi; }

char *task_by_name(int n) { return table_pvm[n].command; }
char *task_by_tid(int n) { return table_pvm[n].tid; }

char *thread_by_pid(int n) { return table_thread[n].thread_id; }
char *thread_by_service(int n) { return table_thread[n].service; }
char *thread_by_prio(int n) { return table_thread[n].prio; }

void exec_command(int command,char *param);

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

void maj_host_count()
{ char message[64];

	sprintf(message, "%d host(s)", nb_host);
	xv_set(host_count_item, PANEL_LABEL_STRING, message, NULL);
}

void maj_task_count()
{ char message[64];

	sprintf(message, "%d task(s)", nb_pvm);
	xv_set(task_count_item, PANEL_LABEL_STRING, message, NULL);
}

void maj_thread_count()
{ char message[64];

	sprintf(message, "%d thread(s)", nb_thread);
	xv_set(thread_count_item, PANEL_LABEL_STRING, message, NULL);
}

void mettre_a_jour_host(void)
{
	char message[128];

	if(!fenetre_cache_host) {

		if(selection_courante == -1) {
			int i;

			xv_set(Pan_Host[0], PANEL_LABEL_STRING, "*** No Host selected ***", NULL);
			for(i=1; i<3; i++)
				xv_set(Pan_Host[i], PANEL_LABEL_STRING, "", NULL);
		} else {
			sprintf(message,"Host Name : %s",table_host[selection_courante].nom);
			xv_set(Pan_Host[0],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"DTID : %s",table_host[selection_courante].dtid);
			xv_set(Pan_Host[1],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"Archi : %s",table_host[selection_courante].archi);
			xv_set(Pan_Host[2],PANEL_LABEL_STRING,message,NULL);
/*
			sprintf(message,"SPEED : %s",table_host[selection_courante].speed);
			xv_set(Pan_Host[3],PANEL_LABEL_STRING,message,NULL);
*/
		}
	}
}

void mettre_a_jour_pvm(void)
{
	char message[128];

	if(!fenetre_cache_pvm) {

		if(selection_pvm == -1) {
			int i;

			xv_set(Pan_Pvm[0], PANEL_LABEL_STRING, "*** No task selected ***", NULL);
			for(i=1; i<3; i++)
				xv_set(Pan_Pvm[i], PANEL_LABEL_STRING, "", NULL);
		} else {
			sprintf(message,"Host Name : %s",table_pvm[selection_pvm].host);
			xv_set(Pan_Pvm[0],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"TID : %s",table_pvm[selection_pvm].tid);
			xv_set(Pan_Pvm[1],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"COMMAND : %s",table_pvm[selection_pvm].command);
			xv_set(Pan_Pvm[2],PANEL_LABEL_STRING,message,NULL);
/*
			sprintf(message,"THREADS : %s",table_pvm[selection_pvm].threads);
			xv_set(Pan_Pvm[3],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"ACTIVE : %s",table_pvm[selection_pvm].active);
			xv_set(Pan_Pvm[4],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"BLOCKED : %s",table_pvm[selection_pvm].blocked);
			xv_set(Pan_Pvm[5],PANEL_LABEL_STRING,message,NULL);
*/
		}
	}
}

void mettre_a_jour_thread(void)
{
	char message[128];

	if(!fenetre_cache_thread) {

		if(selection_thread == -1) {
			int i;

			xv_set(Pan_Thread[0], PANEL_LABEL_STRING, "*** No thread selected ***", NULL);
			for(i=1; i<7; i++)
				xv_set(Pan_Thread[i], PANEL_LABEL_STRING, "", NULL);
		} else {
			sprintf(message,"Thread_ID : %s",table_thread[selection_thread].thread_id);
			xv_set(Pan_Thread[0],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"NUM : %s",table_thread[selection_thread].num);
			xv_set(Pan_Thread[1],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"PRIO : %s",table_thread[selection_thread].prio);
			xv_set(Pan_Thread[2],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"RUNNING : %s",table_thread[selection_thread].running);
			xv_set(Pan_Thread[3],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"STACK SIZE : %s",table_thread[selection_thread].stack);
			xv_set(Pan_Thread[4],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"MIGRATABLE : %s",table_thread[selection_thread].migratable);
			xv_set(Pan_Thread[5],PANEL_LABEL_STRING,message,NULL);
			sprintf(message,"SERVICE : %s",table_thread[selection_thread].service);
			xv_set(Pan_Thread[6],PANEL_LABEL_STRING,message,NULL);
		}
	}
}

void exec_command(int command,char *param)
{
	char comm[128];

	switch(command)
	{
	case S_ADD :
		strcpy(comm,"add ");
		strcat(comm,param);
		strcat(comm, "\n");
		etat = S_ADD;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1],comm,strlen(comm));
		break;
	case S_CONF :
		strcpy(comm,"conf");
		strcat(comm, "\n");
		etat = S_CONF;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1],comm,strlen(comm));
		etat=S_CONF;
		break;
	case S_DELETE :
		strcpy(comm,"delete ");
		strcat(comm,param);
		strcat(comm, "\n");
		etat = S_DELETE;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1],comm,strlen(comm));	
		break;
	case S_HALT :
		strcpy(comm,"halt");
		strcat(comm, "\n");
		etat = S_HALT;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1],comm,strlen(comm));
		break;
	case S_QUIT :
		strcpy(comm,"quit");
		strcat(comm, "\n");
		etat = S_QUIT;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1],comm,strlen(comm));
		break;
	case S_PS :
		strcpy(comm,"ps a");
		strcat(comm, "\n");
		etat = S_PS;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1],comm,strlen(comm));	
		break;
	case TH :
		strcpy(comm,"th ");
		strcat(comm,param);
		strcat(comm, "\n");
		etat = TH;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1],comm,strlen(comm));	
		break;
	case S_KILL :
		strcpy(comm, "kill ");
		strcat(comm, param);
		strcat(comm, "\n");
		etat = S_KILL;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1],comm,strlen(comm));	
		break;
	case FREEZE :
		strcpy(comm, "freeze ");
		strcat(comm, param);
		strcat(comm, "\n");
		etat = FREEZE;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1], comm, strlen(comm));
		break;
	case MIGR :
		strcpy(comm, "mig ");
		strcat(comm, param);
		strcat(comm, "\n");
		etat = MIGR;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1], comm, strlen(comm));
		break;
	case USER :
		strcpy(comm, param);
		strcat(comm, "\n");
		etat = USER;
		if(verbose_mode) write(tty_fd, comm, strlen(comm));
		write(pipe_io[0][1], comm, strlen(comm));
		break;
	}
}


void user_results_done_proc(subframe)
Frame subframe;
{ Textsw t;

	t = (Textsw)xv_get(subframe, XV_KEY_DATA, 0);
	textsw_reset(t, 0, 0);

	xv_destroy_safe(subframe);
}

unsigned long MAX_BUF = 65000;

char *buf;
int offset = 0;

Notify_value
read_it(client,fd)
Notify_client client;
int fd;
{
	int bytes,i,x,x_rect,y,y_rect,ligne_courante,j,compte;
	int res, retrouve;

	if(offset == MAX_BUF) {
/*		fprintf(stderr, "Reallocating buffer...\n"); */
		MAX_BUF += 10000;
		buf = (char *)realloc(buf, MAX_BUF+1);
	}

	do {
		if ((i = read(fd, buf+offset, MAX_BUF - offset)) < 0) {
			fprintf(stderr, "Error : read failed !\n");
			exit(1);
		}
		offset += i;

	} while(i == MAX_BUF);


	buf[offset] = 0;

	if(etat != S_QUIT && etat != S_HALT) {
		char *fin = buf + offset - 4;
		if(offset < 4 || strcmp(fin, "pm2>"))
			return NOTIFY_DONE;
	}

	if(etat == S_ANY || verbose_mode)
		(void)write(tty_fd, buf, offset);

	switch(etat)
	{
	case INITIAL_STATE :
		xv_set(racine, FRAME_BUSY, TRUE, NULL);
		exec_command(S_CONF, NULL);
		if(!verbose_mode) {	/* curieux non ? */
			char prompt[4]="pm2>";

			write(tty_fd, prompt, 4);
		}
		break;
	case S_ANY :
		break;
	case S_CONF :
		if(selection_courante == -1)
			strcpy(old_selected_host, "");
		else
			strcpy(old_selected_host, table_host[selection_courante].nom);
		selection_courante = -1;

		extract_conf(buf);
		XClearWindow(dpy_host,xwin_host);
		for (i=0;i<min(nb_host, MAX_VIEWABLE_HOST);i++)
   		{
    			x=(i*64)+((i+1)*10);
    			XCopyPlane(dpy_host,host[0],xwin_host,gc,0,0,64,64,x,10,1);
    			XDrawString(dpy_host, xwin_host, gc, x, 95,
       			(*host_view_func)(i), strlen((*host_view_func)(i)));
			if(!strcmp(table_host[i].nom, old_selected_host))
				selection_courante = i;
   		}
		maj_host_count(); mettre_a_jour_host();
		if(selection_courante == -1) {
			selection_pvm = -1; nb_pvm = 0;
			XClearWindow(dpy_pvm,xwin_pvm);
			maj_task_count(); mettre_a_jour_pvm();

			selection_thread = -1; nb_thread = 0;
			XClearWindow(dpy_thread,xwin_thread);
			maj_thread_count();mettre_a_jour_thread();

			xv_set(racine, FRAME_BUSY, FALSE, NULL);
			etat = S_ANY;
		} else {
			x_rect = (selection_courante*64)+((selection_courante)*10)+7;
			XDrawRectangle(dpy_host,xwin_host,gc_rectangle,x_rect,5,70,72);
			exec_command(S_PS, NULL);
		}
		break;
	case S_DELETE :
		exec_command(S_CONF,NULL);
		break;
	case S_ADD :
		exec_command(S_CONF,NULL);
	 	break; 
	case S_PS :
		if(selection_pvm == -1)
			strcpy(old_selected_task, "");
		else
			strcpy(old_selected_task, table_pvm[selection_pvm].tid);
		selection_pvm = -1;

		extract_ps(buf);
		XClearWindow(dpy_pvm,xwin_pvm);
		compte = 0;
		for (i=0;i<min(nb_pvm, MAX_VIEWABLE_TASK);i++)
   		{
    			x=(i*64)+((i+1)*10);
			XCopyPlane(dpy_pvm,pvm_task,xwin_pvm,gc,0,0,64,64,x,10,1);
    			XDrawString(dpy_pvm, xwin_pvm, gc, x+10, 95,
        			(*task_view_func)(i), strlen((*task_view_func)(i)));
 			if(!strcmp(table_pvm[i].tid, old_selected_task))
				selection_pvm = i;
  		}
		maj_task_count(); mettre_a_jour_pvm();
		if(selection_pvm == -1) {
			selection_thread = -1; nb_thread = 0;
			XClearWindow(dpy_thread,xwin_thread);
			maj_thread_count(); mettre_a_jour_thread();

			xv_set(racine, FRAME_BUSY, FALSE, NULL);
			etat = S_ANY;
		} else {
			x = (selection_pvm*64)+((selection_pvm)*10)+7;
			XDrawRectangle(dpy_pvm,xwin_pvm,gc_rect_pvm,x,5,72,73);
			exec_command(TH, table_pvm[selection_pvm].tid);
		}
		break;
	case TH :
		if(selection_thread == -1)
			strcpy(old_selected_thread, "");
		else
			strcpy(old_selected_thread, table_thread[selection_thread].num);
		selection_thread = -1;

		extract_th(buf);
		XClearWindow(dpy_thread, xwin_thread);
		compte = 0;
		for (i=0;i<min(nb_thread, MAX_VIEWABLE_THREAD);i++)
   		{
    			if (compte == 8) compte =0;
    			x=(compte*64)+((compte+1)*10);
 			y = (i/8)*95+10;

			if(table_thread[i].running[0] == 'a')
				XCopyPlane(dpy_thread,img_endormi,xwin_thread,gc,0,0,64,64,x,y,1);
			else if(table_thread[i].running[0] == 'b')
				XCopyPlane(dpy_thread,img_blocke,xwin_thread,gc,0,0,64,64,x,y,1);
			else if(table_thread[i].migratable[0] == 'y')
				XCopyPlane(dpy_thread,img_thread,xwin_thread,gc,0,0,64,64,x,y,1);
			else
				XCopyPlane(dpy_thread,img_boulet,xwin_thread,gc,0,0,64,64,x,y,1);

			XDrawString(dpy_thread, xwin_thread, gc, x, y+85,
       			 	(*thread_view_func)(i), strlen((*thread_view_func)(i)));
			compte ++;
			if(!strcmp(table_thread[i].num, old_selected_thread))
				selection_thread = i;
   		}
		maj_thread_count(); mettre_a_jour_thread();
		if(selection_thread != -1) {
			y = (selection_thread / 8); 
			if (y > 0)
				x = selection_thread - (y*8);
			else
				x = selection_thread;
			x_rect = (x*64)+((x)*10)+7;
			y_rect = (y) * 95;
			XDrawRectangle(dpy_thread,xwin_thread,gc_rect_thread,x_rect,y_rect+5,72,73);
		}
		if(nb_thread == 0) { /* Pas courant hein ? */
			exec_command(S_PS, NULL);
		} else {
			xv_set(racine, FRAME_BUSY, FALSE, NULL);
			etat = S_ANY;
		}
		break;
	case S_HALT :
		xv_set(racine, FRAME_BUSY, FALSE, NULL);
		etat = S_ANY;
		break;
	case S_QUIT :
		xv_set(racine, FRAME_BUSY, FALSE, NULL);
		etat = S_ANY;
		break;
	case S_KILL :
		exec_command(S_PS,NULL);
		break;
	case MIGR :
		if(selection_pvm == -1) {
			xv_set(racine, FRAME_BUSY, FALSE, NULL);
			etat = S_ANY;
		} else {
			exec_command(TH, table_pvm[selection_pvm].tid);
		}
		break;
	case FREEZE :
		if(selection_pvm == -1) {
			xv_set(racine, FRAME_BUSY, FALSE, NULL);
			etat = S_ANY;
		} else {
			exec_command(TH, table_pvm[selection_pvm].tid);
		}
		break;
	case USER :
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
		break;
	}
	offset = 0;
	return NOTIFY_DONE;
}


typedef char serpent_de_panier[32];

struct {
	char archi[32];
	char task_id[32];
	int nb;
	serpent_de_panier thread[50];
} panier;

void mettre_a_jour_panier()
{	char message[128];

	if(!fenetre_cache_panier) {
		if(panier.nb)
			XCopyPlane(dpy_panier,img_panier_ouvert,xwin_panier,gc,0,0,64,64,0,0,1);
		else
			XCopyPlane(dpy_panier,img_panier,xwin_panier,gc,0,0,64,64,0,0,1);

		sprintf(message, "(%d threads)", panier.nb);
		xv_set(panier_count_item, PANEL_LABEL_STRING, message, NULL);
		if(panier.nb)
			sprintf(message, "(Archi = %s)", panier.archi);
		else
			sprintf(message, "");
		xv_set(panier_archi_item, PANEL_LABEL_STRING, message, NULL);
	}
}

void aller_dans_panier(Menu_item item, Menu_generate op)
{	char message[64], args[128];
	int result, x, y, x_rect, y_rect;

	if(selection_thread == -1) {

		result = notice_prompt(canvas_paint_window(canvas_thread), NULL,
			NOTICE_FOCUS_XY,	300,			100,
			NOTICE_MESSAGE_STRINGS,	"No thread selected",	NULL,
			NOTICE_BUTTON,		"Ok",			0,	
			NULL);

	} else {
		y = (selection_thread / 8);
		if (y > 0)
			x = selection_thread - (y*8);
		else
			x = selection_thread;
		x_rect = (x*64)+((x)*10)+39;
		y_rect = (y) * 95+32;

		if(table_thread[selection_thread].migratable[0] != 'y') {

			result = notice_prompt(canvas_paint_window(canvas_thread), NULL,
				NOTICE_FOCUS_XY,	x_rect,				y_rect,
				NOTICE_MESSAGE_STRINGS,	"Thread is not migratable",	NULL,
				NOTICE_BUTTON,		"Ok",				0,	
				NULL);

		} else if(panier.nb && strcmp(panier.task_id, table_pvm[selection_pvm].tid)) {

			sprintf(message, "Thread must come from task %s", panier.task_id);

			result = notice_prompt(canvas_paint_window(canvas_thread), NULL,
				NOTICE_FOCUS_XY,	x_rect,		y_rect,
				NOTICE_MESSAGE_STRINGS,	message,	NULL,
				NOTICE_BUTTON,		"Ok",		0,	
				NULL);

		} else {
			if(!panier.nb) {
				strcpy(panier.archi, table_host[selection_courante].archi);
				strcpy(panier.task_id, table_pvm[selection_pvm].tid);
			}
			strcpy(panier.thread[panier.nb++], table_thread[selection_thread].thread_id);

			sprintf(args, "%s %s", table_pvm[selection_pvm].tid, table_thread[selection_thread].thread_id);

			mettre_a_jour_panier();

			xv_set(racine, FRAME_BUSY, TRUE, NULL);
			exec_command(FREEZE, args);
		}
	}
}

void prendre_du_panier(Menu_item item, Menu_generate op)
{	int i, result, x;
	char message[64], args[1024];

	if(!panier.nb) {

		result = notice_prompt(canvas_paint_window(canvas_thread), NULL,
			NOTICE_FOCUS_XY,	300,				100,
			NOTICE_MESSAGE_STRINGS,	"Threads basket is empty",	NULL,
			NOTICE_BUTTON,		"Ok",				0,	
			NULL);

	} else if(selection_pvm == -1) {

		result = notice_prompt(canvas_paint_window(canvas_pvm), NULL,
			NOTICE_FOCUS_XY,	150,			50,
			NOTICE_MESSAGE_STRINGS,	"No task selected",	NULL,
			NOTICE_BUTTON,		"Ok",			0,	
			NULL);

	} else if(strcmp(panier.archi, table_host[selection_courante].archi)) {

		x = (selection_courante*64)+((selection_courante)*10)+39;

		sprintf(message, "In the basket, threads come from a %s architecture", panier.archi);

		result = notice_prompt(canvas_paint_window(canvas), NULL,
			NOTICE_FOCUS_XY,	x,		50,
			NOTICE_MESSAGE_STRINGS,	message,	NULL,
			NOTICE_BUTTON,		"Ok",		0,	
			NULL);

	} else {
		sprintf(args, "%s %s ", panier.task_id, table_pvm[selection_pvm].tid);
		for(i=0; i<panier.nb; i++) {
			strcat(args, panier.thread[i]);
			strcat(args, " ");
		}
		panier.nb = 0;
		mettre_a_jour_panier();
		xv_set(racine, FRAME_BUSY, TRUE, NULL);
		exec_command(MIGR, args);
	}
}

done_panier(subframe)
Frame subframe;
{
	fenetre_cache_panier = 1;
	xv_set(subframe,XV_SHOW,FALSE,NULL);
}

void
redessiner_panier(canvas, fenetre, dpy, xwin, xrects)
Canvas          canvas;
Xv_Window       fenetre;
Display         *dpy;
Window          xwin;
Xv_xrectlist    *xrects;
{
 int hx,hy,larg,haut,i,x,y,x_rect,y_rect,compte;
	unsigned long blanc,noir;

 	gc_panier=DefaultGC(dpy,DefaultScreen(dpy));

	dpy_panier = dpy;
	xwin_panier = xwin;

	XReadBitmapFile(dpy,xwin,icon_panier,&larg,&haut,&img_panier,&hx,&hy);
	XReadBitmapFile(dpy,xwin,icon_panier_ouvert,&larg,&haut,&img_panier_ouvert,&hx,&hy);

	if(panier.nb)
		XCopyPlane(dpy,img_panier_ouvert,xwin,gc,0,0,64,64,0,0,1);
	else
		XCopyPlane(dpy,img_panier,xwin,gc,0,0,64,64,0,0,1);
}

Menu_item voir_panier(menu, item)
Menu menu;
Menu_item item;
{	char message[128];
	Panel panier_panel;

	if(!panier_frame) {
		panier_frame = xv_create(racine, FRAME_CMD,
        		XV_WIDTH,		200,
        		XV_HEIGHT,		130,
        		FRAME_LABEL,		"Threads Basket",
			FRAME_DONE_PROC,	done_panier,
 			FRAME_CMD_PUSHPIN_IN,	TRUE,
       			NULL);

		panier_panel=xv_get(panier_frame,FRAME_CMD_PANEL,NULL);

		canvas_panier=(Canvas)xv_create(panier_frame, CANVAS,
			CANVAS_AUTO_EXPAND,	FALSE,
			CANVAS_AUTO_SHRINK,	FALSE,
			CANVAS_X_PAINT_WINDOW,	TRUE,
			CANVAS_WIDTH,		64,
			CANVAS_HEIGHT,		64,
			CANVAS_REPAINT_PROC,	redessiner_panier,
			XV_X,			68,
			XV_Y,			10,
			XV_WIDTH,		64,
			XV_HEIGHT,		64,
			NULL);

		sprintf(message, "%d thread(s)", panier.nb);

		panier_count_item = (Panel_item)xv_create(panier_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING,	message,
			XV_X,			50,
			XV_Y,			85,
			NULL);

		if(panier.nb)
			sprintf(message, "(Archi = %s)", panier.archi);
		else
			sprintf(message, "");

		panier_archi_item = (Panel_item)xv_create(panier_panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING,	message,
			XV_X,			30,
			XV_Y,			110,
			NULL);

		fenetre_cache_panier = 0;

		xv_set(panier_frame,XV_SHOW,TRUE,NULL);
	} else {
		fenetre_cache_panier = 0;

		xv_set(panier_frame,XV_SHOW,TRUE,NULL);
		mettre_a_jour_panier();
	}
}

done_proc_thread(subframe)
Frame subframe;
{
	fenetre_cache_thread = 1;
	xv_set(subframe,XV_SHOW,FALSE,NULL);
}

Menu_item info_thread(menu, item)
Menu menu;
Menu_item item;
{
	Panel panel_conf;

	if(!conf_thread) {

		conf_thread = xv_create(racine, FRAME_CMD,
        		XV_WIDTH,		200,
        		XV_HEIGHT,		180,
        		FRAME_LABEL,		"Thread Info",
			FRAME_DONE_PROC,	done_proc_thread,
 			FRAME_CMD_PUSHPIN_IN,	TRUE,
        		NULL);

		panel_conf=xv_get(conf_thread,FRAME_CMD_PANEL,NULL);

		Pan_Thread[0] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			5,
			NULL);

		Pan_Thread[1] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			30,
			NULL);

		Pan_Thread[2] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			55,
			NULL);

		Pan_Thread[3] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			80,
			NULL);

		Pan_Thread[4] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			105,
			NULL);

		Pan_Thread[5] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			130,
			NULL);

		Pan_Thread[6] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			155,
			NULL);

		fenetre_cache_thread = 0;
		mettre_a_jour_thread();
		xv_set(conf_thread,XV_SHOW,TRUE,NULL);
	} else {
		fenetre_cache_thread = 0;
		mettre_a_jour_thread();
		xv_set(conf_thread,XV_SHOW,TRUE,NULL);
	}
}

done_proc_pvm(subframe)
Frame subframe;
{
	fenetre_cache_pvm = 1;
	xv_set(subframe,XV_SHOW,FALSE,NULL);
}


Menu_item info_pvm(menu,item)
Menu menu;
Menu_item item;
{
	Panel panel_conf;

	if(!conf_pvm)
	{
		conf_pvm = xv_create(racine, FRAME_CMD,
        		XV_WIDTH,		200,
        		XV_HEIGHT,		80,
        		FRAME_LABEL,		"Task Info",
			FRAME_DONE_PROC,	done_proc_pvm,
 			FRAME_CMD_PUSHPIN_IN,	TRUE,
        		NULL);

		panel_conf=xv_get(conf_pvm,FRAME_CMD_PANEL,NULL);

		Pan_Pvm[0] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			5,
			NULL);

		Pan_Pvm[1] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			30,
			NULL);

		Pan_Pvm[2] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			55,
			NULL);

/*
		Pan_Pvm[3] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			80,
			NULL);

	
		Pan_Pvm[4] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			105,
			NULL);

		Pan_Pvm[5] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			130,
			NULL);
*/
		fenetre_cache_pvm = 0;
		mettre_a_jour_pvm();
		xv_set(conf_pvm,XV_SHOW,TRUE,NULL);

	} else {
		fenetre_cache_pvm = 0;
		mettre_a_jour_pvm();
		xv_set(conf_pvm,XV_SHOW,TRUE,NULL);
	}
}

Menu_item delete_pvm(menu, item)
Menu menu;
Menu_item item;
{
	int result, x;

	if(selection_pvm == -1) {

		result = notice_prompt(canvas_paint_window(canvas_pvm), NULL,
			NOTICE_FOCUS_XY,	150,			50,
			NOTICE_MESSAGE_STRINGS,	"No task selected",	NULL,
			NOTICE_BUTTON,		"Ok",			0,	
			NULL);

	} else {
		x = (selection_pvm*64)+((selection_pvm)*10)+39;

		result = notice_prompt(canvas_paint_window(canvas_pvm),	NULL,
			NOTICE_FOCUS_XY,	x,	50,
			NOTICE_MESSAGE_STRINGS,	"Do you really want to kill this task ?",	NULL,
			NOTICE_BUTTON_YES,	"Yes",
			NOTICE_BUTTON_NO,	"No",
			NULL);

		if(result == NOTICE_YES)
		{
			xv_set(racine, FRAME_BUSY, TRUE, NULL);
			exec_command(S_KILL, table_pvm[selection_pvm].tid);		
		}
	}
}

void validate_host(void)
{
	char machine[128];

	strcpy(machine,(char *)xv_get(text_saisi,PANEL_VALUE));
	xv_set(racine, FRAME_BUSY, TRUE, NULL);
	exec_command(S_ADD, machine);
	xv_destroy_safe(xv_add_host);
}

Menu_item add_host(menu, item)
Menu menu;
Menu_item item;
{
	char param[256];
	Panel panel_host;

	xv_add_host = xv_create(racine, FRAME_CMD,
		XV_WIDTH,	200,
		XV_HEIGHT,	60,
		FRAME_LABEL,	"Add Host",
		NULL);

	panel_host=xv_get(xv_add_host,FRAME_CMD_PANEL,NULL);

	text_saisi=(Panel)xv_create(panel_host,PANEL_TEXT,
		PANEL_LABEL_STRING,	"Host Name : ",
		PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING,	"\n\r",
		PANEL_NOTIFY_PROC,	validate_host,
		NULL);

	(void)xv_create(panel_host,PANEL_BUTTON,
		PANEL_LABEL_STRING,	"Ok",
		PANEL_NOTIFY_PROC,	validate_host,
		XV_X,			80,
		NULL);

	xv_set(xv_add_host, XV_SHOW, TRUE, NULL);

}

void validate_info_host(void)
{
	xv_destroy_safe(conf_host);
}

done_proc_host(subframe)
Frame subframe;
{
	fenetre_cache_host = 1;
	xv_set(subframe,XV_SHOW,FALSE,NULL);
}


Menu_item info_host(menu, item)
Menu menu;
Menu_item item;
{
	Panel panel_conf;

	if(!conf_host)
	{
		conf_host = xv_create(racine, FRAME_CMD,
        		XV_WIDTH,		200,
        		XV_HEIGHT,		80,
        		FRAME_LABEL,		"Host Info",
			FRAME_DONE_PROC,	done_proc_host,
 			FRAME_CMD_PUSHPIN_IN,	TRUE,
        		NULL);

		panel_conf=xv_get(conf_host,FRAME_CMD_PANEL,NULL);

		Pan_Host[0] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			5,
			NULL);

		Pan_Host[1] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			30,
			NULL);

		Pan_Host[2] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			55,
			NULL);
/*
		Pan_Host[3] = xv_create(panel_conf,PANEL_MESSAGE,
			PANEL_LABEL_STRING,	"",
			XV_X,			0,
			XV_Y,			80,
			NULL);
*/
		fenetre_cache_host = 0;
		mettre_a_jour_host();
		xv_set(conf_host,XV_SHOW,TRUE,NULL);
	} else {
		fenetre_cache_host = 0;
		mettre_a_jour_host();
		xv_set(conf_host,XV_SHOW,TRUE,NULL);
	}
}

Menu_item delete_host(menu, item)
Menu menu;
Menu_item item;
{
	int result,x;

	if(selection_courante == -1) {

		result = notice_prompt(canvas_paint_window(canvas), NULL,
			NOTICE_FOCUS_XY,	150,			50,
			NOTICE_MESSAGE_STRINGS,	"No host selected",	NULL,
			NOTICE_BUTTON,		"Ok",			0,	
			NULL);

	} else {
		x = (selection_courante*64)+((selection_courante)*10)+39;

		result = notice_prompt(canvas_paint_window(canvas), NULL,
			NOTICE_FOCUS_XY,	x,	50,
			NOTICE_MESSAGE_STRINGS,	"Do you really want to delete this host ?",	NULL,
			NOTICE_BUTTON_YES,	"Yes",
			NOTICE_BUTTON_NO,	"No",
			NULL);

		if(result == NOTICE_YES) {
			xv_set(racine, FRAME_BUSY, TRUE, NULL);
			exec_command(S_DELETE,table_host[selection_courante].nom);
		}
	}
}

void au_sujet_de(void)
{
	Panel panel_conf;
	char message[128];

	a_propos = xv_create(racine, FRAME_CMD,
		XV_WIDTH,	200,
		XV_HEIGHT,	150,
		FRAME_LABEL,	"About Xpm2 designers",
		NULL);

	panel_conf=xv_get(a_propos,FRAME_CMD_PANEL,NULL);
	
	sprintf(message,"X-Console For PM2");
	(void) xv_create(panel_conf, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	message,
		PANEL_LABEL_BOLD,	TRUE,
		XV_X,			50,
		XV_Y,			5,
		NULL);

	sprintf(message,"Programmed By :");
	(void) xv_create(panel_conf, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	message,
		PANEL_LABEL_BOLD,	TRUE,
		XV_X,			0,
		XV_Y,			30,
		NULL);

	sprintf(message,"Gilles CYMBALISTA");
	(void) xv_create(panel_conf, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	message,
		XV_X,			10,
		XV_Y,			50,
		NULL);
	
	sprintf(message,"Yvon HUBERT");
	(void) xv_create(panel_conf, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	message,
		XV_X,			10,
		XV_Y,			70,
		NULL);

	sprintf(message,"Technical Help : ");
	(void) xv_create(panel_conf, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	message,
		PANEL_LABEL_BOLD,	TRUE,
		XV_X,			0,
		XV_Y,			110,
		NULL);

	sprintf(message,"Raymond NAMYST");
	(void) xv_create(panel_conf, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	message,
		XV_X,			10,
		XV_Y,			130,
		NULL);

	xv_set(a_propos,XV_SHOW,TRUE,NULL);
}

int refresh(void) /* appele par le timer */
{
	if(automatic_refresh && etat == S_ANY) {
		xv_set(racine, FRAME_BUSY, TRUE, NULL);
		exec_command(S_CONF, NULL);
	}
	return NOTIFY_DONE;
}

int force_refresh()	/* appele par le bouton refresh */
{
	xv_set(racine, FRAME_BUSY, TRUE, NULL);
	exec_command(S_CONF, NULL);
	return NOTIFY_DONE;
}

void quitter(item, event)
Panel_item item;
Event *event;
{
	int result;

	result = notice_prompt(racine,NULL,
		NOTICE_FOCUS_XY,	event_x(event),		event_y(event),
		NOTICE_MESSAGE_STRINGS,	"Do you really want to quit ?",	NULL,
		NOTICE_BUTTON_YES,	"Yes",
		NOTICE_BUTTON_NO,	"No",
		NULL);

	if(result == NOTICE_YES) {
	  xv_set(racine, FRAME_BUSY, TRUE, NULL);
	  stop_timer();
	  exec_command(S_QUIT, NULL);
	}
}

void ma_procedure_thread(window,event,arg)
Xv_Window window;
Event *event;
Notify_arg arg;
{
	int x,y,indice,x_rect,y_rect,indice_x,indice_y;

	if(event_action(event) == ACTION_MENU && event_is_down(event))
	{
			menu_show(menu_thread,window,event,NULL);
	} else if(nb_thread) {
		if(event_action(event) == ACTION_SELECT && event_is_down(event)) {
			x = event_x(event);
			y = event_y(event);
			indice_x = (x/74);
			indice_y = (y/95);
			indice = indice_x + (indice_y*8);
			if (indice > (nb_thread-1))
				indice = -1;
			if(selection_thread != indice) {
				x = indice_x;
				y = indice_y;
				if(selection_thread != -1) {
					indice_y = (selection_thread / 8); 
					if (indice_y > 0)
						indice_x = selection_thread - (indice_y*8);
					else
						indice_x = selection_thread;
					x_rect = (indice_x*64)+((indice_x)*10)+7;
					y_rect = (indice_y) * 95; 	
					XDrawRectangle(dpy_thread,xwin_thread,gc_rect_thread,x_rect,y_rect+5,72,73);
				}
				if(indice != -1) {
					x_rect = (x*64)+((x)*10)+7;
					y_rect = y*95;
					XDrawRectangle(dpy_thread,xwin_thread,gc_rect_thread,x_rect,y_rect+5,72,73);
				}
				selection_thread = indice;
				mettre_a_jour_thread();
			}
		}
	}
}


void ma_procedure_pvm(window,event,arg)
Xv_Window window;
Event *event;
Notify_arg arg;
{
	int x, x_rect, indice;

	if(event_action(event) == ACTION_MENU && event_is_down(event)) {
			menu_show(menu_pvm,window,event,NULL);
	} else if (nb_pvm) {
		if(event_action(event) == ACTION_SELECT && event_is_down(event)) {
			x = event_x(event);
			indice = (x/74);
			if (indice > (nb_pvm-1)) {
				indice = -1;
				XClearWindow(dpy_thread, xwin_thread);
			}
			if(selection_pvm != indice) {
				if(selection_pvm != -1) {
					x_rect = (selection_pvm*64)+((selection_pvm)*10)+7;
					XDrawRectangle(dpy_pvm,xwin_pvm,gc_rect_pvm,x_rect,5,72,73);
				}
				if(indice != -1) {
					x_rect = (indice*64)+((indice)*10)+7;
					XDrawRectangle(dpy_pvm,xwin_pvm,gc_rect_pvm,x_rect,5,72,73);
				}
				selection_pvm = indice;
				selection_thread = -1;
				mettre_a_jour_pvm();
				mettre_a_jour_thread();
			}
			if(selection_pvm != -1) {
				xv_set(racine, FRAME_BUSY, TRUE, NULL);
				exec_command(TH, table_pvm[selection_pvm].tid);
			} else {
				nb_thread = 0;
				maj_thread_count();
			}
		}
	}
}

void ma_procedure(window,event,arg)
Xv_Window window;
Event *event;
Notify_arg arg;
{
	int x, x_rect, indice;

	if(event_action(event) == ACTION_MENU && event_is_down(event)) {
		menu_show(menu_host,window,event,NULL);
	} else if(event_action(event) == ACTION_SELECT && event_is_down(event)) {
		x = event_x(event);
		indice = (x/74);
		if(indice > nb_host-1) {
			indice = -1;
			XClearWindow(dpy_pvm,xwin_pvm);
			XClearWindow(dpy_thread, xwin_thread);
		}
		if(selection_courante != indice) {
			if(selection_courante != -1) {
				x_rect = (selection_courante*64)+((selection_courante)*10)+7;
				XDrawRectangle(dpy_host,xwin_host,gc_rectangle,x_rect,5,70,72);
			}
			if(indice != -1) {
				x_rect = (indice*64)+((indice)*10)+7;
				XDrawRectangle(dpy_host,xwin_host,gc_rectangle,x_rect,5,70,72);
			}
			selection_courante = indice;
			selection_pvm = -1;
			selection_thread = -1;
			mettre_a_jour_host();
			mettre_a_jour_pvm();
			mettre_a_jour_thread();
		}
		if(indice != -1) {
			xv_set(racine, FRAME_BUSY, TRUE, NULL);
			exec_command(S_PS,NULL);
		} else {
			nb_pvm = 0;
			maj_task_count();
			nb_thread = 0;
			maj_thread_count();
		}
	}
}

/*------------------------------------------------------------------*/
/*    REDESSINER_THREAD : fonction s occupant de l affichage des    */
/*    threads							    */
/*------------------------------------------------------------------*/

void
redessiner_thread(canvas, fenetre, dpy, xwin, xrects)
Canvas          canvas;
Xv_Window       fenetre;
Display         *dpy;
Window          xwin;
Xv_xrectlist    *xrects;
{
 int hx,hy,larg,haut,i,x,y,x_rect,y_rect,compte;
	unsigned long blanc,noir;

	gc_thread=DefaultGC(dpy,DefaultScreen(dpy));
	
	noir = BlackPixel(dpy,DefaultScreen(dpy));
	blanc = WhitePixel(dpy,DefaultScreen(dpy));

	gcv.function = GXinvert;
	gcv.foreground = noir;
	gcv.background = blanc;
	gcmask = GCForeground | GCBackground | GCFunction;
	gc_rect_thread = XCreateGC(dpy,xwin,gcmask,&gcv);

	XReadBitmapFile(dpy,xwin,icon_thread,&larg,&haut,&img_thread,&hx,&hy);
	XReadBitmapFile(dpy,xwin,icon_boulet,&larg,&haut,&img_boulet,&hx,&hy);
	XReadBitmapFile(dpy,xwin,icon_endormi,&larg,&haut,&img_endormi,&hx,&hy);
	XReadBitmapFile(dpy,xwin,icon_blocke,&larg,&haut,&img_blocke,&hx,&hy);

	dpy_thread = dpy;
   	xwin_thread = xwin;

	for (i=0;i<min(nb_thread, MAX_VIEWABLE_THREAD);i++)
   	{
    		if (compte == 8) compte =0;
    		x=(compte*64)+((compte+1)*10);
 		y = (i/8)*95+10;

		if(table_thread[i].running[0] == 'a')
			XCopyPlane(dpy_thread,img_endormi,xwin_thread,gc,0,0,64,64,x,y,1);
		else if(table_thread[i].running[0] == 'b')
			XCopyPlane(dpy_thread,img_blocke,xwin_thread,gc,0,0,64,64,x,y,1);
		else if(table_thread[i].migratable[0] == 'y')
			XCopyPlane(dpy_thread,img_thread,xwin_thread,gc,0,0,64,64,x,y,1);
		else
			XCopyPlane(dpy_thread,img_boulet,xwin_thread,gc,0,0,64,64,x,y,1);

		XDrawString(dpy_thread, xwin_thread, gc, x, y+85,
       			 	(*thread_view_func)(i), strlen((*thread_view_func)(i)));
		compte ++;
   	}
	if(selection_thread != -1) {
		y = (selection_thread / 8);
		if (y > 0)
			x = selection_thread - (y*8);
		else
			x = selection_thread;
		x_rect = (x*64)+((x)*10)+7;
		y_rect = (y) * 95;
		XDrawRectangle(dpy_thread,xwin_thread,gc_rect_thread,x_rect,y_rect+5,72,73);
	}
}


void
redessiner_pvm(canvas, fenetre, dpy, xwin, xrects)
Canvas          canvas;
Xv_Window       fenetre;
Display         *dpy;
Window          xwin;
Xv_xrectlist    *xrects;
{
 int hx,hy,larg,haut,i,x;
 unsigned long blanc,noir;


	gc_pvm=DefaultGC(dpy,DefaultScreen(dpy));
	noir = BlackPixel(dpy,DefaultScreen(dpy));
	blanc = WhitePixel(dpy,DefaultScreen(dpy));

	gcv.function = GXinvert;
	gcv.foreground = noir;
	gcv.background = blanc;
	gcmask = GCForeground | GCBackground | GCFunction;
	gc_rect_pvm = XCreateGC(dpy,xwin,gcmask,&gcv);

	XReadBitmapFile(dpy,xwin,icon_task,&larg,&haut,&pvm_task,&hx,&hy);

	dpy_pvm = dpy;
	xwin_pvm = xwin;

	for (i=0;i<min(nb_pvm, MAX_VIEWABLE_TASK);i++)
   	{
    		x=(i*64)+((i+1)*10);
		XCopyPlane(dpy_pvm,pvm_task,xwin_pvm,gc,0,0,64,64,x,10,1);
    		XDrawString(dpy_pvm, xwin_pvm, gc, x+10, 95,
        		(*task_view_func)(i), strlen((*task_view_func)(i)));
  	}
	if(selection_pvm != -1) {
		x = (selection_pvm*64)+((selection_pvm)*10)+7;
		XDrawRectangle(dpy_pvm,xwin_pvm,gc_rect_pvm,x,5,72,73);
	}
}

void
redessiner(canvas, fenetre, dpy, xwin, xrects)
Canvas          canvas;
Xv_Window       fenetre;
Display         *dpy;
Window          xwin;
Xv_xrectlist    *xrects;
{
	 int hx,hy,larg,haut,i,x;
	 unsigned long blanc,noir;

	gc=DefaultGC(dpy,DefaultScreen(dpy));
	noir = BlackPixel(dpy,DefaultScreen(dpy));
	blanc = WhitePixel(dpy,DefaultScreen(dpy));

	gcv.function = GXinvert;
	gcv.foreground = noir;
	gcv.background = blanc;
	gcmask = GCForeground | GCBackground | GCFunction;
	gc_rectangle = XCreateGC(dpy,xwin,gcmask,&gcv);
/*
        XCopyGC(dpy, gc, ~0 , gc_rectangle);
	gcv.function = GXinvert;
	XChangeGC(dpy, gc_rectangle, GCFunction, &gcv); 
*/
XReadBitmapFile(dpy,xwin,icon_host,&larg,&haut,&host[0],&hx,&hy);
        dpy_host = dpy;
	xwin_host = xwin;


	XClearWindow(dpy_host,xwin_host);
	for (i=0;i<min(nb_host, MAX_VIEWABLE_HOST);i++)
   	{
    		x=(i*64)+((i+1)*10);
    		XCopyPlane(dpy_host,host[0],xwin_host,gc,0,0,64,64,x,10,1);
    		XDrawString(dpy_host, xwin_host, gc, x, 95,
       		(*host_view_func)(i), strlen((*host_view_func)(i)));
	}
	if(selection_courante != -1) {
		x = (selection_courante*64)+((selection_courante)*10)+7;
		XDrawRectangle(dpy_host,xwin_host,gc_rectangle,x,5,70,72);
	}
}

void tty_input()	/* on envoie la commande tapee vers la console */
{ char buf[512];
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
	verbose_mode = !value;
}

void mode_refresh(Panel_item item, int value)
{
	if(value) { /* Automatic refresh */
		automatic_refresh = TRUE;
		timerclear(&refresh_timer.it_value);
		refresh_timer.it_value.tv_sec = REFRESH_INTERVAL;
		refresh_timer.it_interval = refresh_timer.it_value;
		notify_set_itimer_func(racine, (Notify_func)refresh,
			ITIMER_REAL, &refresh_timer,
			NULL);
	} else {
		automatic_refresh = FALSE;
		notify_set_itimer_func(racine, NOTIFY_FUNC_NULL,
			ITIMER_REAL,	NULL,
			NULL);
	}
}

int execute_user_cmd(char *s)
{ char cmd[128], macro[32], source[128], tmp;
  char *src = source, *dest = cmd;
  int x, y;

	strcpy(source, s);
	while(*src != '\0') {
		if(*src != '$') {
			*dest++ = *src++;
		} else if(*(src+1) == '$') {
			src++;
			*dest++ = *src++;
		} else {
			char *deb = ++src;

			while(*src != '\0' && *src != ' ' && *src != '\t')
				src++;
			tmp = *src;
			*src = '\0';
			if(!strcmp(deb, "thread")) {
				if(selection_thread == -1) {
					x = (int)xv_get(user_button, XV_X);
					y = (int)xv_get(user_button, XV_Y);
					(void)notice_prompt(racine, NULL,
						NOTICE_FOCUS_XY,	x+40,	y+5,
						NOTICE_MESSAGE_STRINGS,	"No thread selected",	NULL,
						NOTICE_BUTTON,		"Ok",			0,	
						NULL);
					return FALSE;
				}
				strcpy(dest, table_thread[selection_thread].thread_id);
				dest += strlen(table_thread[selection_thread].thread_id);
			} else if(!strcmp(deb, "task")) {
				x = (int)xv_get(user_button, XV_X);
				y = (int)xv_get(user_button, XV_Y);
				if(selection_pvm == -1) {
					(void)notice_prompt(racine, NULL,
						NOTICE_FOCUS_XY,	x+40,	y+5,
						NOTICE_MESSAGE_STRINGS,	"No task selected",	NULL,
						NOTICE_BUTTON,		"Ok",			0,	
						NULL);
					return FALSE;
				}
				strcpy(dest, table_pvm[selection_pvm].tid);
				dest += strlen(table_pvm[selection_pvm].tid);
			} else if(!strcmp(deb, "host")) {
				x = (int)xv_get(user_button, XV_X);
				y = (int)xv_get(user_button, XV_Y);
				if(selection_courante == -1) {
					(void)notice_prompt(racine, NULL,
						NOTICE_FOCUS_XY,	x+40,	y+5,
						NOTICE_MESSAGE_STRINGS,	"No host selected",	NULL,
						NOTICE_BUTTON,		"Ok",			0,	
						NULL);
					return FALSE;
				}
				strcpy(dest, table_host[selection_courante].dtid);
				dest += strlen(table_host[selection_courante].dtid);
			} else {
				x = (int)xv_get(user_button, XV_X);
				y = (int)xv_get(user_button, XV_Y);
				(void)notice_prompt(racine, NULL,
					NOTICE_FOCUS_XY,	x+40,	y+5,
					NOTICE_MESSAGE_STRINGS,	"unknown macro",	NULL,
					NOTICE_BUTTON,		"Ok",			0,	
					NULL);
				return FALSE;
			}
			*src = tmp;
		}
	}
	*dest = *src;	/* pour le '\0' */

	xv_set(racine, FRAME_BUSY, TRUE, NULL);
	exec_command(USER, cmd);
	return TRUE;
}

void user_menu_proc(menu, item)
Menu menu;
Menu_item item;
{
	execute_user_cmd((char *)xv_get(item, MENU_STRING));
}

int user_button_proc(item, event)
Panel_item item;
Event *event;
{
	return XV_OK;
}

Panel_setting user_text_proc(item, event)
Panel_item item;
Event *event;
{ char *text, *text_copy;
  int n;

	text = (char *)xv_get(item, PANEL_VALUE);
	text_copy = (char *)malloc(strlen(text)+1);
	strcpy(text_copy, text);

	if(strcmp(text, "")) {
		if(execute_user_cmd(text)) {
			xv_set(user_text, PANEL_VALUE, "", NULL);
			xv_set(user_menu, MENU_INSERT, 0, xv_create((Panel)NULL, MENUITEM,
							MENU_STRING, text_copy,
							MENU_RELEASE,
							NULL),
				NULL);

			n = (int)xv_get(user_menu, MENU_NITEMS);
			if(n > MAX_USER_COMMANDS) {
				xv_set(user_menu, MENU_REMOVE, n, NULL);
			}
		}
	}

	return PANEL_NONE;
}

int
main(argc,argv)
int     argc;
char    *argv[];
{
 
        int pid,i;
	
	if(getenv("PM2_ROOT") == NULL) {
		fprintf(stderr, "Fatal error : PM2_ROOT environment variable not set.\n");
		exit(1);
	}

	etat=INITIAL_STATE;
        pipe(pipe_io[0]);
	pipe(pipe_io[1]);
	switch(pid = fork())
	{
	case -1 :
		close(pipe_io[0][0]);
		close(pipe_io[0][1]);
		close(pipe_io[1][0]);
		close(pipe_io[1][1]);
		perror("fork failed");
		exit(1);
	case 0 :
		dup2(pipe_io[0][0],0);
		dup2(pipe_io[1][1],1);
		dup2(pipe_io[1][1],2);
		for(i = 0;i <NSIG;i++)
			(void)signal(i,SIG_DFL);
		execvp(*my_argv,my_argv);
		if (errno == ENOENT)
			printf("s : command not found\n",*argv);
		else
			perror(*argv);
		perror("execvp");
		_exit(1);
	default :
		close(pipe_io[0][0]);
		close(pipe_io[1][1]);
	}
 

	strcpy(icon_host, getenv("PM2_ROOT"));
	strcpy(icon_task, icon_host);
	strcpy(icon_thread, icon_host);
	strcpy(icon_boulet, icon_host);
	strcpy(icon_endormi, icon_host);
	strcpy(icon_blocke, icon_host);
	strcpy(icon_panier, icon_host);
	strcpy(icon_panier_ouvert, icon_host);

	strcat(icon_host, "/console/icons/hutte.xbm");
	strcat(icon_task, "/console/icons/task.xbm");
	strcat(icon_thread, "/console/icons/thread.xbm");
	strcat(icon_boulet, "/console/icons/boulet.xbm");
	strcat(icon_endormi, "/console/icons/sleeping.xbm");
	strcat(icon_blocke, "/console/icons/blocked.xbm");
	strcat(icon_panier, "/console/icons/panier.xbm");
	strcat(icon_panier_ouvert, "/console/icons/panier_ouvert.xbm");

	selection_courante = -1;
	selection_pvm = -1;
	selection_thread = -1;

	buf = (char *)malloc(MAX_BUF+1);

	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);

	window1_image = xv_create(XV_NULL, SERVER_IMAGE,
		SERVER_IMAGE_DEPTH,	1,
		SERVER_IMAGE_BITS,	window1_bits,
		XV_WIDTH,		64,
		XV_HEIGHT,		64,
		NULL);

	racine = (Frame)xv_create(XV_NULL, FRAME,
        	FRAME_LABEL,			"Xpm2 v1.0",
		XV_HEIGHT,			680,
		XV_WIDTH,			600,
		FRAME_SHOW_RESIZE_CORNER,	FALSE,
		FRAME_ICON,			xv_create(XV_NULL, ICON,
							ICON_IMAGE,		window1_image,
							ICON_TRANSPARENT,	TRUE,
							NULL),
        	NULL);

	notify_set_input_func(racine,read_it,pipe_io[1][0]);

	controle = (Panel)xv_create(racine, PANEL,
		PANEL_LAYOUT,	PANEL_HORIZONTAL,
		XV_HEIGHT,	80,
		XV_WIDTH,	600,
		NULL);

	/* Definition du menu */

	menu_host = (Menu)xv_create((Frame)NULL,MENU,
		MENU_ACTION_ITEM,	"Info", 	info_host,
		MENU_ACTION_ITEM,	"Add",		add_host,
		MENU_ACTION_ITEM,	"Delete",	delete_host,
		MENU_PULLRIGHT_ITEM,	"View",			xv_create((Frame)NULL, MENU,
			MENU_ACTION_ITEM,	"by NAME",		by_hname_proc,
			MENU_ACTION_ITEM,	"by HOST_ID",		by_dtid_proc,
			MENU_ACTION_ITEM,	"by ARCH.",		by_arch_proc,
			NULL),
		NULL);

	menu_pvm = (Menu)xv_create((Frame)NULL,MENU,
		MENU_ACTION_ITEM,	"Info",			info_pvm,
		MENU_ACTION_ITEM,	"Delete",		delete_pvm,
		MENU_PULLRIGHT_ITEM,	"View",			xv_create((Frame)NULL, MENU,
			MENU_ACTION_ITEM,	"by TASK_ID",		by_tid_proc,
			MENU_ACTION_ITEM,	"by NAME",		by_name_proc,
			NULL),
		NULL);

	menu_thread = (Menu)xv_create((Frame)NULL,MENU,
		MENU_ACTION_ITEM,	"Info",			info_thread,
		MENU_PULLRIGHT_ITEM,	"Basket",		xv_create((Frame)NULL, MENU,
			MENU_ACTION_ITEM,	"Show",		voir_panier,
			MENU_ACTION_ITEM,	"Put into",	aller_dans_panier,
			MENU_ACTION_ITEM,	"Take from",	prendre_du_panier,
			NULL),
		MENU_PULLRIGHT_ITEM,	"View",			xv_create((Frame)NULL, MENU,
			MENU_ACTION_ITEM,	"by PTHREAD_ID",	by_pid_proc,
			MENU_ACTION_ITEM,	"by SERVICE",		by_service_proc,
			MENU_ACTION_ITEM,	"by PRIORITY",		by_prio_proc,
			NULL),
		NULL);

	(void)xv_create(controle, PANEL_BUTTON,
        	PANEL_LABEL_STRING,	"Hosts",
        	PANEL_ITEM_MENU,	menu_host,
        	NULL);

	(void)xv_create(controle, PANEL_BUTTON,
		PANEL_LABEL_STRING,	"Tasks",
		PANEL_ITEM_MENU,	menu_pvm,
		NULL);

	(void)xv_create(controle, PANEL_BUTTON,
		PANEL_LABEL_STRING,	"Threads",
		PANEL_ITEM_MENU,	menu_thread,
		NULL);

	(void)xv_create(controle, PANEL_BUTTON,
		PANEL_LABEL_STRING,	"Refresh",
		PANEL_NOTIFY_PROC,	force_refresh,
		NULL);

	user_menu = (Menu)xv_create((Panel)NULL, MENU,
		MENU_NOTIFY_PROC,	user_menu_proc,
		MENU_STRINGS,		"user -t $task",
					"ping $task",
					"user -t $task -enable_mig $thread",
					"user -t $task -disable_mig $thread",
					"user -t $task -save /tmp/toto",
					"user -t $task -load /tmp/toto",
					NULL,
		NULL);

	user_button = (Panel_item)xv_create(controle, PANEL_BUTTON,
		PANEL_LABEL_STRING,	"User cmd",
		PANEL_NOTIFY_PROC,	user_button_proc,
		PANEL_ITEM_MENU,	user_menu,
		NULL);

	user_text = (Panel_item)xv_create(controle, PANEL_TEXT,
		PANEL_LABEL_STRING,		"",
		PANEL_VALUE_DISPLAY_LENGTH,	16,
		PANEL_VALUE_STORED_LENGTH,	128,
		PANEL_VALUE,			"",
		PANEL_NOTIFY_LEVEL,		PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING,		"\n\r",
		PANEL_NOTIFY_PROC,		user_text_proc,
		NULL);

	(void)xv_create(controle, PANEL_CHOICE,
		PANEL_LABEL_STRING,	"Mode",
		PANEL_CHOICE_STRINGS,	"Verbose", "Silent", NULL,
		PANEL_NOTIFY_PROC,	mode_console,
		PANEL_VALUE,		1,
		NULL);

	refresh_item = (Panel_item)xv_create(controle, PANEL_CHECK_BOX,
		PANEL_LABEL_STRING,	"   ",
		PANEL_CHOICE_STRINGS,	"Automatic refresh", NULL,
		PANEL_NOTIFY_PROC,	mode_refresh,
		PANEL_VALUE,		0,
		NULL);

	(void)xv_create(controle, PANEL_BUTTON,
		PANEL_LABEL_STRING,	"About ...",
		PANEL_NOTIFY_PROC,	au_sujet_de,
		NULL);

	(void)xv_create(controle, PANEL_BUTTON,
		PANEL_LABEL_STRING,	"Quit",
		PANEL_NOTIFY_PROC,	quitter,
		NULL);

	controle1 = (Panel)xv_create(racine, PANEL,
        	PANEL_LAYOUT,	PANEL_HORIZONTAL,
		XV_X,		0,
		XV_Y,		80,
		XV_HEIGHT,	20,
		XV_WIDTH,	305,
        	NULL);

	host_count_item = (Panel_item)xv_create(controle1, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"0 host(s)",
		NULL);

	/* bouche trou */
	(void)xv_create(racine, PANEL,
		XV_X,		295,
		XV_Y,		100,
		XV_WIDTH,	10,
		XV_HEIGHT,	120,
		NULL);

	/*-----------------------*/
	/* Definition des canvas */
	/*-----------------------*/

	canvas=(Canvas)xv_create(racine,CANVAS,
		CANVAS_AUTO_EXPAND,	TRUE,
		CANVAS_AUTO_SHRINK,	FALSE,
		CANVAS_X_PAINT_WINDOW,	TRUE,
		CANVAS_WIDTH,		MAX_VIEWABLE_HOST*74,
		CANVAS_HEIGHT,		120,
		CANVAS_REPAINT_PROC,	redessiner,
		XV_X,			0,
		XV_Y,			100,
		XV_WIDTH,		295,
		XV_HEIGHT,		120,
		NULL);

	xv_set(canvas_paint_window(canvas),
		WIN_EVENT_PROC,		ma_procedure,
		WIN_CONSUME_EVENTS,	WIN_MOUSE_BUTTONS, NULL,
		NULL);

	controle2 = (Panel)xv_create(racine, PANEL,
        	PANEL_LAYOUT,	PANEL_HORIZONTAL,
		XV_X,		305,
		XV_Y,		80,
		XV_HEIGHT,	20,
 		XV_WIDTH,	295,
       		NULL);

	task_count_item = (Panel_item)xv_create(controle2, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"0 task(s)",
		NULL);

	canvas_pvm=(Canvas)xv_create(racine, CANVAS,
		CANVAS_AUTO_EXPAND,	TRUE,
		CANVAS_AUTO_SHRINK,	FALSE,
		CANVAS_X_PAINT_WINDOW,	TRUE,
		CANVAS_WIDTH,		MAX_VIEWABLE_TASK*74,
		CANVAS_HEIGHT,		120,
		CANVAS_REPAINT_PROC,	redessiner_pvm,
		XV_X,			305,
		XV_Y,			100,
		XV_WIDTH,		295,
		XV_HEIGHT,		120,
		NULL);

	xv_set(canvas_paint_window(canvas_pvm),
		WIN_EVENT_PROC,		ma_procedure_pvm,
		WIN_CONSUME_EVENTS,	WIN_MOUSE_BUTTONS, NULL,
		NULL);


	controle3 = (Panel)xv_create(racine, PANEL,
        	PANEL_LAYOUT,	PANEL_HORIZONTAL,
		XV_X,		0,
		XV_Y,		220,
		XV_HEIGHT,	20,
		XV_WIDTH,	600,
        	NULL);

	thread_count_item = (Panel_item)xv_create(controle3, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"0 thread(s)",
		NULL);

	canvas_thread=(Canvas)xv_create(racine, CANVAS,
		CANVAS_AUTO_EXPAND,	TRUE,
		CANVAS_AUTO_SHRINK,	FALSE,
		CANVAS_X_PAINT_WINDOW,	TRUE,
		CANVAS_WIDTH,		600,
		CANVAS_HEIGHT,		(MAX_VIEWABLE_THREAD>>3)*95,
		CANVAS_REPAINT_PROC,	redessiner_thread,
		XV_X,			0,
		XV_Y,			240,
		XV_WIDTH,		600,
		XV_HEIGHT,		200,
		NULL);

	xv_set(canvas_paint_window(canvas_thread),
		WIN_EVENT_PROC,		ma_procedure_thread,
		WIN_CONSUME_EVENTS,	WIN_MOUSE_BUTTONS, NULL,
		NULL);

    	(void)xv_create(canvas, SCROLLBAR,
        	SCROLLBAR_SPLITTABLE,	TRUE,
        	SCROLLBAR_DIRECTION,	SCROLLBAR_HORIZONTAL,
        	NULL);

	(void)xv_create(canvas_pvm, SCROLLBAR,
        	SCROLLBAR_SPLITTABLE,	TRUE,
        	SCROLLBAR_DIRECTION,	SCROLLBAR_HORIZONTAL,
        	NULL);

	(void)xv_create(canvas_thread, SCROLLBAR,
        	SCROLLBAR_SPLITTABLE,	TRUE,
        	SCROLLBAR_DIRECTION,	SCROLLBAR_VERTICAL,
        	NULL);

	controle4 = (Panel)xv_create(racine, PANEL,
        	PANEL_LAYOUT,	PANEL_HORIZONTAL,
		XV_X,		0,
		XV_Y,		440,
		XV_HEIGHT,	20,
		XV_WIDTH,	600,
        	NULL);

	(void)xv_create(controle4, PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"Text console : ",
		NULL);

	ttysw = (Tty)xv_create(racine, TERMSW,
		XV_X,		0,
		XV_Y,		460,
		XV_HEIGHT,	240,
		XV_WIDTH,	600,
		TTY_ARGV,	TTY_ARGV_DO_NOT_FORK,
		NULL);

	tty_fd = (int)xv_get(ttysw, TTY_TTY_FD);

	notify_set_input_func(racine, (Notify_func)tty_input, tty_fd);

	notify_set_wait3_func(racine, (Notify_func)la_console_est_morte_paix_a_son_ame,
			pid);

    	xv_main_loop(racine);
}

