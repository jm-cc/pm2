
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
#include "menu.h"
#include "options.h"
#include "shell.h"
#include "print.h"
#include <string.h>
#include <stdio.h>

static GtkItemFactory *item_factory;

/* callbacks concerning the actions in the menu File/ */
static void quit_callback(GtkWidget *w, gpointer data)
{
  gtk_main_quit();
}

static void open_file_callback(GtkWidget *w, gpointer data)
{
  create_fileselection();
  pause();
}

static void option_show_callback(GtkWidget *w, gpointer data)
{
  /* has to be connected */
  char s[1000];
  options_to_string(s);
  printf("options string: %s\n", s);
}

static void print_callback(GtkWidget *w, gpointer data)
{
  /* has to be connected */
  show_print();

}


/* callbacks in the menu View/ */
static void cpu_view_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmundps ");
  options_to_string(s + 10);
  strcat(s, " --print-process");
  strcat(s, " -o /tmp/pipo.eps");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
  pid = exec_single_cmd_fmt(NULL, "gv /tmp/pipo.eps");
  exec_wait(pid);
  pid = exec_single_cmd_fmt(NULL, "rm -f /tmp/pipo.eps");
  exec_wait(pid); 
}

static void thread_view_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmundps ");
  options_to_string(s + 10);
  strcat(s, " --print-thread");
  strcat(s, " -o /tmp/pipo.eps");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid); 
  pid = exec_single_cmd_fmt(NULL, "gv /tmp/pipo.eps");
  exec_wait(pid);
  pid = exec_single_cmd_fmt(NULL, "rm -f /tmp/pipo.eps");
  exec_wait(pid); 
}


/* callbacks concerning tha actions in the menu View/Text/ */
static void list_events_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmund ");
  options_to_string(s + 8);
  strcat(s, " --list-events");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
}

static void nb_events_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmund ");
  options_to_string(s + 8);
  strcat(s, " --nb-events");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
}

static void nb_calls_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmund ");
  options_to_string(s + 8);
  strcat(s, " --nb-calls");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
}

static void active_time_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmund ");
  options_to_string(s + 8);
  strcat(s, " --active-time");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
}


static void idle_time_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmund ");
  options_to_string(s + 8);
  strcat(s, " --idle-time");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
}

static void time_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmund ");
  options_to_string(s + 8);
  strcat(s, " --time");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
}


static void act_slices_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmund ");
  options_to_string(s + 8);
  strcat(s, " --act-slices");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
}

static void avg_slices_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmund ");
  options_to_string(s + 8);
  strcat(s, " --avg-slices");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
}

static void function_time_callback(GtkWidget *w, gpointer data)
{
  int pid;
  char s[1010];
  strcpy(s, "sigmund ");
  options_to_string(s + 8);
  strcat(s, " --function-time");
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
}

/* Creation of the menubar */
static GtkItemFactoryEntry menu_items[] = {
  { "/_File",         NULL,          NULL, 0, "<Branch>" },
  { "/File/Open",     NULL,          open_file_callback, 0, NULL },
  { "/File/Options string",    NULL,    option_show_callback, 0, NULL },
  { "/File/sep1",     NULL,          NULL, 0, "<Separator>" },
  { "/File/Print",    "<control>P",  print_callback, 0, NULL },
  { "/File/sep1",     NULL,          NULL, 0, "<Separator>" },
  { "/File/Quit",     "<control>Q",  quit_callback, 0, NULL },
  { "/_View",         NULL,          NULL, 0, "<Branch>" },
  { "/View/CPU",      "<control>C",  cpu_view_callback, 0, NULL },
  { "/View/Thread",     "<control>T",  thread_view_callback, 0, NULL },
  { "/View/Text",   NULL,          NULL, 0, "<LastBranch>"},
  { "/View/Text/list-events",    NULL,        list_events_callback, 0,NULL },
  { "/View/Text/nb-events",      NULL,        nb_events_callback, 0, NULL},
  { "/View/Text/nb-calls",       NULL,        nb_calls_callback , 0,NULL},
  { "/View/Text/active-time",    NULL,        active_time_callback, 0,NULL},
  { "/View/Text/idle-time",      NULL,        idle_time_callback, 0, NULL},
  { "/View/Text/time",           NULL,        time_callback, 0,NULL},
  { "/View/Text/active-slices",  NULL,        act_slices_callback , 0,NULL},
  { "/View/Text/avg-active-slices",   NULL,      avg_slices_callback , 0,NULL},
  { "/View/Text/function-time",  NULL,        function_time_callback , 0,NULL},
       };


void menu_init(GtkWidget *vbox)
{
  GtkAccelGroup *accel_group;
  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
  GtkWidget *menubar;

  accel_group = gtk_accel_group_new();

  item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", 
				      accel_group);

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);

  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
  gtk_widget_show(menubar);
}

