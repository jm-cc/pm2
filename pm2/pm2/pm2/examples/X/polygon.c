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

#include "pm2.h"
#include "custom.h"
#include "regul.h"

#include <unistd.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/cms.h>


short black_bits[] = {
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
};


#define X_SIZE	460
#define Y_SIZE	320

unsigned module_suivant, module_precedent;

typedef struct {
  int rouge, vert, bleu;
} couleur_t;

#define NB_COULEURS	11

#define c_rouge 0
#define c_noir  10

#define TABLEAU_COULEURS \
{ \
	{ 255, 0, 0 }, /* rouge */ \
	{ 255, 118, 0 }, /* orange */ \
	{ 255, 235, 0 }, /* jaune */ \
	{ 255, 162, 150 }, /* saumon */ \
	{ 0, 255, 0 }, /* vert */ \
	{ 0, 200, 255 }, /* bleu clair */ \
	{ 255, 0, 255 }, /* mauve */ \
	{ 159, 24, 255 }, /* autre mauve */ \
	{ 0, 0, 255 }, /* bleu */ \
	{ 255, 255, 255 }, /* blanc */ \
	{ 0, 0, 0 } /* noir */ \
}

char *color_names[] = {
	"rouge",
	"orange",
	"jaune",
	"saumon",
	"vert",
	"bleu clair",
	"rose",
	"mauve",
	"bleu",
	"blanc",
	"noir"
};

Xv_singlecolor colors[] = TABLEAU_COULEURS;

couleur_t les_couleurs[NB_COULEURS] = TABLEAU_COULEURS;
int finished = FALSE;

marcel_mutex_t X_mutex;

marcel_key_t color_key;

Display *dpy;
int ecran;
int Xfdes;
Window win;
XGCValues xgc_noir, xgc_blanc;
unsigned long gcmask;
GC gc_noir[NB_COULEURS], gc_blanc, gc_black;
Frame base_frame;
Panel_item migr_button, prio_slider, color_choices, regul_item;
Canvas canvas;
Server_image chip;
Cms cms;

int current_color = 0;

static unsigned SYNC_DISPLAY;

static __inline__ void X_lock(void)
{
  marcel_mutex_lock(&X_mutex);
}

static __inline__ void X_unlock(void)
{
  marcel_mutex_unlock(&X_mutex);
}

typedef struct maillon {
  struct {
    unsigned x, y;
  } point;
  struct maillon *suivant;
} *liste;

liste generer_liste()
{
  int nb;
  liste head = NULL, temp;

  nb = rand() % 2 + 3;

  while(nb--) {
    temp = (liste)pm2_isomalloc(sizeof(struct maillon));
    temp->point.x = rand() % (X_SIZE-20) + 10;
    temp->point.y = rand() % (Y_SIZE-20) + 10;
    temp->suivant = head;
    head = temp;
  }
  return head;
}

void afficher_polygone(liste l)
{
  while(l) {
    fprintf(stderr, "Point(%d,%d) ", l->point.x, l->point.y);
    l = l->suivant;
  }
  fprintf(stderr, "\n");
}

void tracer_polygone(void *arg)
{
  int couleur = current_color;
  liste l, premier, suivant;
  
  premier = l = generer_liste();
  suivant = premier->suivant;

  marcel_setspecific(color_key, color_names[couleur]);
  pm2_enable_migration();

  while(1) {

    X_lock();
    XDrawLine(dpy, win, gc_noir[couleur],
	      l->point.x, l->point.y,
	      suivant->point.x, suivant->point.y);
    XFlush(dpy);
    X_unlock();

    marcel_delay(1);

    l = suivant;
    suivant = suivant->suivant;
    if(!suivant)
      suivant = premier;
  }
}

static void sync_display(unsigned regul)
{
  if(!regul) /* Regulation On */
    regul_start();
  else
    regul_stop();

  X_lock();
  xv_set(regul_item, PANEL_VALUE, regul, NULL);
  XFlush(dpy);
  X_unlock();
}

static void sync_display_service(void)
{
  unsigned regul;

  old_mad_unpack_int(MAD_IN_HEADER, &regul, 1);
  pm2_rawrpc_waitdata();

  sync_display(regul);
}

static void positionner_regul(any_t arg)
{
  int p;

  for(p=0; p<pm2_config_size(); p++) {
    if(pm2_self() == p)
      sync_display((unsigned)arg);
    else {
      pm2_rawrpc_begin(p, SYNC_DISPLAY, NULL);
      old_mad_pack_int(MAD_IN_HEADER, (unsigned *)&arg, 1);
      pm2_rawrpc_end();
    }
  }
}

void redessiner(void)
{
  XFillRectangle(dpy, win, gc_black, 0, 0, X_SIZE, Y_SIZE);
  XFlush(dpy);
}

