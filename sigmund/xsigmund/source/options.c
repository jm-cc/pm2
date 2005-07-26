
#ifdef GTK2
#define GTK_ENABLE_BROKEN
#ifdef PM2_DEV
#warning TODO: fix GTK2 code
#endif
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include "options.h"
#include "init.h"
#include <string.h>
#include "statusbar.h"
#include "sigmund.h"
#include <ctype.h>
#include <fkt/names.h>

static char supertrace[1000];

static GtkWidget *checkbutton_cpu_1;
static GtkWidget *checkbutton_cpu_2;
static GtkWidget *checkbutton_cpu_3;
static GtkWidget *checkbutton_cpu_4;

static GtkWidget *combo_proc;
static GList *combo_proc_items = NULL;
static GtkWidget *entry_proc;
static GtkWidget *clist_proc;
static gint proc_row_select = -1;

static GtkWidget *checkbutton_thread;
static GtkWidget *text_thread;

static GtkWidget *checkbutton_time;
static GtkWidget *text_time;

static GtkWidget *combo_events;
static GList *combo_events_items = NULL;
static GtkWidget *entry_events;
static GtkWidget *clist_event;
static gint event_row_select = -1;

static GtkWidget *combo_function;
static GList *combo_function_items = NULL;
static GtkWidget *entry_function;
static GtkWidget *clist_function;
static gint function_row_select = -1;

static GtkWidget *checkbutton_evnum;
static GtkWidget *text_evnum;

static GtkWidget *text_begin;

static GtkWidget *fileselection;

static char *itoa(int i)
{
  static char s[20];
  sprintf(s,"%d",i);
  return s;
}

/* add a thread to the filter */
static void add_thread(char *s)
{
  int first = 1;
  int i;
  int begin_thread = -10;
  int end_thread = -10;
  gchar m;
  GtkText *t = GTK_TEXT(text_thread);
  for(i = 0; (m = GTK_TEXT_INDEX(t, i)) != '\0'; i++) {
    if (m == ' ') {continue;}
    if (m == ';') {
      if (begin_thread != -10) {
	if (end_thread != -10) {
	  int j;
	  for(j = begin_thread; j <= end_thread; j++) {
	    strcat(s, " --thread ");
	    strcat(s, itoa(j));
	  }
	} else {
	  strcat(s, " --thread ");
	  strcat(s, itoa(begin_thread));
	}
      }

      begin_thread = -10;
      end_thread = -10;
      first = 1;
      continue;
    }
    if (isdigit(m)) {
      if (first) {
	if (begin_thread == -10) begin_thread = m - '0';
	else begin_thread = begin_thread * 10 + m - '0';
      } else 
	end_thread = end_thread * 10 + m - '0';
      continue;
    }
    if (m == '-') {
      if (begin_thread == -10) begin_thread = 0;
      first = 0;
      end_thread = 0;
      continue;
    }
  }
  if (begin_thread != -10) {
    if (end_thread != -10) {
      int j;
      for(j = begin_thread; j <= end_thread; j++) {
	strcat(s, " --thread ");
	strcat(s, itoa(j));
      }
    } else {
      strcat(s, " --thread ");
      strcat(s, itoa(begin_thread));
    }
  }
}


/* add a time-slice zone */
static void add_time(char *s)
{
  int first = 1;
  int i;
  int begin_time = 0;
  int end_time = 0;
  gchar m;
  GtkText *t = GTK_TEXT(text_time);
  for(i = 0; (m = GTK_TEXT_INDEX(t, i)) != '\0'; i++) {
    if (m == ' ') {continue;}
    if (m == ';') {
      strcat(s, " --time-slice ");
      strcat(s, itoa(begin_time));
      strcat(s, " ");
      strcat(s, itoa(end_time));
      first = 1;
      continue;
    }
    if (isdigit(m)) {
      if (first)
	begin_time = begin_time * 10 + m - '0';
      else 
	end_time = end_time * 10 + m - '0';
      continue;
    }
    if (m == '-') {
      first = 0;
      end_time = 0;
      continue;
    }
  }
  if (!first) {
    strcat(s, " --time-slice ");
    strcat(s, itoa(begin_time));
    strcat(s, " ");
    strcat(s, itoa(end_time));
  }
}


/* add an event in the filter */
static void add_evnum(char *s)
{
  int first = 1;
  int i;
  int begin_evnum = 0;
  int end_evnum = 0;
  gchar m;
  GtkText *t = GTK_TEXT(text_evnum);
  for(i = 0; (m = GTK_TEXT_INDEX(t, i)) != '\0'; i++) {
    if (m == ' ') {continue;}
    if (m == ';') {
      strcat(s, " --event-slice ");
      strcat(s, itoa(begin_evnum));
      strcat(s, " ");
      strcat(s, itoa(end_evnum));
      first = 1;
      continue;
    }
    if (isdigit(m)) {
      if (first)
	begin_evnum = begin_evnum * 10 + m - '0';
      else 
	end_evnum = end_evnum * 10 + m - '0';
      continue;
    }
    if (m == '-') {
      first = 0;
      end_evnum = 0;
      continue;
    }
  }
  if (!first) {
    strcat(s, " --event-slice ");
    strcat(s, itoa(begin_evnum));
    strcat(s, " ");
    strcat(s, itoa(end_evnum));
  }
}

/* add the action given by the string s in the filter */
static void add_begin(char *s)
{
  gchar *m = gtk_editable_get_chars(GTK_EDITABLE(&(GTK_TEXT(text_begin)->old_editable)), 0, -1);
  strcat(s, m);
  g_free(m);
}

/* associate the graphic otions to the string that 
   must be given in the console mode */
