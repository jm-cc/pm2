
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "main.h"
#include "intro.h"
#include "trace.h"
#include "parser.h"

static GtkWidget *win = NULL;
static GtkWidget *label1 = NULL;
static GtkWidget *pbar   = NULL;
static GtkWidget *logo_area   = NULL;
static GdkPixmap *logo_pixmap = NULL;
static gint logo_width        = 0;
static gint logo_height       = 0;
static gint logo_area_width   = 0;
static gint logo_area_height  = 0;
static gint max_steps;
static gint step = 0;

guchar *pm2_root(void)
{
  guchar *ptr= getenv("PM2_ROOT");

  if(!ptr) {
    fprintf(stderr, "Error: undefined PM2_ROOT variable\n");
    exit(1);
  }

  return ptr;
}

static void logo_load_size(void)
{
  gchar buf[1024];
  FILE *fp;

  if (logo_pixmap)
    return;

  g_snprintf(buf, sizeof(buf), "%s/ezflavor/ezflavor.ppm", pm2_root());

  fp = fopen(buf, "rb");
  if(!fp) {
    perror("fopen");
    exit(1);
  }

  fgets (buf, sizeof (buf), fp);
  if(strcmp(buf, "P6\n") != 0) {
    fclose(fp);
    fprintf(stderr, "Error: bad ppm file header\n");
    exit(1);
  }

  fgets (buf, sizeof (buf), fp);
  fgets (buf, sizeof (buf), fp);
  sscanf (buf, "%d %d", &logo_width, &logo_height);

  fclose (fp);
}

static void logo_load(GtkWidget *window)
{
  GtkWidget *preview;
  GdkGC *gc;
  gchar   buf[1024];
  guchar *pixelrow;
  FILE *fp;
  gint count;
  gint i;

  if(logo_pixmap) {
    fprintf(stderr, "Curieux!!\n");
    return;
  }

  g_snprintf(buf, sizeof(buf), "%s/ezflavor/ezflavor.ppm", pm2_root());

  fp = fopen(buf, "rb");
  if(!fp) {
    perror("fopen");
    exit(1);
  }

  fgets(buf, sizeof (buf), fp);
  if(strcmp(buf, "P6\n") != 0) {
    fclose (fp);
    fprintf(stderr, "Error: bad ppm file header\n");
    exit(1);
  }

  fgets(buf, sizeof (buf), fp);
  fgets(buf, sizeof (buf), fp);
  sscanf(buf, "%d %d", &logo_width, &logo_height);

  fgets(buf, sizeof (buf), fp);
  if(strcmp (buf, "255\n") != 0) {
    fclose (fp);
    fprintf(stderr, "Error: bad ppm file header\n");
    exit(1);
  }

  preview = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW (preview), logo_width, logo_height);
  pixelrow = g_new(guchar, logo_width * 3);

  for (i = 0; i < logo_height; i++) {
    count = fread (pixelrow, sizeof (unsigned char), logo_width * 3, fp);
    if (count != (logo_width * 3)) {
      gtk_widget_destroy (preview);
      g_free (pixelrow);
      fclose (fp);
      fprintf(stderr, "Error: bad ppm file structure\n");
      exit(1);
    }
    gtk_preview_draw_row (GTK_PREVIEW (preview), pixelrow, 0, i, logo_width);
  }

  gtk_widget_realize (window);
  logo_pixmap = gdk_pixmap_new (window->window, logo_width, logo_height,
				gtk_preview_get_visual ()->depth);
  gc = gdk_gc_new (logo_pixmap);
  gtk_preview_put (GTK_PREVIEW (preview),
		   logo_pixmap, gc,
		   0, 0, 0, 0, logo_width, logo_height);
  gdk_gc_destroy (gc);

  gtk_widget_unref (preview);
  g_free (pixelrow);

  fclose (fp);
}

