
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

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
#include "menu.h"
#include "common_opt.h"

static GtkWidget *notebook;
static GtkWidget *module_frame, *general_frame;
static GtkWidget *select_button;
static GtkWidget *deselect_button;
static GtkWidget *pEntry;

static GList *mod_names = NULL;
static GList *visible_names = NULL;

typedef struct {
  char *name;
  GtkWidget *page;
  GList *options;
} module_t;

typedef struct option_data {

        GtkWidget *widget;
        char str[1];

} option_data_t;

#define _str2data(s) ((struct option_data *)((char *)(s) - (unsigned long)(&((struct option_data *)0)->str)))


static GList *the_modules = NULL;
static GList *the_static_modules = NULL;

char *known_static_modules[] =
{
  "common",
  "appli",
  NULL
};

#if 0 
/* not used ?! */
static gint mod_cmp(gconstpointer a, gconstpointer b)
{
  return strcmp(((module_t *)a)->name, (char *)b);
}

static module_t *find_module(char *name)
{
  GList *ptr;

  ptr = g_list_find_custom(the_modules, (gpointer)name, mod_cmp);

  if(!ptr) {
    return NULL;
  } else {
    return (module_t *)ptr->data;
  }
}
#endif

static void module_rescan(void)
{
  intro_begin_step("Getting list of modules...");

  string_list_destroy(&mod_names);

  /* First build the complete list of modules */
  parser_start_cmd("%s/bin/pm2-module modules", pm2_objroot());

  mod_names = string_list_from_parser();

  parser_stop();

  /* Then build the list of visible modules */
  if(show_all_modules)

    visible_names = mod_names;

  else {

    parser_start_cmd("%s/bin/pm2-module modules --user-only", pm2_objroot());

    visible_names = string_list_from_parser();

    parser_stop();

  }

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

  sprintf(arg, "echo \\\"`%s/bin/pm2-module option=%s --module=%s --help`\\\"",
	  pm2_objroot(), option, module);

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

  sprintf(arg, "echo \\\"`%s/bin/pm2-module module=%s --help`\\\"",
	  pm2_objroot(), module);

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
                                 GtkWidget *text_entry,
				 GList **list)
{
  char buf[1024];
  option_data_t *ptr;
  
  // Insert the toggle button widget in the list
  *list = g_list_append(*list, (gpointer)widget);

  // Graphical tooltips using the help message for the option
  if(tips_enabled)
    option_set_tooltip_msg(widget, module, option);

  sprintf(buf, "--%s=%s", module, option);

  ptr = g_malloc(sizeof(struct option_data)+strlen(buf));

  strcpy(ptr->str, buf);
  ptr->widget = text_entry; 
  gtk_object_set_user_data(GTK_OBJECT(widget), (gpointer)(ptr->str));
  
}