void options_to_string(char *s)
{
  gboolean cpu_1;        /* cpu */
  gboolean cpu_2;
  gboolean cpu_3;
  gboolean cpu_4;
  gchar *m;
  int row;

  cpu_1 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_cpu_1));
  cpu_2 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_cpu_2));
  cpu_3 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_cpu_3));
  cpu_4 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_cpu_4));

  s[0] = '\0';

  strcat(s, " --trace-file ");
  strcat(s, supertrace);
  
  if (!cpu_1 || !cpu_2 || !cpu_3 || !cpu_4) {
    if (cpu_1) strcat(s," --cpu 0");
    if (cpu_2) strcat(s," --cpu 1");
    if (cpu_3) strcat(s," --cpu 2");
    if (cpu_4) strcat(s," --cpu 3");
  }
  
  for(row = 0;; row ++) {         /* process */
    if (gtk_clist_get_text(GTK_CLIST(clist_proc), row, 0, &m) == 0) break;
    strcat(s, " --process ");
    strcat(s, m);
  }
  
  for(row = 0;; row ++) {        /* events */
    if (gtk_clist_get_text(GTK_CLIST(clist_event), row, 0, &m) == 0) break;
    strcat(s, " --event ");
    strcat(s, m);
  }
  
  for(row = 0;; row ++) {       /* functions */
    if (gtk_clist_get_text(GTK_CLIST(clist_function), row, 0, &m) == 0) break;
    strcat(s, " --function ");
    strcat(s, m);
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_thread)))
    add_thread(s);
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_time)))
    add_time(s);  
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_evnum)))
    add_evnum(s);
  add_begin(s);
  
}


/* callbacks to select the process wanted in the filter */
static void proc_all_callback(GtkWidget *w, gpointer data)
{
  GList *tmp;
  gchar *adding;
  gtk_clist_clear(GTK_CLIST(clist_proc));
  tmp = combo_proc_items;
  while (tmp != NULL) {
    adding = tmp->data;
    gtk_clist_append(GTK_CLIST(clist_proc),&adding);
    tmp = g_list_next(tmp);
  }
}

static void proc_add_one_callback(GtkWidget *w, gpointer data)
{
  gchar *adding;
  adding = gtk_entry_get_text(GTK_ENTRY(entry_proc)); 
  gtk_clist_append(GTK_CLIST(clist_proc), &adding);
}

static void proc_del_one_callback(GtkWidget *w, gpointer data)
{
  if (proc_row_select != -1)
    gtk_clist_remove(GTK_CLIST(clist_proc), proc_row_select);
}

static void proc_del_all_callback(GtkWidget *w, gpointer data)
{
  gtk_clist_clear(GTK_CLIST(clist_proc));
}

static void proc_select_callback(GtkWidget *w, gint row, gint column, GdkEventButton *event, gpointer data)
{
  proc_row_select = row;
}

static void proc_unselect_callback(GtkWidget *w, gint row, gint column, GdkEventButton *event, gpointer data)
{
  proc_row_select = -1;
}


/* callbacks to select the events wanted in the filter */
static void event_all_callback(GtkWidget *w, gpointer data)
{
  GList *tmp;
  gchar *adding;
  gtk_clist_clear(GTK_CLIST(clist_event));
  tmp = combo_events_items;
  while (tmp != NULL) {
    adding = tmp->data;
    gtk_clist_append(GTK_CLIST(clist_event),&adding);
    tmp = g_list_next(tmp);
  }
}

static void event_add_one_callback(GtkWidget *w, gpointer data)
{
  gchar *adding;
  adding = gtk_entry_get_text(GTK_ENTRY(entry_events));
  gtk_clist_append(GTK_CLIST(clist_event), &adding);
}

static void event_del_one_callback(GtkWidget *w, gpointer data)
{
  if (event_row_select != -1)
    gtk_clist_remove(GTK_CLIST(clist_event), event_row_select);
}

static void event_del_all_callback(GtkWidget *w, gpointer data)
{
  gtk_clist_clear(GTK_CLIST(clist_event));
}

static void event_select_callback(GtkWidget *w, gint row, gint column, GdkEventButton *event, gpointer data)
{
  event_row_select = row;
}

static void event_unselect_callback(GtkWidget *w, gint row, gint column, GdkEventButton *event, gpointer data)
{
  event_row_select = -1;
}


/* callbacks to select the functions wanted in the filter */
static void function_all_callback(GtkWidget *w, gpointer data)
{
  GList *tmp;
  gchar *adding;
  gtk_clist_clear(GTK_CLIST(clist_function));
  tmp = combo_function_items;
  while (tmp != NULL) {
    adding = tmp->data;
    gtk_clist_append(GTK_CLIST(clist_function),&adding);
    tmp = g_list_next(tmp);
  }
}

static void function_add_one_callback(GtkWidget *w, gpointer data)
{
  gchar *adding;
  adding = gtk_entry_get_text(GTK_ENTRY(entry_function)); 
  gtk_clist_append(GTK_CLIST(clist_function), &adding);
}

static void function_del_one_callback(GtkWidget *w, gpointer data)
{
  if (function_row_select != -1)
    gtk_clist_remove(GTK_CLIST(clist_function), function_row_select);
}

static void function_del_all_callback(GtkWidget *w, gpointer data)
{
  gtk_clist_clear(GTK_CLIST(clist_function));
}

static void function_select_callback(GtkWidget *w, gint row, gint column, GdkEventButton *event, gpointer data)
{
  function_row_select = row;
}

static void function_unselect_callback(GtkWidget *w, gint row, gint column, GdkEventButton *event, gpointer data)
{
  function_row_select = -1;
}


