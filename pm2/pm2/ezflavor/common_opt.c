
#include <gtk/gtk.h>

#include "common_opt.h"
#include "menu.h"

static char *well_known_options[] = {
  "debug",
  "gdb",
  "opt",
  "profile",
  NULL
};

typedef enum {
  OPT_AS_IS,
  OPT_SET,
  OPT_CLEAR
} opt_state_t;

typedef struct {
  char *name;
  opt_state_t state;
  GList *widgets;
} opt_t;

static gint panel_displayed = 0;

static GtkWidget *win = NULL;
static GtkWidget *main_vbox;

static GList *options;

static gint opt_cmp(gconstpointer a, gconstpointer b)
{
  return strcmp(((opt_t *)a)->name, (char *)b);
}

void common_opt_register_option(char *name,
				GtkWidget *button)
{
  GList *ptr;
  opt_t *opt;

  ptr = g_list_find_custom(options, (gpointer)name, opt_cmp);

  if(!ptr)
    return;

  opt = (opt_t *)ptr->data;
  opt->widgets = g_list_append(opt->widgets, (gpointer)button);
}

static gint delete_event(GtkWidget *widget,
			 GdkEvent  *event,
			 gpointer   data )
{
  common_opt_hide_panel();
  return TRUE;
}

static void destroy(GtkWidget *widget,
		    gpointer   data )
{
  gtk_main_quit();
}

static GList *create_option_list(void)
{
  GList *l = NULL;
  opt_t *ptr_opt;
  unsigned i;

  for(i=0; well_known_options[i] != NULL; i++) {
    ptr_opt = (opt_t *)g_malloc(sizeof(opt_t));
    ptr_opt->name = well_known_options[i];
    ptr_opt->state = OPT_AS_IS;
    ptr_opt->widgets = NULL;
    l = g_list_append(l, (gpointer)ptr_opt);
  }

  return l;
}

static void as_is_callback(GtkWidget *widget,
			   gpointer data)
{
  opt_t *opt = (opt_t *)data;

  if(GTK_TOGGLE_BUTTON(widget)->active) {
    opt->state = OPT_AS_IS;
  }
}

static void set_callback(GtkWidget *widget,
			 gpointer data)
{
  opt_t *opt = (opt_t *)data;

  if(GTK_TOGGLE_BUTTON(widget)->active) {
    opt->state = OPT_SET;
  }
}

static void clear_callback(GtkWidget *widget,
			   gpointer data)
{
  opt_t *opt = (opt_t *)data;

  if(GTK_TOGGLE_BUTTON(widget)->active) {
    opt->state = OPT_CLEAR;
  }
}

static void create_subframe(GtkWidget *box, opt_t *opt)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button = NULL;

  frame = gtk_frame_new(opt->name);
  gtk_container_set_border_width (GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER(vbox), 5);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(vbox);

  // As is
  button = gtk_radio_button_new_with_label(NULL, "Let as is");
  gtk_widget_show(button);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     GTK_SIGNAL_FUNC(as_is_callback), (gpointer)opt);

  // Set
  button = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(button)),
					   "Set");
  gtk_widget_show(button);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     GTK_SIGNAL_FUNC(set_callback), (gpointer)opt);

  // Unset
  button = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(button)),
					   "Unset");
  gtk_widget_show(button);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     GTK_SIGNAL_FUNC(clear_callback), (gpointer)opt);
}

static void create_all_subframes(GtkWidget *vbox)
{
  GList *ptr;

  for(ptr = g_list_first(options);
      ptr != NULL;
      ptr = g_list_next(ptr)) {
    create_subframe(vbox, (opt_t *)ptr->data);
  }
}

static void apply_to_all(GtkWidget *widget, gpointer data)
{
  GList *lopt;
  opt_t *opt;

  for(lopt = g_list_first(options);
      lopt != NULL;
      lopt = g_list_next(lopt)) {

    opt = (opt_t *)lopt->data;

    if(opt->state != OPT_AS_IS) {
      GList *ptr;

      for(ptr = g_list_first(opt->widgets);
	  ptr != NULL;
	  ptr = g_list_next(ptr)) {

	if(opt->state == OPT_SET)
	  gtk_toggle_button_set_active((GtkToggleButton *)ptr->data, TRUE);
	else
	  gtk_toggle_button_set_active((GtkToggleButton *)ptr->data, FALSE);
      }
    }
  }
}

static void create_window(void)
{
  GtkWidget *button;

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (win), "Common options");

  gtk_signal_connect(GTK_OBJECT(win), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), NULL);

  gtk_signal_connect(GTK_OBJECT(win), "destroy",
		     GTK_SIGNAL_FUNC(destroy), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(win), 10);

  main_vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_border_width(GTK_CONTAINER(main_vbox), 0);
  gtk_container_add(GTK_CONTAINER(win), main_vbox);
  gtk_widget_show(main_vbox);

  create_all_subframes(main_vbox);

  button = gtk_button_new_with_label("Apply to all");
  gtk_container_border_width(GTK_CONTAINER(button), 5);
  gtk_box_pack_start(GTK_BOX(main_vbox), button, FALSE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(apply_to_all),
		     NULL);
  gtk_widget_show(button);

  panel_displayed = 0;
}

void common_opt_init(void)
{
  options = create_option_list();

  create_window();
}

void common_opt_display_panel(void)
{
  gtk_widget_show(win);
  panel_displayed = 1;
  menu_update_common_opt(panel_displayed);
}

void common_opt_hide_panel(void)
{
  gtk_widget_hide(win);
  panel_displayed = 0;
  menu_update_common_opt(panel_displayed);
}
