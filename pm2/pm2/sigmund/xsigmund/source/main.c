
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

#include <gtk/gtk.h>

#include "main.h"
#include "intro.h"
#include "statusbar.h"
#include "menu.h"
#include "init.h"
#include "options.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

gint destroy_phase = FALSE;

// Customization
gint tips_enabled = TRUE;
gint skip_intro = FALSE;
gint with_sound = FALSE;
static gint show_help = FALSE;

GtkWidget *main_window;
GtkTooltips *the_tooltip = NULL;


static gint delete_event(GtkWidget *widget,
			 GdkEvent  *event,
			 gpointer   data )
{
  return FALSE;
}

/* quit XSigmund */
static void destroy(GtkWidget *widget,
		    gpointer   data )
{
  gtk_main_quit();
}

/* parse the options */
static void parse_options(int *argc, char *argv[])
{
  int i, j;

  if (!argc)
    return;

  i = j = 1;

  while(i < *argc) {
    if(!strcmp(argv[i], "--expert") || !strcmp(argv[i], "-e")) {
      tips_enabled = FALSE;
      i++;
    } else if(!strcmp(argv[i], "--skip-intro") || !strcmp(argv[i], "-ni")) {
      skip_intro = TRUE;
      i++;
    } else if(!strcmp(argv[i], "--with-sound") || !strcmp(argv[i], "-s")) {
      with_sound = TRUE;
      i++;
    } else if(!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
      show_help = TRUE;
      i++;
    } else
      argv[j++] = argv[i++];
  }
  *argc = j;
  argv[j] = NULL;
}

static void do_show_help(char *cmd)
{
  g_print("Usage: %s { <option> }\n", cmd);
  g_print("<option> can be:\n");
  g_print("\t--expert | -e      : disables tips (noticeably accelerates startup!)\n");
  g_print("\t--skip-intro | -ni : skips introduction window\n");
  g_print("\t--with-sound | -s  : enables sounds during intro\n");
  g_print("\t--help | -h        : shows this help screen\n");
}


/* THE main...*/
int main(int argc, char *argv[])
{
  GtkWidget *main_vbox;
  GtkWidget *bottom_frame;
  GtkWidget *bottom_hbox;

  gtk_init(&argc, &argv);

  parse_options(&argc, argv);

  if(show_help) {
    do_show_help(argv[0]);
    exit(0);
  }

  intro_init();
  
  init_names();
  sleep (0);

  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (main_window), "sigmund");

  gtk_window_set_wmclass (GTK_WINDOW (main_window),
			  "sigmund", "sigmund");

  gtk_signal_connect(GTK_OBJECT(main_window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), NULL);

  gtk_signal_connect(GTK_OBJECT(main_window), "destroy",
		     GTK_SIGNAL_FUNC(destroy), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(main_window), 0);

  main_vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_border_width(GTK_CONTAINER(main_vbox), 1);
  gtk_container_add(GTK_CONTAINER(main_window), main_vbox);
  gtk_widget_show(main_vbox);

  menu_init(main_vbox);

  options_init(main_vbox);

  bottom_frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(bottom_frame), GTK_SHADOW_OUT);
  gtk_box_pack_end(GTK_BOX(main_vbox), bottom_frame, FALSE, FALSE, 0);
  gtk_widget_show(bottom_frame);

  bottom_hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(bottom_frame), bottom_hbox);
  gtk_widget_show(bottom_hbox);

  the_tooltip = gtk_tooltips_new();
  gtk_tooltips_set_delay(the_tooltip, 1000); /* 1 seconde */

  statusbar_init(bottom_hbox);

  intro_exit();

  gtk_widget_show(main_window);

  gtk_main();

  return 0;
}
