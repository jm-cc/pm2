
#include <stdio.h>
#include <unistd.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "main.h"
#include "module.h"
#include "shell.h"
#include "parser.h"
#include "str_list.h"
#include "trace.h"
#include "flavor.h"
#include "intro.h"

static GtkWidget *notebook;
static GtkWidget *module_frame, *general_frame;
static GtkWidget *select_button;
static GtkWidget *deselect_button;
static GtkWidget *builddir_entry, *extension_entry;

static GList *mod_names = NULL;

typedef struct {
  char *name;
  GtkWidget *page;
  GList *options;
} module_t;

static GList *the_modules = NULL;
static module_t *common_module;

static void module_rescan(void)
{
  intro_begin_step("Getting list of modules...");

  string_list_destroy(&mod_names);

  parser_start_cmd("pm2-module modules");

  mod_names = string_list_from_parser();

  parser_stop();

  intro_end_step();
}

static GList *module_list(void)
{
  if(!mod_names)
    module_rescan();

  return mod_names;
}

static void option_set_tooltip_msg(GtkWidget *widget, char *module, char *option)
{
  token_t tok;
  char arg[1024];
  int pid, fd;

  sprintf(arg, "echo \\\"`pm2-module option=%s --module=%s --help`\\\"",
	  option, module);

  pid = exec_single_cmd_args(&fd, "sh", "-c", arg, NULL);

  parser_start(fd);

  tok = parser_next_token();

  if(tok != STRING_TOKEN)
    parser_error();

  gtk_tooltips_set_tip(the_tooltip, widget, parser_token_image(), NULL);

  parser_stop();

  exec_wait(pid);
}

static void module_set_tooltip_msg(GtkWidget *widget, char *module)
{
  token_t tok;
  char arg[1024];
  int pid, fd;

  sprintf(arg, "echo \\\"`pm2-module module=%s --help`\\\"",
	  module);

  pid = exec_single_cmd_args(&fd, "sh", "-c", arg, NULL);

  parser_start(fd);

  tok = parser_next_token();

  if(tok != STRING_TOKEN)
    parser_error();

  gtk_tooltips_set_tip(the_tooltip, widget, parser_token_image(), NULL);

  parser_stop();

  exec_wait(pid);
}

static void attach_specific_data(GtkWidget *widget,
				 char *module,
				 char *option,
				 GList **list)
{
  char buf[1024], *str;

  // Insert the toggle button widget in the list
  *list = g_list_append(*list, (gpointer)widget);

  // Graphical tooltips using the help message for the option
  if(tips_enabled)
    option_set_tooltip_msg(widget, module, option);

  sprintf(buf, "--%s=%s", module, option);
  str = string_new(buf);

  gtk_object_set_user_data(GTK_OBJECT(widget), (gpointer)str);
}

static void button_callback(GtkWidget *widget,
			    gpointer data)
{
  if(!destroy_phase) {

    flavor_mark_modified();

#ifdef DEBUG
    if(GTK_TOGGLE_BUTTON(widget)->active)
      g_print ("The %s (%s) option was enabled\n",
	       (char *)gtk_object_get_user_data(GTK_OBJECT(widget)),
	       (char *)data);
    else
      g_print ("The %s (%s) option was disabled\n",
	       (char *)gtk_object_get_user_data(GTK_OBJECT(widget)),
	       (char *)data);
#endif
  }
}

static void settings_changed(GtkWidget *widget,
			     gpointer data)
{
  if(!destroy_phase)
    flavor_mark_modified();
}

static void add_exclusive_option_set(char *module,
				     char *set,
				     GtkWidget *vbox,
				     GList **list)
{
  GList *options, *ptr;
  GtkWidget *button = NULL;

  parser_start_cmd("pm2-module options --module=%s --get-excl=%s", module, set);

  options = string_list_from_parser();

  parser_stop();

  for(ptr = g_list_first(options);
      ptr != NULL;
      ptr = g_list_next(ptr)) {

    if(button == NULL) { /* first option */
      button = gtk_radio_button_new_with_label(NULL, (char *)ptr->data);
    } else {
      button =
	gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(button)),
					(char *)ptr->data);
    }
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       GTK_SIGNAL_FUNC(button_callback), (gpointer)"exclusive");

    attach_specific_data(button, module, (char *)ptr->data, list);

    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  }

  string_list_destroy(&options);
}