/* reset all the options and does not select anything, as at the beginning */
static void reset_callback(GtkWidget *w, gpointer data)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_cpu_1), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_cpu_2), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_cpu_3), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_cpu_4), TRUE);

  gtk_clist_clear(GTK_CLIST(clist_proc));
  gtk_clist_clear(GTK_CLIST(clist_event));
  gtk_clist_clear(GTK_CLIST(clist_function));

  gtk_text_set_point(GTK_TEXT(text_thread), 0);
  gtk_text_forward_delete(GTK_TEXT(text_thread), gtk_text_get_length(GTK_TEXT(text_thread))); 
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_thread), FALSE);

  gtk_text_set_point(GTK_TEXT(text_time), 0);
  gtk_text_forward_delete(GTK_TEXT(text_time), gtk_text_get_length(GTK_TEXT(text_time))); 
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_time), FALSE);

  gtk_text_set_point(GTK_TEXT(text_evnum), 0);
  gtk_text_forward_delete(GTK_TEXT(text_evnum), gtk_text_get_length(GTK_TEXT(text_evnum))); 
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_evnum), FALSE);

  gtk_text_set_point(GTK_TEXT(text_begin), 0);
  gtk_text_forward_delete(GTK_TEXT(text_begin), gtk_text_get_length(GTK_TEXT(text_begin))); 
  
}

/* this sis the window contained in the main one 
   where you can select the options you want for the filters */