static void migrer(any_t arg)
{
  marcel_t pids[128];
  int nb;

  do {
    pm2_freeze();
    pm2_threads_list(128, pids, &nb, MIGRATABLE_ONLY);
    pm2_migrate_group(pids, min(nb, 128), module_suivant);
  } while(nb > 128);

  X_lock(); redessiner(); X_unlock();
}

static void reset_threads(void)
{
  marcel_t pids[32];
  int i, nb;

#ifndef SMP
  do {
    pm2_threads_list(32, pids, &nb, MIGRATABLE_ONLY);
    for(i=0; i<min(nb, 32); i++)
      marcel_cancel(pids[i]);
  } while(nb > 32);
#endif

  redessiner();
}

static void pre_migr(marcel_t pid)
{
  X_lock(); redessiner(); X_unlock();
}

static void draw(void)
{
  pm2_thread_create(tracer_polygone, NULL);
}

static void migr(void)
{
  pm2_thread_create(migrer, NULL);
}

static void toggle_regulate(Panel_item item, int value)
{
  pm2_thread_create(positionner_regul, (void *)value);
}

static void color_notify(Panel_item p, int choice)
{
  current_color = choice;
}

static void quit(void)
{
  finished = TRUE;
}

static void ma_main_loop(void)
{
  for(;;) {

    X_lock();
    notify_dispatch();
    X_unlock();

    if(finished)
      break;

    marcel_read(Xfdes, NULL, 0);
  }
}

static void startup_func()
{
  module_suivant = (pm2_self() + 1) % pm2_config_size();
  module_precedent = (pm2_self() == 0) ? pm2_config_size() - 1 : pm2_self() - 1;

  regul_init();
}