static void add_inclusive_option_set(char *module,
				     char *set,
				     GtkWidget *vbox,
				     GList **list)
{
  GList *options, *ptr;
  GtkWidget *button = NULL;

  parser_start_cmd("pm2-module options --module=%s --get-incl=%s", module, set);

  options = string_list_from_parser();

  parser_stop();

  for(ptr = g_list_first(options);
      ptr != NULL;
      ptr = g_list_next(ptr)) {

    button = gtk_check_button_new_with_label((char *)ptr->data);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       GTK_SIGNAL_FUNC(button_callback), (gpointer)"inclusive");

    attach_specific_data(button, module, (char *)ptr->data, list);

    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  }

  string_list_destroy(&options);
}

static void add_exclusive_options(char *module,
				  GtkWidget *vbox,
				  GList **list)
{
  GtkWidget *sep;
  GList *sets, *ptr;

  parser_start_cmd("pm2-module options --module=%s --get-excl-sets", module);

  sets = string_list_from_parser();

  parser_stop();

  for(ptr = g_list_first(sets);
      ptr != NULL;
      ptr = g_list_next(ptr)) {

    add_exclusive_option_set(module, (char *)ptr->data, vbox, list);

    sep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, TRUE, 0);
    gtk_widget_show(sep);
  }

  string_list_destroy(&sets);
}

static void add_inclusive_options(char *module,
				  GtkWidget *vbox,
				  GList **list)
{
  GtkWidget *sep;
  GList *sets, *ptr;

  parser_start_cmd("pm2-module options --module=%s --get-incl-sets", module);

  sets = string_list_from_parser();

  parser_stop();

  for(ptr = g_list_first(sets);
      ptr != NULL;
      ptr = g_list_next(ptr)) {

    add_inclusive_option_set(module, (char *)ptr->data, vbox, list);

    if(g_list_next(ptr) != NULL) {
      sep = gtk_hseparator_new();
      gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, TRUE, 0);
      gtk_widget_show(sep);
    }
  }

  string_list_destroy(&sets);
}

/* Should be customized to use either a "notebook" style or a column style... */
static GtkWidget *add_options(char *module, GList **list)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *scrolled_window;

  frame = gtk_frame_new("Module options");
  gtk_container_set_border_width (GTK_CONTAINER(frame), 10);
  gtk_widget_set_usize(frame, 300, 300);
  gtk_widget_show(frame);

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(scrolled_window);
  gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER(vbox), 5);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vbox);
  gtk_widget_show(vbox);

  intro_begin_step("Getting options for module %s...", module);

  add_exclusive_options(module, vbox, list);

  add_inclusive_options(module, vbox, list);

  intro_end_step();

  return frame;
}

static void set_module_info(GtkWidget *widget, char *module)
{
  char buf[1024], *str;

  sprintf(buf, "--modules=%s", module);
  str = string_new(buf);

  gtk_object_set_user_data(GTK_OBJECT(widget), (gpointer)str);
}

static void add_common_options(GtkWidget *box)
{
  GtkWidget *frame;
  GtkWidget *vbox;

  frame = gtk_frame_new("Common options");
  gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER(vbox), 5);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(vbox);
  
  common_module = (module_t *)g_malloc(sizeof(module_t));

  common_module->name = string_new("common");
  common_module->options = NULL;
  common_module->page = frame;

  set_module_info(common_module->page, common_module->name);

  add_inclusive_options(common_module->name, vbox, &common_module->options);
}

static void set_page_select_state(GtkWidget *page, gint selected)
{
  gtk_widget_set_sensitive(page, selected);
  gtk_widget_set_sensitive(gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),
						      page),
			   selected);
}

