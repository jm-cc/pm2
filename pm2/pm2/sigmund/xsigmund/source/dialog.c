
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

#include "dialog.h"


/* strucure of a dailog box */
typedef struct {
  GtkWidget *window;
  GtkWidget *button1, *button2, *button3;
  GtkWidget *label;
  GtkSignalFunc func1, func2, func3;
  gpointer data1, data2, data3;
} dialog_param_t;



/* callbacks for dialog box */
static void callback1(GtkWidget *widget,
		      gpointer data)
{
  dialog_param_t *ptr = (dialog_param_t *)data;

  if(ptr->func1) {
    (*ptr->func1)(ptr->data1);
  }

  gtk_widget_destroy(ptr->window);
  g_free(ptr);
}

static void callback2(GtkWidget *widget,
		      gpointer data)
{
  dialog_param_t *ptr = (dialog_param_t *)data;

  if(ptr->func2) {
    (*ptr->func2)(ptr->data2);
  }

  gtk_widget_destroy(ptr->window);
  g_free(ptr);
}

static void callback3(GtkWidget *widget,
		      gpointer data)
{
  dialog_param_t *ptr = (dialog_param_t *)data;

  if(ptr->func3) {
    (*(ptr->func3))(ptr->data3);
  }

  gtk_widget_destroy(ptr->window);
  g_free(ptr);
}


/* main function */
void dialog_prompt(char *question,
		   char *choice1, dialog_func_t func1, gpointer data1,
		   char *choice2, dialog_func_t func2, gpointer data2,
		   char *choice3, dialog_func_t func3, gpointer data3)
{
  dialog_param_t *ptr;

  ptr = (dialog_param_t *)g_malloc(sizeof(dialog_param_t));

  ptr->func1 = func1; ptr->func2 = func2; ptr->func3 = func3;
  ptr->data1 = data1; ptr->data2 = data2; ptr->data3 = data3;

  ptr->window = gtk_dialog_new();
  gtk_window_set_position(GTK_WINDOW (ptr->window), GTK_WIN_POS_CENTER);

  gtk_container_set_border_width (GTK_CONTAINER(GTK_DIALOG (ptr->window)->vbox), 10);

  ptr->label = gtk_label_new(question);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ptr->window)->vbox),
		      ptr->label, TRUE, TRUE, 0);
  gtk_widget_show(ptr->label);

  if(choice1) {
    ptr->button1 = gtk_button_new_with_label(choice1);
    gtk_signal_connect(GTK_OBJECT(ptr->button1), "clicked",
		       GTK_SIGNAL_FUNC(callback1), (gpointer)ptr);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG (ptr->window)->action_area),
		       ptr->button1, TRUE, TRUE, 0);
    gtk_widget_show(ptr->button1);
  }

  if(choice2) {
    ptr->button2 = gtk_button_new_with_label(choice2);
    gtk_signal_connect(GTK_OBJECT(ptr->button2), "clicked",
		       GTK_SIGNAL_FUNC(callback2), (gpointer)ptr);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG (ptr->window)->action_area),
		       ptr->button2, TRUE, TRUE, 0);
    gtk_widget_show(ptr->button2);
  }

  if(choice3) {
    ptr->button3 = gtk_button_new_with_label(choice3);
    gtk_signal_connect(GTK_OBJECT(ptr->button3), "clicked",
		       GTK_SIGNAL_FUNC(callback3), (gpointer)ptr);
    gtk_box_pack_start(GTK_BOX (GTK_DIALOG (ptr->window)->action_area),
		       ptr->button3, TRUE, TRUE, 0);
    gtk_widget_show(ptr->button3);
  }

  gtk_widget_show(ptr->window);
}