static void create_window(GtkWidget *main_vbox)
{
  /* the components */
  GtkWidget *vbox;
  GtkWidget *vbox_notebook;
  GtkWidget *notebook;
  GtkWidget *vbox_cpu;
  GtkWidget *label_cpu;
  GtkWidget *vbox_proc;
  GtkWidget *hbuttonbox_proc;
  GtkWidget *button_proc_all;
  GtkWidget *button_proc_add_one;
  GtkWidget *button_proc_delete_one;
  GtkWidget *button_proc_delete_all;
  GtkWidget *hbox_proc_select;
  GtkWidget *scrolledwindow_proc;
  GtkWidget *label_act_proc;
  GtkWidget *label_proc;
  GtkWidget *vbox_thread;
  GtkWidget *hbox_high_thread;
  GtkWidget *label_thr_void1;
  GtkWidget *vbox_hight_thread;
  GtkWidget *label_thr_void3;
  GtkWidget *label_thr_void4;
  GtkWidget *label_thr_void2;
  GtkWidget *label_explain_thread;
  GtkWidget *scrolledwindow_thread;
  GtkWidget *label_thread;
  GtkWidget *vbox_time;
  GtkWidget *hbox_high_time;
  GtkWidget *label_time_void1;
  GtkWidget *vbox_high_time;
  GtkWidget *label_time_void3;
  GtkWidget *label_time_void4;
  GtkWidget *label_time_void2;
  GtkWidget *label_explain_time;
  GtkWidget *scrolledwindow_time;
  GtkWidget *label_time;
  GtkWidget *vbox_event;
  GtkWidget *hbuttonbox_events;
  GtkWidget *button_all_events;
  GtkWidget *button_add_event;
  GtkWidget *button_delete_event;
  GtkWidget *button_delete_all_event;
  GtkWidget *hbox_event;
  GtkWidget *scrolledwindow_events;
  GtkWidget *label_activ_events;
  GtkWidget *label_event;
  GtkWidget *vbox_function;
  GtkWidget *hbuttonbox_function;
  GtkWidget *button_all_functions;
  GtkWidget *button_add_function;
  GtkWidget *button_delete_function;
  GtkWidget *button_delete_all_function;
  GtkWidget *hbox_function;
  GtkWidget *scrolledwindow_function;
  GtkWidget *label_activ_function;
  GtkWidget *label_function;
  GtkWidget *vbox_evnum;
  GtkWidget *hbox_high_evnum;
  GtkWidget *label_evnum_void3;
  GtkWidget *vbox_high_evnum;
  GtkWidget *label_evnum_void1;
  GtkWidget *label_evnum_void2;
  GtkWidget *label_evnum_void4;
  GtkWidget *label_explain_evenum;
  GtkWidget *scrolledwindow_evnum;
  GtkWidget *label_evnum;
  GtkWidget *vbox_begin;
  GtkWidget *label_explain_begin;
  GtkWidget *scrolledwindow_begin;
  GtkWidget *label_begin_end;
  GtkWidget *hbuttonbox_reset;
  GtkWidget *button_reset;

  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);

  vbox_notebook = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_notebook);
  gtk_widget_show (vbox_notebook);
  gtk_box_pack_start (GTK_BOX (vbox), vbox_notebook, TRUE, TRUE, 0);

  notebook = gtk_notebook_new ();
  gtk_widget_ref (notebook);
  gtk_widget_show (notebook);
  gtk_box_pack_start (GTK_BOX (vbox_notebook), notebook, TRUE, TRUE, 0);
  
  /* vbox for the cpu choice */
  vbox_cpu = gtk_vbox_new (FALSE, 15);
  gtk_widget_ref (vbox_cpu);
  gtk_widget_show (vbox_cpu);
  gtk_container_add (GTK_CONTAINER (notebook), vbox_cpu);
  gtk_container_set_border_width (GTK_CONTAINER (vbox_cpu), 120);
  GTK_WIDGET_SET_FLAGS (vbox_cpu, GTK_CAN_DEFAULT);
  
  /* the checkbuttons for the cpu */
  checkbutton_cpu_1 = gtk_check_button_new_with_label ("CPU 1");
  gtk_widget_ref (checkbutton_cpu_1);
  gtk_widget_show (checkbutton_cpu_1);
  gtk_box_pack_start (GTK_BOX (vbox_cpu), checkbutton_cpu_1, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_cpu_1), TRUE);

  checkbutton_cpu_2 = gtk_check_button_new_with_label ("CPU 2");
  gtk_widget_ref (checkbutton_cpu_2);
  gtk_widget_show (checkbutton_cpu_2);
  gtk_box_pack_start (GTK_BOX (vbox_cpu), checkbutton_cpu_2, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_cpu_2), TRUE);

  checkbutton_cpu_3 = gtk_check_button_new_with_label ("CPU 3");
  gtk_widget_ref (checkbutton_cpu_3);
  gtk_widget_show (checkbutton_cpu_3);
  gtk_box_pack_start (GTK_BOX (vbox_cpu), checkbutton_cpu_3, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_cpu_3), TRUE);

  checkbutton_cpu_4 = gtk_check_button_new_with_label ("CPU 4");
  gtk_widget_ref (checkbutton_cpu_4);
  gtk_widget_show (checkbutton_cpu_4);
  gtk_box_pack_start (GTK_BOX (vbox_cpu), checkbutton_cpu_4, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_cpu_4), TRUE);

  /* the label cpu, on the top of the notebook */
  label_cpu = gtk_label_new ("cpu");
  gtk_widget_ref (label_cpu);
  gtk_widget_show (label_cpu);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 0), label_cpu);
  gtk_label_set_justify (GTK_LABEL (label_cpu), GTK_JUSTIFY_FILL);

  /* vbox for the process choice */
  vbox_proc = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_proc);
  gtk_widget_show (vbox_proc);
  gtk_container_add (GTK_CONTAINER (notebook), vbox_proc);
  
  /* the buttons on the top of the vbox */
  hbuttonbox_proc = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox_proc);
  gtk_widget_show (hbuttonbox_proc);
  gtk_box_pack_start (GTK_BOX (vbox_proc), hbuttonbox_proc, TRUE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox_proc), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox_proc), 85, 31);

  button_proc_all = gtk_button_new_with_label ("ALL");
  gtk_widget_ref (button_proc_all);
  gtk_widget_show (button_proc_all);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_proc), button_proc_all);
  gtk_signal_connect(GTK_OBJECT(button_proc_all), "clicked",
		     GTK_SIGNAL_FUNC(proc_all_callback), NULL);

  button_proc_add_one = gtk_button_new_with_label ("ADD ONE");
  gtk_widget_ref (button_proc_add_one);
  gtk_widget_show (button_proc_add_one);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_proc), button_proc_add_one);
  gtk_signal_connect(GTK_OBJECT(button_proc_add_one), "clicked",
		     GTK_SIGNAL_FUNC(proc_add_one_callback), NULL);

  button_proc_delete_one = gtk_button_new_with_label ("DELETE ONE");
  gtk_widget_ref (button_proc_delete_one);
  gtk_widget_show (button_proc_delete_one);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_proc), button_proc_delete_one);
  gtk_signal_connect(GTK_OBJECT(button_proc_delete_one), "clicked",
		     GTK_SIGNAL_FUNC(proc_del_one_callback), NULL);

  button_proc_delete_all = gtk_button_new_with_label ("DELETE ALL");
  gtk_widget_ref (button_proc_delete_all);
  gtk_widget_show (button_proc_delete_all);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_proc), button_proc_delete_all);
  gtk_signal_connect(GTK_OBJECT(button_proc_delete_all), "clicked",
  		     GTK_SIGNAL_FUNC(proc_del_all_callback), NULL);


  hbox_proc_select = gtk_hbox_new (TRUE, 5);
  gtk_widget_ref (hbox_proc_select);
  gtk_widget_show (hbox_proc_select);
  gtk_box_pack_start (GTK_BOX (vbox_proc), hbox_proc_select, TRUE, TRUE, 0);

  /* the combo box with all the process */
  combo_proc = gtk_combo_new ();
  gtk_widget_ref (combo_proc);
  gtk_widget_show (combo_proc);
  gtk_box_pack_start (GTK_BOX (hbox_proc_select), combo_proc, TRUE, FALSE, 0);
  

  entry_proc = GTK_COMBO (combo_proc)->entry;
  gtk_widget_ref (entry_proc);
  gtk_widget_show (entry_proc);

  scrolledwindow_proc = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow_proc);
  gtk_widget_show (scrolledwindow_proc);
  gtk_box_pack_start (GTK_BOX (hbox_proc_select), scrolledwindow_proc, TRUE, TRUE, 15);

  clist_proc = gtk_clist_new (1);
  gtk_widget_ref (clist_proc);
  gtk_widget_show (clist_proc);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_proc), clist_proc);
  gtk_clist_set_column_width (GTK_CLIST (clist_proc), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist_proc));
  gtk_clist_set_auto_sort(GTK_CLIST (clist_proc), TRUE);
  gtk_signal_connect(GTK_OBJECT(clist_proc), "select_row", GTK_SIGNAL_FUNC(proc_select_callback), NULL);
  gtk_signal_connect(GTK_OBJECT(clist_proc), "unselect_row", GTK_SIGNAL_FUNC(proc_unselect_callback), NULL);

  label_act_proc = gtk_label_new ("activated process");
  gtk_widget_ref (label_act_proc);
  gtk_widget_show (label_act_proc);
  gtk_clist_set_column_widget (GTK_CLIST (clist_proc), 0, label_act_proc);

  label_proc = gtk_label_new ("proc");
  gtk_widget_ref (label_proc);
  gtk_widget_show (label_proc);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1), label_proc);


  /* vbox for the thread choice */
  vbox_thread = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_thread);
  gtk_widget_show (vbox_thread);
  gtk_container_add (GTK_CONTAINER (notebook), vbox_thread);

  hbox_high_thread = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_high_thread);
  gtk_widget_show (hbox_high_thread);
  gtk_box_pack_start (GTK_BOX (vbox_thread), hbox_high_thread, TRUE, TRUE, 0);

  label_thr_void1 = gtk_label_new ("");
  gtk_widget_ref (label_thr_void1);
  gtk_widget_show (label_thr_void1);
  gtk_box_pack_start (GTK_BOX (hbox_high_thread), label_thr_void1, TRUE, TRUE, 0);

  vbox_hight_thread = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_hight_thread);
  gtk_widget_show (vbox_hight_thread);
  gtk_box_pack_start (GTK_BOX (hbox_high_thread), vbox_hight_thread, FALSE, FALSE, 0);

  label_thr_void3 = gtk_label_new ("");
  gtk_widget_ref (label_thr_void3);
  gtk_widget_show (label_thr_void3);
  gtk_box_pack_start (GTK_BOX (vbox_hight_thread), label_thr_void3, TRUE, TRUE, 0);

  checkbutton_thread = gtk_check_button_new_with_label ("Select thread");
  gtk_widget_ref (checkbutton_thread);
  gtk_widget_show (checkbutton_thread);
  gtk_box_pack_start (GTK_BOX (vbox_hight_thread), checkbutton_thread, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_thread), FALSE);

  label_thr_void4 = gtk_label_new ("");
  gtk_widget_ref (label_thr_void4);
  gtk_widget_show (label_thr_void4);
  gtk_box_pack_start (GTK_BOX (vbox_hight_thread), label_thr_void4, TRUE, TRUE, 0);

  label_thr_void2 = gtk_label_new ("");
  gtk_widget_ref (label_thr_void2);
  gtk_widget_show (label_thr_void2);
  gtk_box_pack_start (GTK_BOX (hbox_high_thread), label_thr_void2, TRUE, TRUE, 0);

  label_explain_thread = gtk_label_new ("Enter the commands as following:\n+ Use a ; to separate two numbers of thread\n+ Use a-b to select all the threads bewteen a and b");
  gtk_widget_ref (label_explain_thread);
  gtk_widget_show (label_explain_thread);
  gtk_box_pack_start (GTK_BOX (vbox_thread), label_explain_thread, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label_explain_thread), GTK_JUSTIFY_LEFT);

  scrolledwindow_thread = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow_thread);
  gtk_widget_show (scrolledwindow_thread);
  gtk_box_pack_start (GTK_BOX (vbox_thread), scrolledwindow_thread, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow_thread), 50);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_thread), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  text_thread = gtk_text_new (NULL, NULL);
  gtk_widget_ref (text_thread);
  gtk_widget_show (text_thread);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_thread), text_thread);
  gtk_text_set_editable (GTK_TEXT (text_thread), TRUE);

  label_thread = gtk_label_new ("thread");
  gtk_widget_ref (label_thread);
  gtk_widget_show (label_thread);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 2), label_thread);


  /* vbox for the time selection */
  vbox_time = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_time);
  gtk_widget_show (vbox_time);
  gtk_container_add (GTK_CONTAINER (notebook), vbox_time);

  hbox_high_time = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_high_time);
  gtk_widget_show (hbox_high_time);
  gtk_box_pack_start (GTK_BOX (vbox_time), hbox_high_time, TRUE, TRUE, 0);

  label_time_void1 = gtk_label_new ("");
  gtk_widget_ref (label_time_void1);
  gtk_widget_show (label_time_void1);
  gtk_box_pack_start (GTK_BOX (hbox_high_time), label_time_void1, TRUE, TRUE, 0);

  vbox_high_time = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_high_time);
  gtk_widget_show (vbox_high_time);
  gtk_box_pack_start (GTK_BOX (hbox_high_time), vbox_high_time, FALSE, FALSE, 0);

  label_time_void3 = gtk_label_new ("");
  gtk_widget_ref (label_time_void3);
  gtk_widget_show (label_time_void3);
  gtk_box_pack_start (GTK_BOX (vbox_high_time), label_time_void3, TRUE, TRUE, 0);

  checkbutton_time = gtk_check_button_new_with_label ("Select time");
  gtk_widget_ref (checkbutton_time);
  gtk_widget_show (checkbutton_time);
  gtk_box_pack_start (GTK_BOX (vbox_high_time), checkbutton_time, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_time), FALSE);

  label_time_void4 = gtk_label_new ("");
  gtk_widget_ref (label_time_void4);
  gtk_widget_show (label_time_void4);
  gtk_box_pack_start (GTK_BOX (vbox_high_time), label_time_void4, TRUE, TRUE, 0);

  label_time_void2 = gtk_label_new ("");
  gtk_widget_ref (label_time_void2);
  gtk_widget_show (label_time_void2);
  gtk_box_pack_start (GTK_BOX (hbox_high_time), label_time_void2, TRUE, TRUE, 0);

  label_explain_time = gtk_label_new ("Enter the commands as following:\n+ Use a ; to separate two dates\n+ Use a-b to select all the events between a and b.");
  gtk_widget_ref (label_explain_time);
  gtk_widget_show (label_explain_time);
  gtk_box_pack_start (GTK_BOX (vbox_time), label_explain_time, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label_explain_time), GTK_JUSTIFY_LEFT);

  scrolledwindow_time = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow_time);
  gtk_widget_show (scrolledwindow_time);
  gtk_box_pack_start (GTK_BOX (vbox_time), scrolledwindow_time, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow_time), 50);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_time), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  text_time = gtk_text_new (NULL, NULL);
  gtk_widget_ref (text_time);
  gtk_widget_show (text_time);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_time), text_time);
  gtk_text_set_editable (GTK_TEXT (text_time), TRUE);

  label_time = gtk_label_new ("time");
  gtk_widget_ref (label_time);
  gtk_widget_show (label_time);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 3), label_time);


  /* vbox for the event choices */
  vbox_event = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_event);
  gtk_widget_show (vbox_event);
  gtk_container_add (GTK_CONTAINER (notebook), vbox_event);

  hbuttonbox_events = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox_events);
  gtk_widget_show (hbuttonbox_events);
  gtk_box_pack_start (GTK_BOX (vbox_event), hbuttonbox_events, TRUE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox_events), GTK_BUTTONBOX_SPREAD);

  button_all_events = gtk_button_new_with_label ("ALL EVENTS");
  gtk_widget_ref (button_all_events);
  gtk_widget_show (button_all_events);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_events), button_all_events);
  gtk_signal_connect(GTK_OBJECT(button_all_events), "clicked",
		     GTK_SIGNAL_FUNC(event_all_callback), NULL);

  button_add_event = gtk_button_new_with_label ("ADD ONE");
  gtk_widget_ref (button_add_event);
  gtk_widget_show (button_add_event);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_events), button_add_event);
  gtk_signal_connect(GTK_OBJECT(button_add_event), "clicked",
		     GTK_SIGNAL_FUNC(event_add_one_callback), NULL);


  button_delete_event = gtk_button_new_with_label ("DELETE ONE");
  gtk_widget_ref (button_delete_event);
  gtk_widget_show (button_delete_event);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_events), button_delete_event);
  gtk_signal_connect(GTK_OBJECT(button_delete_event), "clicked",
		     GTK_SIGNAL_FUNC(event_del_one_callback), NULL);  

  button_delete_all_event = gtk_button_new_with_label ("DELETE ALL");
  gtk_widget_ref (button_delete_all_event);
  gtk_widget_show (button_delete_all_event);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_events), button_delete_all_event);
  gtk_signal_connect(GTK_OBJECT(button_delete_all_event), "clicked",
		     GTK_SIGNAL_FUNC(event_del_all_callback), NULL);


  hbox_event = gtk_hbox_new (TRUE, 5);
  gtk_widget_ref (hbox_event);
  gtk_widget_show (hbox_event);
  gtk_box_pack_start (GTK_BOX (vbox_event), hbox_event, TRUE, TRUE, 0);

  combo_events = gtk_combo_new ();
  gtk_widget_ref (combo_events);
  gtk_widget_show (combo_events);
  gtk_box_pack_start (GTK_BOX (hbox_event), combo_events, TRUE, FALSE, 0);

  entry_events = GTK_COMBO (combo_events)->entry;
  gtk_widget_ref (entry_events);
  gtk_widget_show (entry_events);

  scrolledwindow_events = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow_events);
  gtk_widget_show (scrolledwindow_events);
  gtk_box_pack_start (GTK_BOX (hbox_event), scrolledwindow_events, TRUE, TRUE, 15);

  clist_event = gtk_clist_new (1);
  gtk_widget_ref (clist_event);
  gtk_widget_show (clist_event);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_events), clist_event);
  gtk_clist_set_column_width (GTK_CLIST (clist_event), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist_event));
  gtk_clist_set_auto_sort(GTK_CLIST (clist_event), TRUE);
  gtk_signal_connect(GTK_OBJECT(clist_event), "select_row", GTK_SIGNAL_FUNC(event_select_callback), NULL);
  gtk_signal_connect(GTK_OBJECT(clist_event), "unselect_row", GTK_SIGNAL_FUNC(event_unselect_callback), NULL);

  label_activ_events = gtk_label_new ("Activated events");
  gtk_widget_ref (label_activ_events);
  gtk_widget_show (label_activ_events);
  gtk_clist_set_column_widget (GTK_CLIST (clist_event), 0, label_activ_events);

  label_event = gtk_label_new ("event");
  gtk_widget_ref (label_event);
  gtk_widget_show (label_event);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 4), label_event);


  /* vbox for the --function */
  vbox_function = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_function);
  gtk_widget_show (vbox_function);
  gtk_container_add (GTK_CONTAINER (notebook), vbox_function);

  hbuttonbox_function = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox_function);
  gtk_widget_show (hbuttonbox_function);
  gtk_box_pack_start (GTK_BOX (vbox_function), hbuttonbox_function, TRUE, FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox_function), GTK_BUTTONBOX_SPREAD);

  button_all_functions = gtk_button_new_with_label ("ALL FUNCTIONS");
  gtk_widget_ref (button_all_functions);
  gtk_widget_show (button_all_functions);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_function), button_all_functions);
  gtk_signal_connect(GTK_OBJECT(button_all_functions), "clicked",
		     GTK_SIGNAL_FUNC(function_all_callback), NULL);

  button_add_function = gtk_button_new_with_label ("ADD ONE");
  gtk_widget_ref (button_add_function);
  gtk_widget_show (button_add_function);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_function), button_add_function);
  gtk_signal_connect(GTK_OBJECT(button_add_function), "clicked",
		     GTK_SIGNAL_FUNC(function_add_one_callback), NULL);

  button_delete_function = gtk_button_new_with_label ("DELETE ONE");
  gtk_widget_ref (button_delete_function);
  gtk_widget_show (button_delete_function);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_function), button_delete_function);
  gtk_signal_connect(GTK_OBJECT(button_delete_function), "clicked",
		     GTK_SIGNAL_FUNC(function_del_one_callback), NULL);

  button_delete_all_function = gtk_button_new_with_label ("DELETE ALL");
  gtk_widget_ref (button_delete_all_function);
  gtk_widget_show (button_delete_all_function);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_function), button_delete_all_function);
  gtk_signal_connect(GTK_OBJECT(button_delete_all_function), "clicked",
		     GTK_SIGNAL_FUNC(function_del_all_callback), NULL);

  hbox_function = gtk_hbox_new (TRUE, 5);
  gtk_widget_ref (hbox_function);
  gtk_widget_show (hbox_function);
  gtk_box_pack_start (GTK_BOX (vbox_function), hbox_function, TRUE, TRUE, 0);

  combo_function = gtk_combo_new ();
  gtk_widget_ref (combo_function);
  gtk_widget_show (combo_function);
  gtk_box_pack_start (GTK_BOX (hbox_function), combo_function, TRUE, FALSE, 0);

  entry_function = GTK_COMBO (combo_function)->entry;
  gtk_widget_ref (entry_function);
  gtk_widget_show (entry_function);

  scrolledwindow_function = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow_function);
  gtk_widget_show (scrolledwindow_function);
  gtk_box_pack_start (GTK_BOX (hbox_function), scrolledwindow_function, TRUE, TRUE, 15);

  clist_function = gtk_clist_new (1);
  gtk_widget_ref (clist_function);
  gtk_widget_show (clist_function);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_function), clist_function);
  gtk_clist_set_column_width (GTK_CLIST (clist_function), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist_function));
  gtk_clist_set_auto_sort(GTK_CLIST (clist_function), TRUE);
  gtk_signal_connect(GTK_OBJECT(clist_function), "select_row", GTK_SIGNAL_FUNC(function_select_callback), NULL);
  gtk_signal_connect(GTK_OBJECT(clist_function), "unselect_row", GTK_SIGNAL_FUNC(function_unselect_callback), NULL);

  label_activ_function = gtk_label_new ("Activated functions");
  gtk_widget_ref (label_activ_function);
  gtk_widget_show (label_activ_function);
  gtk_clist_set_column_widget (GTK_CLIST (clist_function), 0, label_activ_function);

  label_function = gtk_label_new ("function");
  gtk_widget_ref (label_function);
  gtk_widget_show (label_function);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 5), label_function);


  /* vbox for the evnum selection */
  vbox_evnum = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_evnum);
  gtk_widget_show (vbox_evnum);
  gtk_container_add (GTK_CONTAINER (notebook), vbox_evnum);

  hbox_high_evnum = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_high_evnum);
  gtk_widget_show (hbox_high_evnum);
  gtk_box_pack_start (GTK_BOX (vbox_evnum), hbox_high_evnum, TRUE, TRUE, 0);

  label_evnum_void3 = gtk_label_new ("");
  gtk_widget_ref (label_evnum_void3);
  gtk_widget_show (label_evnum_void3);
  gtk_box_pack_start (GTK_BOX (hbox_high_evnum), label_evnum_void3, TRUE, TRUE, 0);

  vbox_high_evnum = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_high_evnum);
  gtk_widget_show (vbox_high_evnum);
  gtk_box_pack_start (GTK_BOX (hbox_high_evnum), vbox_high_evnum, FALSE, FALSE, 0);

  label_evnum_void1 = gtk_label_new ("");
  gtk_widget_ref (label_evnum_void1);
  gtk_widget_show (label_evnum_void1);
  gtk_box_pack_start (GTK_BOX (vbox_high_evnum), label_evnum_void1, TRUE, TRUE, 0);

  checkbutton_evnum = gtk_check_button_new_with_label ("Select evnum");
  gtk_widget_ref (checkbutton_evnum);
  gtk_widget_show (checkbutton_evnum);
  gtk_box_pack_start (GTK_BOX (vbox_high_evnum), checkbutton_evnum, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_evnum), FALSE);

  label_evnum_void2 = gtk_label_new ("");
  gtk_widget_ref (label_evnum_void2);
  gtk_widget_show (label_evnum_void2);
  gtk_box_pack_start (GTK_BOX (vbox_high_evnum), label_evnum_void2, TRUE, TRUE, 0);

  label_evnum_void4 = gtk_label_new ("");
  gtk_widget_ref (label_evnum_void4);
  gtk_widget_show (label_evnum_void4);
  gtk_box_pack_start (GTK_BOX (hbox_high_evnum), label_evnum_void4, TRUE, TRUE, 0);

  label_explain_evenum = gtk_label_new ("Enter the commands as following:\n+ Use a ; to separate two different numbers of event\n+ Use a-b to select all the events between number a and number b.");
  gtk_widget_ref (label_explain_evenum);
  gtk_widget_show (label_explain_evenum);
  gtk_box_pack_start (GTK_BOX (vbox_evnum), label_explain_evenum, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label_explain_evenum), GTK_JUSTIFY_LEFT);

  scrolledwindow_evnum = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow_evnum);
  gtk_widget_show (scrolledwindow_evnum);
  gtk_box_pack_start (GTK_BOX (vbox_evnum), scrolledwindow_evnum, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow_evnum), 50);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_evnum), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  text_evnum = gtk_text_new (NULL, NULL);
  gtk_widget_ref (text_evnum);
  gtk_widget_show (text_evnum);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_evnum), text_evnum);
  gtk_text_set_editable (GTK_TEXT (text_evnum), TRUE);

  label_evnum = gtk_label_new ("evnum");
  gtk_widget_ref (label_evnum);
  gtk_widget_show (label_evnum);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 6), label_evnum);


  /* vbox for the begin/end selection */
  vbox_begin = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_begin);
  gtk_widget_show (vbox_begin);
  gtk_container_add (GTK_CONTAINER (notebook), vbox_begin);

  label_explain_begin = gtk_label_new ("\nSorry, but you have to enter the commands as in the console mode.");
  gtk_widget_ref (label_explain_begin);
  gtk_widget_show (label_explain_begin);
  gtk_box_pack_start (GTK_BOX (vbox_begin), label_explain_begin, FALSE, FALSE, 0);

  scrolledwindow_begin = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow_begin);
  gtk_widget_show (scrolledwindow_begin);
  gtk_box_pack_start (GTK_BOX (vbox_begin), scrolledwindow_begin, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow_begin), 50);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_begin), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  text_begin = gtk_text_new (NULL, NULL);
  gtk_widget_ref (text_begin);
  gtk_widget_show (text_begin);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_begin), text_begin);
  gtk_text_set_editable (GTK_TEXT (text_begin), TRUE);

  label_begin_end = gtk_label_new ("begin/end");
  gtk_widget_ref (label_begin_end);
  gtk_widget_show (label_begin_end);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 7), label_begin_end);

  hbuttonbox_reset = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox_reset);
  gtk_widget_show (hbuttonbox_reset);
  gtk_box_pack_start (GTK_BOX (vbox), hbuttonbox_reset, FALSE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox_reset), GTK_BUTTONBOX_END);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox_reset), 85, 0);

  button_reset = gtk_button_new_with_label ("Reset");
  gtk_widget_ref (button_reset);
  gtk_widget_show (button_reset);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_reset), button_reset);
  gtk_signal_connect(GTK_OBJECT(button_reset), "clicked",
		     GTK_SIGNAL_FUNC(reset_callback), NULL);

}
/* end!!! of create_window */

