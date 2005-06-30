#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>


#include <string.h>
#include "options.h"
#include "shell.h"

static GtkWidget *print;
static GtkWidget *radiobutton_print_cpu;
static GtkWidget *radiobutton_print_thread;
static GtkWidget *entry_print;


static void print_destroy(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(print);
}

static void print_ok_callback(GtkWidget *widget, gpointer data)
{
  gchar *psoutput;
  char s[1200];
  int pid;
  psoutput = gtk_entry_get_text(GTK_ENTRY(entry_print));
  strcpy(s, "sigmundps ");
  options_to_string(s + 10);
  strcat(s, " -o ");
  strcat(s, psoutput);
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton_print_cpu))) 
    strcat(s, " --print-cpu");
  else strcat(s, " --print-thread");
  //  printf("Exec : %s\n", s);
  pid = exec_single_cmd_fmt(NULL,s);
  exec_wait(pid);
  gtk_widget_destroy(print);
}

void show_print (void)
{
  GtkWidget *vbox_print;
  GSList *vbox_print_group = NULL;
  GtkWidget *label_print_1;
  GtkWidget *hbox_print;
  GtkWidget *label_print_2;
  GtkWidget *hbuttonbox_print;
  GtkWidget *button_print_ok;
  GtkWidget *button_print_cancel;

  print = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (print), "print", print);
  gtk_window_set_title (GTK_WINDOW (print), "Print");
  gtk_signal_connect(GTK_OBJECT(print), "destroy", 
  		     GTK_SIGNAL_FUNC(print_destroy), &print);

  vbox_print = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_print);
  gtk_widget_show (vbox_print);
  gtk_container_add (GTK_CONTAINER (print), vbox_print);

  radiobutton_print_cpu = gtk_radio_button_new_with_label (vbox_print_group, "Cpu view");
  vbox_print_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_print_cpu));
  gtk_widget_ref (radiobutton_print_cpu);
  gtk_widget_show (radiobutton_print_cpu);
  gtk_box_pack_start (GTK_BOX (vbox_print), radiobutton_print_cpu, FALSE, FALSE, 0);

  radiobutton_print_thread = gtk_radio_button_new_with_label (vbox_print_group, "Thread view");
  vbox_print_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_print_thread));
  gtk_widget_ref (radiobutton_print_thread);
  gtk_widget_show (radiobutton_print_thread);
  gtk_box_pack_start (GTK_BOX (vbox_print), radiobutton_print_thread, FALSE, FALSE, 0);

  label_print_1 = gtk_label_new ("Please enter the name of the output postscript");
  gtk_widget_ref (label_print_1);
  gtk_widget_show (label_print_1);
  gtk_box_pack_start (GTK_BOX (vbox_print), label_print_1, FALSE, FALSE, 0);

  hbox_print = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_print);
  gtk_widget_show (hbox_print);
  gtk_box_pack_start (GTK_BOX (vbox_print), hbox_print, TRUE, TRUE, 0);

  label_print_2 = gtk_label_new ("File name: ");
  gtk_widget_ref (label_print_2);
  gtk_widget_show (label_print_2);
  gtk_box_pack_start (GTK_BOX (hbox_print), label_print_2, FALSE, FALSE, 0);

  entry_print = gtk_entry_new ();
  gtk_widget_ref (entry_print);
  gtk_widget_show (entry_print);
  gtk_box_pack_start (GTK_BOX (hbox_print), entry_print, TRUE, TRUE, 0);

  hbuttonbox_print = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox_print);
  gtk_widget_show (hbuttonbox_print);
  gtk_box_pack_end (GTK_BOX (vbox_print), hbuttonbox_print, FALSE, FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox_print), GTK_BUTTONBOX_END);

  button_print_ok = gtk_button_new_with_label ("OK");
  gtk_widget_ref (button_print_ok);
  gtk_widget_show (button_print_ok);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_print), button_print_ok);
  GTK_WIDGET_SET_FLAGS (button_print_ok, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button_print_ok), "clicked",
		     GTK_SIGNAL_FUNC(print_ok_callback), NULL);

  button_print_cancel = gtk_button_new_with_label ("Cancel");
  gtk_widget_ref (button_print_cancel);
  gtk_widget_show (button_print_cancel);
  gtk_container_add (GTK_CONTAINER (hbuttonbox_print), button_print_cancel);
  GTK_WIDGET_SET_FLAGS (button_print_cancel, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button_print_cancel), "clicked",
		     GTK_SIGNAL_FUNC(print_destroy), NULL);
  gtk_widget_show(print);

}
