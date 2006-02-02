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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
} bubble_t;

typedef struct {
	GtkWidget *label;
} task_t;

static GtkWidget *main_window;
static GtkItemFactory *item_factory;

static bubble_t *main_bubble;

static void destroy(GtkWidget *widget, gpointer data) {
  gtk_main_quit();
}

static void go(GtkWidget *widget, gpointer data) {
}

static GtkItemFactoryEntry menu_items[]= {
  { "/_File",		NULL,	NULL, 0, "<Branch>" },
  { "/File/_Quit",	"<control>Q", destroy, 0, NULL },
  { "/File/_Go",	"<control>G", go, 0, NULL },
};

static task_t *new_task_widget(bubble_t *holder);

static void taskplusplus(GtkWidget *widget, gpointer data) {
  bubble_t *bubble = (bubble_t *) data;
  new_task_widget(bubble);
}

static bubble_t *new_bubble_widget(bubble_t *holder);
static void bubbleplusplus(GtkWidget *widget, gpointer data) {
  bubble_t *bubble = (bubble_t *) data;
  new_bubble_widget(bubble);
}

static task_t *new_task_widget(bubble_t *holder) {
  task_t *task;
  static int num = 0;
  static char name[16];
  if (!(task = malloc(sizeof(*task)))) {
    fprintf(stderr,"no room for task");
    abort();
  }
  snprintf(name,sizeof(name),"thread %d",num++);
  task->label = gtk_label_new(name);
  gtk_widget_show(task->label);
  if (holder) {
    gtk_box_pack_start(GTK_BOX(holder->vbox), task->label, FALSE, FALSE, 0);
  }
  return task;
}

static bubble_t *new_bubble_widget(bubble_t *holder) {
  bubble_t *bubble;
  GtkWidget *frame,/**label,*/*button,*hbox,*box,*vbox;

  if (!(bubble = malloc(sizeof(*bubble)))) {
    fprintf(stderr,"no room for bubble");
    abort();
  }

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_widget_show(hbox);

  /*
  bubble->label = label = gtk_label_new("bubble");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  */

  button = gtk_button_new_with_label("bubble++");
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		  GTK_SIGNAL_FUNC(bubbleplusplus), bubble);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("task++");
  gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		  GTK_SIGNAL_FUNC(taskplusplus), bubble);
  gtk_widget_show(button);

  bubble->hbox = box = gtk_hbox_new(TRUE, 1);
  gtk_widget_show(box);

  bubble->vbox = vbox = gtk_vbox_new(TRUE, 1);
  gtk_widget_show(vbox);
  gtk_box_pack_start(GTK_BOX(box), vbox, TRUE, TRUE, 0);

  bubble->frame = frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
  gtk_frame_set_label_widget(GTK_FRAME(frame), hbox);
  if (holder) {
    gtk_box_pack_start(GTK_BOX(holder->hbox), frame, TRUE, TRUE, 0);
  }
  gtk_widget_show(frame);

  return bubble;
}

static void menu_init(GtkWidget *box)  {
  GtkAccelGroup *accel_group;
  gint nmenu_items = sizeof(menu_items) / sizeof (menu_items[0]);
  GtkWidget *menubar;

  accel_group = gtk_accel_group_new();
  item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  gtk_box_pack_start(GTK_BOX(box), menubar, FALSE, FALSE, 0);
  gtk_widget_show(menubar);
}

int main(int argc, char *argv[])
{
  GtkWidget *main_vbox;

  gtk_init(&argc, &argv);

  /* main window */
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW (main_window), "bubble gum");
  gtk_window_set_wmclass(GTK_WINDOW (main_window), "bubble gum", "bubble gum");

  gtk_signal_connect(GTK_OBJECT(main_window), "destroy",
		     GTK_SIGNAL_FUNC(destroy), NULL);

  /* vertical box for packing menu */
  main_vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_border_width(GTK_CONTAINER(main_vbox), 1);
  gtk_container_add(GTK_CONTAINER(main_window), main_vbox);
  gtk_widget_show(main_vbox);
  menu_init(main_vbox);

  /* frame for bubbles */
  main_bubble = new_bubble_widget(NULL);
  gtk_widget_set_size_request(main_bubble->frame, 800, 600);
  gtk_box_pack_end(GTK_BOX(main_vbox), main_bubble->frame, TRUE, TRUE, 0);

  /* go go go ! */
  gtk_widget_show(main_window);
  gtk_main();
  return 0;
}

