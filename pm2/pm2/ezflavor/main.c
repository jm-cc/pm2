
#include <gtk/gtk.h>

#include "main.h"
#include "module.h"
#include "flavor.h"
#include "intro.h"

gint destroy_phase = FALSE;

// Customization
gint tips_enabled = TRUE;
gint skip_intro = FALSE;
gint with_sound = FALSE;
static gint show_help = FALSE;

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

int main(int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *top_paned;
  GtkWidget *left_vbox, *right_vbox;

  gtk_init(&argc, &argv);

  parse_options(&argc, argv);

  if(show_help) {
    do_show_help(argv[0]);
    exit(0);
  }

  intro_init();

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), NULL);

  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(destroy), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(window), 10);

  top_paned = gtk_hpaned_new();
  gtk_widget_show(top_paned);
  gtk_container_add(GTK_CONTAINER(window), top_paned);
  gtk_container_set_border_width(GTK_CONTAINER(top_paned), 10);

  left_vbox = gtk_vbox_new(FALSE, 10);
  gtk_paned_add1((GtkPaned *)top_paned, left_vbox);
  gtk_widget_show(left_vbox);

  right_vbox = gtk_vbox_new(FALSE, 10);
  gtk_paned_add2((GtkPaned *)top_paned, right_vbox);
  gtk_widget_show(right_vbox);

  the_tooltip = gtk_tooltips_new();
  gtk_tooltips_set_delay(the_tooltip, 1000); /* 1 seconde */

  flavor_init(left_vbox);

  module_init(left_vbox, right_vbox);

  intro_exit();

  gtk_widget_show(window);

  gtk_main();

  return 0;
}