static void update_select_buttons(gint active_page)
{
   GtkWidget *page;

   page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), active_page);

   if(page->state == GTK_STATE_INSENSITIVE) {
     gtk_widget_set_sensitive(select_button, TRUE);
     gtk_widget_set_sensitive(deselect_button, FALSE);
   } else {
     gtk_widget_set_sensitive(select_button, FALSE);
     gtk_widget_set_sensitive(deselect_button, TRUE);
   }
}

static void select_pressed(GtkWidget *widget,
			   gpointer data)
{
  GtkWidget *page;
  gint num;

  if(!destroy_phase) {
    num = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), num);

    set_page_select_state(page, TRUE);

    update_select_buttons(num);

    flavor_mark_modified();
  }
}

static void deselect_pressed(GtkWidget *widget,
			     gpointer data)
{
  GtkWidget *page;
  gint num;

  if(!destroy_phase) {
    num = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), num);

    set_page_select_state(page, FALSE);

    update_select_buttons(num);

    flavor_mark_modified();
  }
}

static void notebook_callback(GtkWidget *widget,
			      gpointer data,
			      gint num)
{
  if(!destroy_phase)
    update_select_buttons(num);
}

static void module_build_notebook(GtkWidget *box)
{
  GList *ptr;
  GtkWidget *label;
  GtkWidget *vbox, *hbox;
  module_t *cur_mod;

  module_frame = gtk_frame_new("Modules");
  gtk_widget_set_sensitive(module_frame, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER(module_frame), 10);
  gtk_box_pack_start(GTK_BOX(box), module_frame, TRUE, TRUE, 0);
  gtk_widget_show(module_frame);

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(module_frame), vbox);
  gtk_widget_show(vbox);

  notebook = gtk_notebook_new();
  gtk_container_set_border_width (GTK_CONTAINER(notebook), 10);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK(notebook), GTK_POS_TOP);
  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show(notebook);

  for(ptr = g_list_first(module_list());
      ptr != NULL;
      ptr = g_list_next(ptr)) {
    char *module = (char *)ptr->data;

    cur_mod = (module_t *)g_malloc(sizeof(module_t));

    the_modules = g_list_append(the_modules, (gpointer)cur_mod);

    cur_mod->name = module; // Attention !!!!! On pointe sur une autre liste !
    cur_mod->options = NULL;

    cur_mod->page = add_options(module, &cur_mod->options);

    set_module_info(cur_mod->page, module);

    label = gtk_label_new(module);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cur_mod->page, label);

    if(tips_enabled)
      module_set_tooltip_msg(label, module);
  }

  hbox = gtk_hbox_new(TRUE, 10);
  gtk_container_set_border_width (GTK_CONTAINER(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show(hbox);

  select_button = gtk_button_new_with_label("Select");
  gtk_box_pack_start(GTK_BOX(hbox), select_button, FALSE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(select_button), "clicked",
		     GTK_SIGNAL_FUNC(select_pressed),
		     NULL);
  gtk_widget_show(select_button);

  deselect_button = gtk_button_new_with_label("Deselect");
  gtk_box_pack_start(GTK_BOX(hbox), deselect_button, FALSE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(deselect_button), "clicked",
		     GTK_SIGNAL_FUNC(deselect_pressed),
		     NULL);
  gtk_widget_show(deselect_button);

  gtk_signal_connect(GTK_OBJECT(notebook), "switch-page",
		     GTK_SIGNAL_FUNC(notebook_callback), NULL);
}

static void module_build_general_options(GtkWidget *box)
{
  GtkWidget *vbox, *subvbox;
  GtkWidget *label;

  general_frame = gtk_frame_new("General settings");
  gtk_widget_set_sensitive(general_frame, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER(general_frame), 10);
  gtk_box_pack_start(GTK_BOX(box), general_frame, FALSE, TRUE, 0);
  gtk_widget_show(general_frame);

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(general_frame), vbox);
  gtk_widget_show(vbox);

  subvbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER(subvbox), 10);
  gtk_container_add(GTK_CONTAINER(vbox), subvbox);
  gtk_widget_show(subvbox);

  label = gtk_label_new("Build directory");
  gtk_box_pack_start(GTK_BOX(subvbox), label, FALSE, TRUE, 0);
  gtk_widget_show(label);

  builddir_entry = gtk_entry_new();
  gtk_signal_connect(GTK_OBJECT(builddir_entry), "changed",
		     GTK_SIGNAL_FUNC(settings_changed), NULL);
  gtk_box_pack_start(GTK_BOX(subvbox), builddir_entry, FALSE, TRUE, 0);
  gtk_widget_show(builddir_entry);

  subvbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER(subvbox), 10);
  gtk_container_add(GTK_CONTAINER(vbox), subvbox);
  gtk_widget_show(subvbox);

  label = gtk_label_new("File extension");
  gtk_box_pack_start(GTK_BOX(subvbox), label, FALSE, TRUE, 0);
  gtk_widget_show(label);

  extension_entry = gtk_entry_new();
  gtk_signal_connect(GTK_OBJECT(extension_entry), "changed",
		     GTK_SIGNAL_FUNC(settings_changed), NULL);
  gtk_box_pack_start(GTK_BOX(subvbox), extension_entry, FALSE, TRUE, 0);
  gtk_widget_show(extension_entry);

  subvbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER(subvbox), 10);
  gtk_container_add(GTK_CONTAINER(vbox), subvbox);
  gtk_widget_show(subvbox);

  add_common_options(subvbox);
}