int pm2_main(int argc, char **argv)
{ 
  Panel base_panel;
  Panel_item reset_button, end_button, draw_button;
  char hostname[64];
  char baniere[64];
  int argc_save;
  char *argv_save[64];

  marcel_mutex_init(&X_mutex, NULL);

  srand(time(NULL));

  argc_save=0;
  while(argv[argc_save] != NULL) {
    argv_save[argc_save] = argv[argc_save];
    argc_save++;
  }
  argv_save[argc_save] = NULL;

  gethostname(hostname, 64);

  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);

  cms = (Cms)xv_create((Frame)NULL, CMS,
		       CMS_CONTROL_CMS,	TRUE,
		       CMS_SIZE,		CMS_CONTROL_COLORS + NB_COULEURS,
		       CMS_COLORS,		colors,
		       NULL);

  chip = (Server_image)xv_create((Frame)NULL, SERVER_IMAGE,
				 XV_WIDTH,		16,
				 XV_HEIGHT,		16,
				 SERVER_IMAGE_DEPTH,	1,
				 SERVER_IMAGE_BITS,	black_bits,
				 NULL);

  base_frame = (Frame)xv_create((Frame)NULL, FRAME,
				NULL);

  base_panel = (Panel)xv_create(base_frame, PANEL,
				XV_WIDTH, 	X_SIZE,
				XV_HEIGHT,	150,
				WIN_CMS,	cms,
				NULL);

  regul_item = (Panel_item)xv_create(base_panel, PANEL_CHOICE,
				     PANEL_LABEL_STRING,	"Regulation",
				     PANEL_CHOICE_STRINGS,	"On", "Off", NULL,
				     PANEL_NOTIFY_PROC,	toggle_regulate,
				     PANEL_VALUE,		1,
				     XV_X,			X_SIZE - 150,
				     PANEL_ITEM_COLOR,	CMS_CONTROL_COLORS + c_noir,
				     NULL);

  draw_button = (Panel_item)xv_create(base_panel, PANEL_BUTTON,
				      PANEL_NEXT_ROW,		-1,
				      PANEL_LABEL_STRING,	"Run",
				      PANEL_NOTIFY_PROC,	draw,
				      PANEL_ITEM_COLOR,	CMS_CONTROL_COLORS + c_noir,
				      NULL);

  color_choices = (Panel_item)xv_create(base_panel, PANEL_CHOICE,
					PANEL_LAYOUT,		PANEL_HORIZONTAL,
					PANEL_LABEL_STRING,	"Color",
					PANEL_CHOICE_IMAGES,	chip, chip, chip, chip, chip,
					chip, chip, chip, chip, chip, NULL,
					PANEL_CHOICE_COLOR,	0, CMS_CONTROL_COLORS + 0,
					PANEL_CHOICE_COLOR,	1, CMS_CONTROL_COLORS + 1,
					PANEL_CHOICE_COLOR,	2, CMS_CONTROL_COLORS + 2,
					PANEL_CHOICE_COLOR,	3, CMS_CONTROL_COLORS + 3,
					PANEL_CHOICE_COLOR,	4, CMS_CONTROL_COLORS + 4,
					PANEL_CHOICE_COLOR,	5, CMS_CONTROL_COLORS + 5,
					PANEL_CHOICE_COLOR,	6, CMS_CONTROL_COLORS + 6,
					PANEL_CHOICE_COLOR,	7, CMS_CONTROL_COLORS + 7,
					PANEL_CHOICE_COLOR,	8, CMS_CONTROL_COLORS + 8,
					PANEL_CHOICE_COLOR,	9, CMS_CONTROL_COLORS + 9,
					PANEL_NOTIFY_PROC,	color_notify,
					NULL);

  migr_button = (Panel_item)xv_create(base_panel, PANEL_BUTTON,
				      PANEL_NEXT_ROW,		-1,
				      PANEL_LABEL_STRING,	"Migrate",
				      PANEL_NOTIFY_PROC,	migr,
				      PANEL_ITEM_COLOR,	CMS_CONTROL_COLORS + c_noir,
				      NULL);

  reset_button = (Panel_item)xv_create(base_panel, PANEL_BUTTON,
				       PANEL_NEXT_ROW,		-1,
				       PANEL_LABEL_STRING,	"Reset",
				       PANEL_NOTIFY_PROC,	reset_threads,
				       PANEL_ITEM_COLOR,	CMS_CONTROL_COLORS + c_noir,
				       NULL);

  end_button = (Panel_item)xv_create(base_panel, PANEL_BUTTON,
				     PANEL_LABEL_STRING,	"Quit",
				     PANEL_NOTIFY_PROC,	quit,
				     PANEL_ITEM_COLOR,	CMS_CONTROL_COLORS + c_rouge,
				     NULL);

  canvas = xv_create(base_frame, CANVAS,
		     CANVAS_X_PAINT_WINDOW, TRUE,
		     CANVAS_REPAINT_PROC, redessiner,
		     CANVAS_AUTO_EXPAND, FALSE,
		     CANVAS_AUTO_SHRINK, FALSE,
		     CANVAS_WIDTH, X_SIZE,
		     CANVAS_HEIGHT, Y_SIZE,
		     XV_WIDTH, X_SIZE,
		     XV_HEIGHT, Y_SIZE,
		     XV_X, 0,
		     XV_Y, 150,
		     NULL);

  window_fit(base_frame);

  dpy = (Display *)xv_get(base_frame, XV_DISPLAY);
  ecran = DefaultScreen(dpy);
  Xfdes = ConnectionNumber(dpy);
  win = (Window)xv_get(canvas_paint_window(canvas), XV_XID);
  xgc_noir.foreground = BlackPixel(dpy, DefaultScreen(dpy));
  xgc_noir.background = BlackPixel(dpy, DefaultScreen(dpy));
  gcmask = GCForeground | GCBackground | GCLineWidth | GCFunction;
  xgc_noir.line_width = 1;
  xgc_noir.function = GXcopy;
  gc_black = XCreateGC(dpy, win, gcmask, &xgc_noir);

  xgc_noir.background = WhitePixel(dpy, DefaultScreen(dpy));
  xgc_blanc = xgc_noir;
  xgc_blanc.foreground = WhitePixel(dpy, DefaultScreen(dpy));
  gc_blanc = XCreateGC(dpy, win, gcmask, &xgc_blanc);

  {
    XColor couleur;
    Colormap cmap;
    int i;

    cmap = XDefaultColormap(dpy, ecran);
    xgc_noir.function = GXxor;
    for(i=0; i<NB_COULEURS; i++) {
      gc_noir[i] = XCreateGC(dpy, win, gcmask, &xgc_noir);
      couleur.red = (les_couleurs[i].rouge)*256;
      couleur.green = (les_couleurs[i].vert)*256;
      couleur.blue = (les_couleurs[i].bleu)*256;
      XAllocColor(dpy, cmap, &couleur);
      XSetForeground(dpy, gc_noir[i], couleur.pixel);
    }
  }
  xv_set(base_frame, XV_SHOW, TRUE, NULL);

  pm2_rpc_init();
  regul_rpc_init();

  pm2_set_startup_func(startup_func);
  pm2_set_user_func(user_func);
  pm2_set_pre_migration_func(pre_migr);

  pm2_rawrpc_register(&SYNC_DISPLAY, sync_display_service);

  pm2_init(&argc_save, argv_save);

  marcel_key_create(&color_key, NULL);

  sprintf(baniere, "Task [%x]  on  %s", pm2_self(), hostname);
  xv_set(base_frame, FRAME_LABEL, baniere, NULL);

  ma_main_loop();

  reset_threads();

  pm2_halt();

  regul_exit();
  pm2_exit();

  return 0;
}