static void button_callback(GtkWidget *widget,
			    gpointer data)
{
  if(!destroy_phase) {

    flavor_mark_modified();

#ifdef PM2DEBUG
      if(GTK_TOGGLE_BUTTON(widget)->active)
      g_print ("The %s (%s) option was enabled\n",
	       (char *)gtk_object_get_user_data(GTK_OBJECT(widget)),
	       (char *)data);
    else
      g_print ("The %s (%s) option was disabled\n",
	       (char *)gtk_object_get_user_data(GTK_OBJECT(widget)),
	       (char *)data);
#endif // PM2DEBUG
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
  GtkWidget *hbox;
  option_data_t *ptr_d;
  GtkWidget *button = NULL;

  parser_start_cmd("%s/bin/pm2-module options --module=%s --get-excl=%s",
                   pm2_objroot(), module, set);

  options = string_list_from_parser();

  parser_stop();

  for(ptr = g_list_first(options);
      ptr != NULL;
      ptr = g_list_next(ptr)) {

          if(button == NULL) { /* first option */
               button = gtk_radio_button_new_with_label(NULL, (char *)ptr->data);
               } else {
                   button =gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(button)),
					(char *)ptr->data);
          }     

          if((((char *) (ptr->data))[(strlen(ptr->data)-1)] == ':')) {
          token_t tok;

          hbox = gtk_hbox_new(FALSE, 0);
          gtk_container_set_border_width (GTK_CONTAINER(hbox), 0);
          gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
          gtk_widget_show(hbox);
                    
          parser_start_cmd("%s/bin/pm2-module option=%s --module=%s --defaultvalue",                        
               pm2_objroot(), ptr->data, module);
          tok = parser_next_token();
          
          if(tok == IDENT_TOKEN) {
            ptr_d = _str2data(gtk_object_get_user_data(GTK_OBJECT(button)));
          
            gtk_signal_connect(GTK_OBJECT(button), "toggled",
          	   GTK_SIGNAL_FUNC(button_callback),
          	   (gpointer)"exclusive");          
            gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
                                        
            pEntry = gtk_entry_new();
            gtk_entry_set_text(GTK_ENTRY(pEntry),
          	   (gpointer)string_new(parser_token_image()));

            gtk_box_pack_start(GTK_BOX(hbox), pEntry, FALSE, FALSE, 0);
            gtk_signal_connect(GTK_OBJECT(pEntry), "changed",
          	   GTK_SIGNAL_FUNC(flavor_mark_modified),
          	   (GtkWidget *) button);

            attach_specific_data(button, module, (char *)ptr->data, pEntry, list);
            common_opt_register_option(ptr->data, button);

            gtk_widget_show(button);
            gtk_widget_show(pEntry);

           } else {
             parser_stop();
          }
          }

          else {
           gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       GTK_SIGNAL_FUNC(button_callback), (gpointer)"exclusive");
           attach_specific_data(button, module, (char *)ptr->data, NULL, list);
           gtk_widget_show(button);
           gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
     }

  }
  string_list_destroy(&options);  

}

static void add_inclusive_option_set(char *module,
				     char *set,
				     GtkWidget *vbox,
				     GList **list)
{
  GList *options, *ptr;
  GtkWidget *hbox;
  option_data_t *ptr_d;
  GtkWidget *button = NULL;
        
  parser_start_cmd("%s/bin/pm2-module options --module=%s --get-incl=%s",
		   pm2_objroot(), module, set);
        
  options = string_list_from_parser();
        
  parser_stop();
        
  for(ptr = g_list_first(options);
      ptr != NULL;
      ptr = g_list_next(ptr)) {
        
    if (((char *) (ptr->data))[(strlen(ptr->data)-1)] == ':') {
      token_t tok;
                        
      hbox = gtk_hbox_new(FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER(hbox), 0);
      gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show(hbox);          
                        
      parser_start_cmd("%s/bin/pm2-module option=%s --module=%s --defaultvalue",                        
		       pm2_objroot(), ptr->data, module);

      tok = parser_next_token();
      
      if(tok == IDENT_TOKEN) {
	button = gtk_check_button_new_with_label((char *)ptr->data);
	ptr_d = _str2data(gtk_object_get_user_data(GTK_OBJECT(button)));
                                        
	gtk_signal_connect(GTK_OBJECT(button), "toggled",
			   GTK_SIGNAL_FUNC(button_callback),
			   (gpointer)"inclusive");          
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
                                        
	pEntry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(pEntry),
			   (gpointer)string_new(parser_token_image()));
	gtk_box_pack_start(GTK_BOX(hbox), pEntry, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(pEntry), "changed",
			   GTK_SIGNAL_FUNC(flavor_mark_modified),
			   (GtkWidget *) button);

	attach_specific_data(button, module, (char *)ptr->data, pEntry, list);

	common_opt_register_option(ptr->data, button);
                                        
	gtk_widget_show(button);
	gtk_widget_show(pEntry);

      } else {
	parser_stop();
      }
                        
    } else {
                                
      button = gtk_check_button_new_with_label((char *)ptr->data);
      gtk_signal_connect(GTK_OBJECT(button), "toggled",
			 GTK_SIGNAL_FUNC(button_callback), (gpointer)"inclusive");
                                
      attach_specific_data(button, module, (char *)ptr->data, NULL, list);
                                
      gtk_widget_show(button);
      gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
                                
      common_opt_register_option(ptr->data, button);
    }
  }   
  string_list_destroy(&options);
}