static void module_update_module_options(module_t *m)
{
  GList *opt;

  for(opt = g_list_first(m->options);
      opt != NULL;
      opt = g_list_next(opt)) {

    GtkWidget *button = (GtkWidget *)opt->data;

    if(flavor_uses_option((char *)gtk_object_get_user_data(GTK_OBJECT(button)))) {
      gtk_toggle_button_set_active((GtkToggleButton *)button, TRUE);
    } else {
      gtk_toggle_button_set_active((GtkToggleButton *)button, FALSE); 
    }

  }
}

static void module_update_general_settings(void)
{
  gtk_entry_set_text(GTK_ENTRY(builddir_entry), flavor_builddir());

  gtk_entry_set_text(GTK_ENTRY(extension_entry), flavor_extension());

  module_update_module_options(common_module);
}

void module_update_with_current_flavor(void)
{
  GList *mod;

  module_update_general_settings();

  for(mod = g_list_first(the_modules);
      mod != NULL;
      mod = g_list_next(mod)) {

    module_t *m = (module_t *)mod->data;

    if(flavor_uses_module((char *)gtk_object_get_user_data(GTK_OBJECT(m->page)))) {

      set_page_select_state(m->page, TRUE);

      module_update_module_options(m);

    } else {

      set_page_select_state(m->page, FALSE);

    }
  }

  gtk_widget_set_sensitive(module_frame, TRUE);
  gtk_widget_set_sensitive(general_frame, TRUE);

  update_select_buttons(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
}

static void module_save_module_options(module_t *m)
{
  GList *opt;

  for(opt = g_list_first(m->options);
      opt != NULL;
      opt = g_list_next(opt)) {

    GtkWidget *button = (GtkWidget *)opt->data;

    if(gtk_toggle_button_get_active((GtkToggleButton *)button)) {
      flavor_add_option((char *)gtk_object_get_user_data(GTK_OBJECT(button)));
    }
  }
}

static void module_save_general_settings(void)
{
  flavor_set_builddir(gtk_entry_get_text(GTK_ENTRY(builddir_entry)));
  flavor_set_extension(gtk_entry_get_text(GTK_ENTRY(extension_entry)));

  module_save_module_options(common_module);
}

void module_save_to_flavor(void)
{
  GList *mod;

  flavor_reset_contents();

  module_save_general_settings();

  for(mod = g_list_first(the_modules);
      mod != NULL;
      mod = g_list_next(mod)) {

    module_t *m = (module_t *)mod->data;

    if(m->page->state != GTK_STATE_INSENSITIVE) {

      module_save_module_options(m);

      flavor_add_module((char *)gtk_object_get_user_data(GTK_OBJECT(m->page)));

    }
  }
}

void module_init(GtkWidget *leftbox, GtkWidget *rightbox)
{
  module_build_general_options(leftbox);

  module_build_notebook(rightbox);
}