/*  file selector */
static void filesel_destroy( GtkWidget *widget, gpointer   data )
{
  gtk_widget_destroy(widget);
}

static void cpu_list_init(char *name);

static void file_ok_sel_callback(GtkWidget *w, GtkFileSelection *fs)
{
  gchar *name;
  name = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));
  statusbar_set_current_supertrace(name);
  cpu_list_init(name);
  strcpy(supertrace, name);
  gtk_widget_destroy(fileselection);
}


void create_fileselection(void)
{
  
  GtkWidget *ok_button;
  GtkWidget *cancel_button;

  fileselection = gtk_file_selection_new ("Select File");
  gtk_object_set_data (GTK_OBJECT (fileselection), "fileselection", fileselection);
  gtk_container_set_border_width (GTK_CONTAINER (fileselection), 10);
  gtk_signal_connect (GTK_OBJECT (fileselection), "destroy",
		      (GtkSignalFunc) filesel_destroy, &fileselection);

  ok_button = GTK_FILE_SELECTION (fileselection)->ok_button;
  gtk_object_set_data (GTK_OBJECT (fileselection), "ok_button", ok_button);
  gtk_widget_show (ok_button);
  gtk_signal_connect (GTK_OBJECT (ok_button),
		      "clicked", (GtkSignalFunc) file_ok_sel_callback, fileselection );

  cancel_button = GTK_FILE_SELECTION (fileselection)->cancel_button;
  gtk_object_set_data (GTK_OBJECT (fileselection), "cancel_button", cancel_button);
  gtk_widget_show (cancel_button);
  gtk_signal_connect_object (GTK_OBJECT(cancel_button), "clicked", (GtkSignalFunc) filesel_destroy,
			     GTK_OBJECT (fileselection));
  
  gtk_widget_show(fileselection);
}