static void add_exclusive_options(char *module,
				  GtkWidget *vbox,
				  GList **list)
{
  GtkWidget *sep;
  GList *sets, *ptr;

  parser_start_cmd("%s/bin/pm2-module options --module=%s --get-excl-sets",
                   pm2_objroot(), module);

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

  parser_start_cmd("%s/bin/pm2-module options --module=%s --get-incl-sets",
                   pm2_objroot(), module);

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
  GtkWidget *vbox;
  GtkWidget *scrolled_window;

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(scrolled_window);

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER(vbox), 5);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vbox);
  gtk_widget_show(vbox);

  intro_begin_step("Getting options for module %s...", module);

  add_exclusive_options(module, vbox, list);

  add_inclusive_options(module, vbox, list);

  intro_end_step();

  return scrolled_window;
}

static void set_module_info(GtkWidget *widget, char *module)
{
  char buf[1024], *str;

  sprintf(buf, "--modules=%s", module);
  str = string_new(buf);

  gtk_object_set_user_data(GTK_OBJECT(widget), (gpointer)str);
}

static void add_misc_options(GtkWidget *box)
{
  GtkWidget *nbook;
  int i;

  nbook = gtk_notebook_new();
  gtk_container_set_border_width (GTK_CONTAINER(nbook), 10);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK(nbook), GTK_POS_TOP);
  gtk_box_pack_start(GTK_BOX(box), nbook, TRUE, TRUE, 0);
  gtk_widget_show(nbook);

  for(i = 0;
      known_static_modules[i] != NULL;
      i++) {
    char *module = known_static_modules[i];
    module_t *cur_mod;
    GtkWidget *label;
  
    cur_mod = (module_t *)g_malloc(sizeof(module_t));

    the_static_modules = g_list_append(the_static_modules, (gpointer)cur_mod);

    cur_mod->name = module;
    cur_mod->options = NULL;

    cur_mod->page = add_options(module, &cur_mod->options);

    set_module_info(cur_mod->page, module);

    label = gtk_label_new(module);

    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), cur_mod->page, label);

    if(tips_enabled)
      module_set_tooltip_msg(label, module);
  }
}

static void set_page_select_state(GtkWidget *page, gint selected)
{
  GtkWidget *tab = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),
					      page);

  gtk_widget_set_sensitive(page, selected);
  if(tab)
    gtk_widget_set_sensitive(tab, selected);
}

static void update_select_buttons(gint active_page)
{
   GtkWidget *page;
   gint select_ok;

   page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), active_page);

   select_ok = page->state == GTK_STATE_INSENSITIVE;
   gtk_widget_set_sensitive(select_button, select_ok);
   gtk_widget_set_sensitive(deselect_button, !select_ok);

   menu_update_module(select_ok, !select_ok);
}

void module_select(void)
{
  GtkWidget *page;
  gint num;

  num = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), num);

  set_page_select_state(page, TRUE);

  update_select_buttons(num);

  flavor_mark_modified();
}

void module_deselect(void)
{
  GtkWidget *page;
  gint num;

  num = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), num);

  set_page_select_state(page, FALSE);

  update_select_buttons(num);

  flavor_mark_modified();
}

static void select_pressed(GtkWidget *widget,
			   gpointer data)
{
  if(!destroy_phase) {
    module_select();
  }
}

