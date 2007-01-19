
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
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "module.h"
#include "flavor.h"
#include "intro.h"
#include "statusbar.h"
#include "menu.h"
#include "common_opt.h"

gint destroy_phase = FALSE;

// Customization
gint tips_enabled = FALSE;
gint skip_intro = FALSE;
gint with_sound = FALSE;
gint show_all_modules = FALSE;
static gint show_help = FALSE;

GtkWidget *main_window;
GtkTooltips *the_tooltip = NULL;

static gint delete_event(GtkWidget *widget,
			 GdkEvent  *event,
			 gpointer   data )
{
  if(flavor_modified) {
    flavor_check_quit();
    return TRUE;
  } else {
    destroy_phase = TRUE;
    return FALSE;
  }
}

static void destroy(GtkWidget *widget,
		    gpointer   data )
{
  gtk_main_quit();
}

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
    } else if(!strcmp(argv[i], "--tips") || !strcmp(argv[i], "-t")) {
      tips_enabled = TRUE;
      i++;
    } else if(!strcmp(argv[i], "--skip-intro") || !strcmp(argv[i], "-ni")) {
      skip_intro = TRUE;
      i++;
    } else if(!strcmp(argv[i], "--with-sound") || !strcmp(argv[i], "-s")) {
      with_sound = TRUE;
      i++;
    } else if(!strcmp(argv[i], "--all-modules") || !strcmp(argv[i], "-a")) {
      show_all_modules = TRUE;
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
  g_print("\t--expert      | -e  : disable tips (noticeably accelerates startup!)\n");
  g_print("\t--tips        | -t  : enable tips (noticeably slows down startup...)\n");
  g_print("\t--all-modules | -a  : show all modules\n");
  g_print("\t--skip-intro  | -ni : skip introduction window\n");
  g_print("\t--with-sound  | -s  : enable sounds during intro\n");
  g_print("\t--help        | -h  : show this help screen\n");
}

int main(int argc, char *argv[])
{
  GtkWidget *main_vbox;
  GtkWidget *bottom_frame;
  GtkWidget *bottom_hbox;
  GtkWidget *top_paned;
  GtkWidget *left_vbox, *right_vbox;

  gtk_init(&argc, &argv);

  parse_options(&argc, argv);

  if(show_help) {
    do_show_help(argv[0]);
    exit(0);
  }

  common_opt_init();

  intro_init();

  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (main_window), "ezflavor");

  gtk_window_set_wmclass (GTK_WINDOW (main_window),
			  "ezflavor", "ezflavor");

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

  top_paned = gtk_hpaned_new();
  gtk_widget_show(top_paned);
  gtk_container_set_border_width(GTK_CONTAINER(top_paned), 10);
  gtk_box_pack_start(GTK_BOX (main_vbox), top_paned, TRUE, TRUE, 0);

  left_vbox = gtk_vbox_new(FALSE, 10);
  gtk_paned_add1((GtkPaned *)top_paned, left_vbox);
  gtk_widget_show(left_vbox);

  right_vbox = gtk_vbox_new(FALSE, 10);
  gtk_paned_add2((GtkPaned *)top_paned, right_vbox);
  gtk_widget_show(right_vbox);

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

  flavor_init(left_vbox);

  module_init(left_vbox, right_vbox);

  intro_exit();

  flavor_modified = FALSE;

  gtk_widget_show(main_window);

  gtk_main();

  return 0;
}