static void logo_draw(GtkWidget *widget)
{
  gdk_draw_pixmap (widget->window,
		   widget->style->black_gc,
		   logo_pixmap,
		   0, 0,
		   ((logo_area_width - logo_width) / 2),
		   ((logo_area_height - logo_height) / 2),
		   logo_width, logo_height);
}

static void logo_expose(GtkWidget *widget)
{
  logo_draw(widget);
}

static void destroy_initialization_window(void)
{
  if(win) {
    gtk_widget_destroy(win);

    if(logo_pixmap != NULL)
      gdk_pixmap_unref(logo_pixmap);

    win = label1 = pbar = logo_area = NULL;
    logo_pixmap = NULL;
  }
}

static void make_initialization_window(void)
{
  GtkWidget *vbox;
  GtkWidget *logo_hbox;

  win = gtk_window_new (GTK_WINDOW_DIALOG);

  gtk_window_set_title (GTK_WINDOW (win), "EZFLAVOR Startup");

  gtk_window_set_position(GTK_WINDOW (win), GTK_WIN_POS_CENTER);
  gtk_window_set_policy(GTK_WINDOW (win), FALSE, FALSE, FALSE);

  logo_load_size();

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (win), vbox);

  logo_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX(vbox), logo_hbox, FALSE, TRUE, 0);

  logo_area = gtk_drawing_area_new ();

  logo_load(win);

  logo_area_width  = logo_width;
  logo_area_height = logo_height;

  gtk_signal_connect (GTK_OBJECT (logo_area), "expose_event",
		      GTK_SIGNAL_FUNC (logo_expose),
		      NULL);

  gtk_drawing_area_size (GTK_DRAWING_AREA (logo_area),
			 logo_area_width, logo_area_height);

  gtk_box_pack_start (GTK_BOX(logo_hbox), logo_area, TRUE, FALSE, 0);

  label1 = gtk_label_new ("");
  gtk_box_pack_start_defaults (GTK_BOX (vbox), label1);

  pbar = gtk_progress_bar_new ();
  gtk_box_pack_start_defaults (GTK_BOX (vbox), pbar);

  gtk_widget_show (vbox);
  gtk_widget_show (logo_hbox);
  gtk_widget_show (logo_area);
  gtk_widget_show (label1);
  gtk_widget_show (pbar);

  gtk_widget_show (win);

  logo_draw(logo_area);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  gdk_flush();
}

static void compute_nb_steps(void)
{
  token_t tok;
  max_steps = 2; // pm2-module + pm2-flavor...

  parser_start_cmd("%s/bin/pm2-module modules", pm2_root());

  while((tok = parser_next_token()) != END_OF_INPUT)
    max_steps++;

  parser_stop();
}

static pid_t sound_pid;

void intro_init(void)
{
  char cmd[1024];

  if(!skip_intro) {
    make_initialization_window();

    compute_nb_steps();

    if(with_sound) {
      sprintf(cmd, "wavp %s/ezflavor/ezflavor.wav", pm2_root());
      sound_pid = exec_single_cmd_fmt(NULL, cmd);
    }
  }
}

void intro_exit(void)
{
  if(!skip_intro) {
    destroy_initialization_window();
    if(with_sound)
      exec_wait(sound_pid);
  }
}

void intro_begin_step(gchar *fmt, ...)
{
  static char msg[4096];
  va_list vl;

  va_start(vl, fmt);
  vsprintf(msg, fmt, vl);
  va_end(vl);

  if(skip_intro) {
    TRACE(msg);
  } else {
    gtk_label_set(GTK_LABEL(label1), msg);

    while (gtk_events_pending ())
      gtk_main_iteration ();

    gdk_flush ();
  }
}

void intro_end_step(void)
{
  float progress;

  if(skip_intro) {
    TRACE("Done.\n");
  } else {
    gtk_label_set(GTK_LABEL(label1), "");

    step++;

    progress = (float)step/max_steps;

    gtk_progress_bar_update (GTK_PROGRESS_BAR (pbar), progress);

    while (gtk_events_pending ())
      gtk_main_iteration ();

    gdk_flush ();
  }
}
