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

#include "rpc_defs.h"
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

int *les_modules, nb_modules;

int mon_rang, module_suivant, module_precedent;

typedef struct {
	int rouge, vert, bleu;
} couleur_t;

#define NB_COULEURS	10

#define c_rouge 0

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
	"noir"
};

Xv_singlecolor colors[] = TABLEAU_COULEURS;

couleur_t les_couleurs[NB_COULEURS] = TABLEAU_COULEURS;
int finished = FALSE;

marcel_key_t color_key;

Display *dpy;
int ecran;
int Xfdes;
Window win;
XGCValues xgc_noir, xgc_blanc;
unsigned long gcmask;
GC gc_noir[NB_COULEURS], gc_blanc;
Frame base_frame;
Panel_item migr_button, prio_slider, color_choices, regul_item;
Canvas canvas;
Server_image chip;
Cms cms;

int current_prio = 1, current_color = 0;

#define abs(a)	((a) < 0 ? -(a) : a)


void x_flush_if(int size)
{ static unsigned nb_draw = 0;

   if(++nb_draw >= size) {
      XFlush(dpy);
      nb_draw = 0;
   }
}

void fractale(int x1, int y1, int x2, int y2, int c)
{
  int xa, xb, xc, xd, ya, yb, yc, yd;

   if(abs(x1-x2) <= 2 && abs(y1-y2) <= 2) {
      lock_task();
      XDrawLine(dpy, win, gc_noir[c], x1, y1, x2, y2); x_flush_if(5);
      unlock_task();
      { int i = 10; while(i--) marcel_yield(); }
   } else {
      xa = (2*x1+x2)/3;			ya = (2*y1+y2)/3;
      xb = (x1+x2)/2+(y2-y1)/3;		yb = (y1+y2)/2 - (x2-x1)/3;
      xc = (x1+2*x2)/3;			yc = (y1+2*y2)/3;
      xd = (x1+x2)/2-(y2-y1)/3;		yd = (y1+y2)/2 + (x2-x1)/3;

      fractale(x1, y1, xa, ya, c);
      fractale(xa, ya, xb, yb, c);
      fractale(xb, yb, xc, yc, c);
      fractale(xc, yc, x2, y2, c);
      fractale(xa, ya, xd, yd, c);
      fractale(xd, yd, xc, yc, c);
      fractale(xa, ya, xc, yc, c);
   }
}

BEGIN_SERVICE(SYNC_DISPLAY)

  if(!req.regul) /* Regulation On */
    regul_start();
  else
    regul_stop();

  lock_task();
  xv_set(regul_item, PANEL_VALUE, req.regul, NULL);
  XFlush(dpy);
  unlock_task();

END_SERVICE(SYNC_DISPLAY)

any_t positionner_regul(any_t arg)
{
  LRPC_REQ(SYNC_DISPLAY) req;

  marcel_detach(marcel_self());
  req.regul = (int)arg;
  MULTI_QUICK_ASYNC_LRPC(les_modules, nb_modules, SYNC_DISPLAY, &req);

  return NULL;
}

void toggle_regulate(Panel_item item, int value)
{
  marcel_create(NULL, NULL, positionner_regul, (void *)value);
}

BEGIN_SERVICE(GRAPH)
   int my_color = current_color;

   marcel_setspecific(color_key, color_names[my_color]);

   marcel_enablemigration_np(marcel_self());

   while(1)
     fractale(10, Y_SIZE/2, X_SIZE-10, Y_SIZE/2, my_color);

END_SERVICE(GRAPH)

any_t graphique(any_t arg)
{
   marcel_detach(marcel_self());
   ASYNC_LRPC(pm2_self(), GRAPH, current_prio, DEFAULT_STACK, NULL);
   return NULL;
}

void lignes(void)
{
   marcel_create(NULL, NULL, graphique, NULL);
}

void quit(void)
{
   finished = TRUE;
}

void redessiner(void)
{
  lock_task();
  XFillRectangle(dpy, win, gc_blanc, 0, 0, X_SIZE, Y_SIZE);
  XFlush(dpy);
  unlock_task();
}

any_t migrer(any_t arg)
{ marcel_t pids[32];
  int nb;

   marcel_detach(marcel_self());

   do {
      pm2_freeze();
      pm2_threads_list(32, pids, &nb, MIGRATABLE_ONLY);
      pm2_migrate_group(pids, min(nb, 32), module_suivant);
   } while(nb > 32);

/*   tfprintf(stderr, "pile inutilisée = %ld octets.\n", marcel_unusedstack_np()); */
   return NULL;
}

void migr(void)
{
  marcel_attr_t attr;

  marcel_attr_init(&attr);
  marcel_attr_setprio(&attr, 10);
  marcel_attr_setstacksize(&attr, 20000);
  marcel_create(NULL, &attr, migrer, NULL);
}

void reset_threads(void)
{ marcel_t pids[32];
  int i, nb;

   do {
      pm2_threads_list(32, pids, &nb, MIGRATABLE_ONLY);
      for(i=0; i<min(nb, 32); i++)
	marcel_cancel(pids[i]);
   } while(nb > 32);

   lock_task(); redessiner(); unlock_task();
}