static void deselect_pressed(GtkWidget *widget,
			     gpointer data)
{
  if(!destroy_phase) {
    module_deselect();
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

  module_frame = gtk_frame_new(" Modules ");
  //gtk_widget_set_sensitive(module_frame, FALSE);
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
          /* Add the page to the notebook is ONLY if module is visible! */
          if(g_list_find_custom(visible_names, module, (GCompareFunc)strcmp))
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
  GtkWidget *vbox;

  general_frame = gtk_frame_new(" General settings ");
  gtk_container_set_border_width (GTK_CONTAINER(general_frame), 10);
  gtk_box_pack_start(GTK_BOX(box), general_frame, TRUE, TRUE, 0);
  gtk_widget_show(general_frame);

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(general_frame), vbox);
  gtk_widget_show(vbox);

  add_misc_options(vbox);
}

static void module_update_module_options(module_t *m)
{
  GList *opt;  
  for(opt = g_list_first(m->options);
      opt != NULL;
      opt = g_list_next(opt)) {
    GtkWidget *button = (GtkWidget *)opt->data;
    option_data_t *ptr;
    char *value;
    
    ptr = _str2data(gtk_object_get_user_data(GTK_OBJECT(button)));
    value = flavor_uses_option(ptr->str);
    
    if(value != NULL) {
      gtk_toggle_button_set_active((GtkToggleButton *)button, TRUE);
      if(ptr->widget != NULL) {
	 gtk_entry_set_text(GTK_ENTRY(pEntry), value+ 1);
      }
    } else
      gtk_toggle_button_set_active((GtkToggleButton *)button, FALSE); 

  }
}

void module_update_with_current_flavor(void)
{
  GList *mod;

  //  if(!strcmp(flavor_name(),"")) {
  //    gtk_widget_set_sensitive(module_frame, FALSE);
  //    gtk_widget_set_sensitive(general_frame, FALSE);
  //    return;
  //  }

  // Modules implicites
  for(mod = g_list_first(the_static_modules);
      mod != NULL;
      mod = g_list_next(mod)) {

    module_t *m = (module_t *)mod->data;

    module_update_module_options(m);
  }

  // Modules sélectionnés
  for(mod = g_list_first(the_modules);
      mod != NULL;
      mod = g_list_next(mod)) {

    module_t *m = (module_t *)mod->data;

    //    if(g_list_find_custom(visible_names, m->name, (GCompareFunc)strcmp)) {
      /* Module "m->name" is visible, so we can update he display */

      if(flavor_uses_module((char *)gtk_object_get_user_data(GTK_OBJECT(m->page)))) {

	set_page_select_state(m->page, TRUE);

	module_update_module_options(m);

      } else {

	set_page_select_state(m->page, FALSE);

      }
      //    }
  }

  update_select_buttons(gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
}

static void module_save_module_options(module_t *m)
{
  GList *opt;
  option_data_t *ptr;

  for(opt = g_list_first(m->options);
      opt != NULL;
      opt = g_list_next(opt)) {

          GtkWidget *button = (GtkWidget *)opt->data;
          ptr = _str2data(gtk_object_get_user_data(GTK_OBJECT(button)));
          
          if(gtk_toggle_button_get_active((GtkToggleButton *)button)) {
                  
                  if(ptr->widget != NULL)
		  {
                                  char buf[1024];
                                  
                                  strcpy(buf, ptr->str);
                                  strcat(buf, gtk_entry_get_text(GTK_ENTRY(ptr->widget)));
                                  gtk_entry_set_text(GTK_ENTRY(pEntry), gtk_entry_get_text(GTK_ENTRY(ptr->widget)));
                                  flavor_add_option(buf);
		  } else {             
                                  flavor_add_option(ptr->str);
		  }
          }   
  }
}

void module_save_to_flavor(void)
{
  GList *mod;

  flavor_reset_contents();

  // Modules implicites
  for(mod = g_list_first(the_static_modules);
      mod != NULL;
      mod = g_list_next(mod)) {

    module_t *m = (module_t *)mod->data;

    module_save_module_options(m);
  }

  // Modules sélectionnés
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

  module_update_with_current_flavor();
}