/* initialize the list of the events */
static void function_events_list_init()
{
  int i;
  for(i = 0; code_table[i].name != NULL; i++) {
    if ((code_table[i].code <= 0x400) || (code_table[i].code > 0x8000))
      combo_events_items = g_list_append(combo_events_items, code_table[i].name);
    else if ((code_table[i].code & 0x100) == 0) {
      code_table[i].name[strlen(code_table[i].name) - 6] = '\0';
      combo_function_items = g_list_append(combo_function_items, code_table[i].name);
    }
  }
  for(i = 0; fkt_code_table[i].name != NULL; i++) {
    int s = strlen(fkt_code_table[i].name);
    if (strcmp(fkt_code_table[i].name + (s-6), "_entry") == 0) {
      combo_function_items = g_list_append(combo_function_items, fkt_code_table[i].name);
    } else if (strcmp(fkt_code_table[i].name + (s-5), "_exit") != 0)
      combo_events_items = g_list_append(combo_events_items, fkt_code_table[i].name);
  }
  for(i = 0; i < FKT_NTRAPS; i++) 
    combo_events_items = g_list_append(combo_events_items, fkt_traps[i]);
  for(i = 0; i < FKT_NSYSCALLS; i++) 
    combo_events_items = g_list_append(combo_events_items, fkt_syscalls[i]);

  gtk_combo_set_popdown_strings(GTK_COMBO(combo_events), combo_events_items);
  gtk_combo_set_popdown_strings(GTK_COMBO(combo_function), combo_function_items);
}

/* initialize the cpu list */
void cpu_list_init(char *supertracename)
{
  FILE *f;
  int pid;
  int n;
  int i;
  char header[200];
  gtk_clist_clear(GTK_CLIST(clist_proc));
  g_list_free(combo_proc_items);
  combo_proc_items = NULL;
  if((f = fopen(supertracename, "r")) == NULL) {
    exit(1);
  }
  fread(&header, sizeof(short int) + 2*sizeof(unsigned long long int)+ sizeof(double) + sizeof(int), 1, f);
  fread(&n, sizeof(int), 1, f);
  for(i = 0; i < n; i++) {
    char *s;
    s = (char *) malloc(12*sizeof(char));
    fread(&pid, sizeof(int), 1, f);
    sprintf(s,"%d",pid);
    combo_proc_items = g_list_append(combo_proc_items, s);
  }
  gtk_combo_set_popdown_strings(GTK_COMBO(combo_proc), combo_proc_items);
  fclose(f);
}



/* creates the window and initialze the events list */
void options_init(GtkWidget *main_vbox)
{
  create_window(main_vbox);
  function_events_list_init();
}