void prio_proc(Panel_item p, int value)
{
   current_prio = value;
}

void color_notify(Panel_item p, int choice)
{
   current_color = choice;
}

void ma_main_loop(void)
{ fd_set rfds;
  int n;

   LOOP(boucle)

      lock_task();
      notify_dispatch();
      unlock_task();

      if(finished)
         EXIT_LOOP(boucle);

      FD_ZERO(&rfds);
      FD_SET(Xfdes, &rfds);
      n = tselect(Xfdes+1, &rfds, (fd_set *)NULL, (fd_set *)NULL);
      if((n <= 0) || !FD_ISSET(Xfdes, &rfds))
         RAISE(PROGRAM_ERROR);

   END_LOOP(boucle)
}

void startup_func(int *modules, int nb)
{
   mon_rang = 0;
   while(modules[mon_rang] != pm2_self())
      mon_rang++;
   module_suivant = modules[(mon_rang+1)%nb];
   if(mon_rang == 0)
      module_precedent = modules[nb-1];
   else
      module_precedent = modules[mon_rang-1];

   regul_init(modules, nb);
}

extern stop_time_slice();

int pm2_main(int argc, char **argv)
{ Panel base_panel;
  Panel_item reset_button, end_button, draw_button;
  char hostname[64];
  char baniere[64];
  int argc_save;
  char *argv_save[64];

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

   prio_slider = (Panel_item)xv_create(base_panel, PANEL_SLIDER,
	PANEL_LABEL_STRING,	"Priority",
	PANEL_VALUE,		1,
	PANEL_MIN_VALUE,	1,
	PANEL_MAX_VALUE,	100,
	PANEL_SLIDER_WIDTH,	100,
	PANEL_TICKS,		1,
	PANEL_NOTIFY_PROC,	prio_proc,
	NULL);

   regul_item = (Panel_item)xv_create(base_panel, PANEL_CHOICE,
	PANEL_LABEL_STRING,	"Regulation",
	PANEL_CHOICE_STRINGS,	"On", "Off", NULL,
	PANEL_NOTIFY_PROC,	toggle_regulate,
	PANEL_VALUE,		1,
	XV_X,			X_SIZE - 150,
	NULL);

   draw_button = (Panel_item)xv_create(base_panel, PANEL_BUTTON,
	PANEL_NEXT_ROW,		-1,
	PANEL_LABEL_STRING,	"Run",
	PANEL_NOTIFY_PROC,	lignes,
	NULL);

   color_choices = (Panel_item)xv_create(base_panel, PANEL_CHOICE,
	PANEL_LAYOUT,		PANEL_HORIZONTAL,
	PANEL_LABEL_STRING,	"Color",
	PANEL_CHOICE_IMAGES,	chip, chip, chip, chip, chip, chip, chip,
				chip, chip, chip, NULL,
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
	NULL);

   reset_button = (Panel_item)xv_create(base_panel, PANEL_BUTTON,
	PANEL_NEXT_ROW,		-1,
	PANEL_LABEL_STRING,	"Reset",
	PANEL_NOTIFY_PROC,	reset_threads,
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
   xgc_noir.background = WhitePixel(dpy, DefaultScreen(dpy));
   xgc_noir.line_width = 1;
   xgc_noir.function = GXcopy;
   gcmask = GCForeground | GCBackground | GCLineWidth | GCFunction;
   xgc_blanc = xgc_noir;
   xgc_blanc.foreground = WhitePixel(dpy, DefaultScreen(dpy));
   gc_blanc = XCreateGC(dpy, win, gcmask, &xgc_blanc);

   { XColor couleur;
     Colormap cmap;
     int i;

      cmap = XDefaultColormap(dpy, ecran);
      for(i=0; i<NB_COULEURS; i++) {
         gc_noir[i] = XCreateGC(dpy, win, gcmask, &xgc_noir);
         couleur.red = les_couleurs[i].rouge*256;
         couleur.green = les_couleurs[i].vert*256;
         couleur.blue = les_couleurs[i].bleu*256;
         XAllocColor(dpy, cmap, &couleur);
         XSetForeground(dpy, gc_noir[i], couleur.pixel);
      }
   }
   xv_set(base_frame, XV_SHOW, TRUE, NULL);

   pm2_init_rpc();
   regul_init_rpc();

   pm2_set_startup_func(startup_func);
   pm2_set_user_func(user_func);

   DECLARE_LRPC_WITH_NAME(GRAPH, "graph", OPTIMIZE_IF_LOCAL);
   DECLARE_LRPC(SYNC_DISPLAY);

   marcel_init();

   marcel_key_create(&color_key, NULL);

   pm2_init(&argc_save, argv_save, ASK_USER, &les_modules, &nb_modules);

   lock_task();
   sprintf(baniere, "Task [%x]  on  %s", pm2_self(), hostname);
   xv_set(base_frame, FRAME_LABEL, baniere, NULL);
   unlock_task();

   marcel_settimeslice_np(10);

   ma_main_loop();

   {
     int self = pm2_self();

     reset_threads();

     pm2_kill_modules(&self, 1);
   }

   regul_exit();
   pm2_exit();

   tfprintf(stderr, "Slave is ending\n");
   return 0;
}
