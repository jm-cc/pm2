
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>

#include "main.h"
#include "flavor.h"
#include "parser.h"
#include "str_list.h"
#include "trace.h"
#include "module.h"
#include "shell.h"
#include "intro.h"
#include "dialog.h"
#include "statusbar.h"
#include "menu.h"

static GtkWidget *the_load_button;
static GtkWidget *the_create_button;
static GtkWidget *the_save_button;
static GtkWidget *combo;

typedef struct flavor_struct_t {
  char *name;
  char *builddir;
  char *extension;
  GList *modules;
  GList *options;
} flavor_t;

static GList *fla_names = NULL;
static GList *the_flavors = NULL;
static flavor_t *cur_flavor = NULL;

gint flavor_modified = FALSE;
static gint flavor_never_saved = FALSE;

static int flavor_save(void);

#ifdef DEBUG
static void flavor_print(void)
{  GList *ptr;

  if(cur_flavor != NULL) {
    g_print("name      : [%s]\n", (cur_flavor->name ? : ""));
    g_print("builddir  : [%s]\n", (cur_flavor->builddir ? : ""));
    g_print("extension : [%s]\n", (cur_flavor->extension ? : ""));
    g_print("modules   : ");
    for(ptr = g_list_first(cur_flavor->modules);
	ptr != NULL;
	ptr = g_list_next(ptr)) {
      g_print("%s ", (char *)ptr->data);
    }
    g_print("\noptions   : ");
    for(ptr = g_list_first(cur_flavor->options);
	ptr != NULL;
	ptr = g_list_next(ptr)) {
      g_print("%s ", (char *)ptr->data);
    }
    g_print("\n");
  }
}
#endif

static gint flavor_exists(char *name);

static void update_the_buttons(void)
{
  char *selected = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
  gint load_enabled, create_enabled, save_enabled, delete_enabled;
  gint save_as = FALSE;
  gint reload = FALSE;
  gint check_enabled;

  if(flavor_exists(selected)) {
    delete_enabled = TRUE;
    create_enabled = FALSE;
    if(cur_flavor == NULL) {
      // Initial state
      load_enabled = TRUE;
      save_enabled = FALSE; // Really nothing to save !
    } else if(strcmp(cur_flavor->name, selected)) {
      load_enabled = TRUE;
      save_enabled = TRUE; save_as = TRUE; // Actually a "save as"
    } else {
      // Selected flavor is the current one
      if(flavor_modified) {
	load_enabled = TRUE; reload = TRUE; // Actually a "reload"
	save_enabled = TRUE;
      } else {
	load_enabled = FALSE;
	save_enabled = FALSE;
      }
    }
  } else if(!strcmp(selected, "")) {
    // No selected flavor
    load_enabled = save_enabled = create_enabled = delete_enabled = FALSE;
  } else {
    // Selected flavor does not exist yet
    delete_enabled = FALSE;
    load_enabled = FALSE;
    create_enabled = TRUE;
    if(cur_flavor == NULL) {
      save_enabled = FALSE; // Nothing to save
    } else {
      save_enabled = TRUE; save_as = TRUE;
    }
  }

  check_enabled = cur_flavor != NULL;

  if(save_as)
    gtk_label_set_text(GTK_LABEL(GTK_BIN(the_save_button)->child), "Save As");
  else
    gtk_label_set_text(GTK_LABEL(GTK_BIN(the_save_button)->child), "Save");

  if(reload)
    gtk_label_set_text(GTK_LABEL(GTK_BIN(the_load_button)->child), "Reload");
  else
    gtk_label_set_text(GTK_LABEL(GTK_BIN(the_load_button)->child), "Load");

  gtk_widget_set_sensitive(the_load_button, load_enabled);
  gtk_widget_set_sensitive(the_create_button, create_enabled);
  gtk_widget_set_sensitive(the_save_button, save_enabled);

  menu_update_flavor(load_enabled, reload,
		     create_enabled,
		     save_enabled, save_as,
		     delete_enabled,
		     check_enabled);
}

void flavor_mark_modified(void)
{
  if(!flavor_modified) {
    flavor_modified = TRUE;

    update_the_buttons();
  }
}

static gint flacmp(gconstpointer a, gconstpointer b)
{
  return strcmp(((flavor_t *)a)->name, (char *)b);
}

static gint flavor_exists(char *name)
{
  return g_list_find_custom(flavor_list(), name, (GCompareFunc)strcmp) ?
    TRUE : FALSE;
}

static flavor_t *find_flavor(char *name)
{
  GList *ptr;

  ptr = g_list_find_custom(the_flavors, (gpointer)name, flacmp);

  if(!ptr) {
    return NULL;
  } else {
    return (flavor_t *)ptr->data;
  }
}

static flavor_t *load_flavor(char *name)
{
  token_t tok;
  char buf[1024];
  enum { FLAVOR_TAG, BUILDDIR_TAG, EXT_TAG, MODULE_TAG, OPTION_TAG } tag;
  flavor_t * ptr;

  if(!flavor_exists(name))
    return NULL;

  statusbar_set_message("Loading flavor %s...", name);

  ptr = (flavor_t *)g_malloc(sizeof(flavor_t));
  ptr->modules = ptr->options = NULL;

  parser_start_cmd("%s/bin/pm2-flavor get --flavor=%s",
    pm2_root(), name);

  while(tok = parser_next_token(), tok != END_OF_INPUT) {

    if(tok != DASH_DASH_TOKEN)
      parser_error();

    if(!strcmp(parser_token_image(), "--flavor"))
      tag = FLAVOR_TAG;
    else if (!strcmp(parser_token_image(), "--builddir"))
      tag = BUILDDIR_TAG;
    else if (!strcmp(parser_token_image(), "--ext"))
      tag = EXT_TAG;
    else if (!strcmp(parser_token_image(), "--modules"))
      tag = MODULE_TAG;
    else
      tag = OPTION_TAG;

    if(tag != OPTION_TAG) {
      tok = parser_next_token();
      if(tok != EQUAL_TOKEN)
	parser_error();
    }

    switch(tag) {
    case FLAVOR_TAG : {
      ptr->name = string_new(parser_token_image()+1);
      break;
    }
    case BUILDDIR_TAG : {
      ptr->builddir = string_new(parser_token_image()+1);
      break;
    }
    case EXT_TAG : {
      ptr->extension = string_new(parser_token_image()+1);
      break;
    }
    case MODULE_TAG : {
      sprintf(buf, "--modules=%s", parser_token_image()+1);
      ptr->modules = g_list_append(ptr->modules, (gpointer)string_new(buf));
      break;
    }
    case OPTION_TAG : {
      strcpy(buf, parser_token_image());
      tok = parser_next_token();
      if(tok != EQUAL_TOKEN)
	parser_error();
      strcat(buf, parser_token_image());
      ptr->options = g_list_append(ptr->options, (gpointer)string_new(buf));
      break;
    }
    }
  }

  parser_stop();

  the_flavors = g_list_append(the_flavors, (gpointer)ptr);

  statusbar_concat_message("Done.");

  return ptr;
}

static void alloc_flavor_fields(flavor_t *ptr, char *name)
{
  ptr->name = string_new(name);
  ptr->builddir = string_new("");
  ptr->extension = string_new("");
  ptr->modules = ptr->options = NULL;
}

static void free_flavor_fields(flavor_t *ptr)
{
  g_free(ptr->name);
  g_free(ptr->builddir);
  g_free(ptr->extension);
  string_list_destroy(&ptr->modules);
  string_list_destroy(&ptr->options);
}

static flavor_t *create_new_flavor(char *name)
{
  flavor_t *ptr;
  char buf[1024];

  statusbar_set_message("Creating flavor %s...", name);

  strcpy(buf, name); // Important _avant_ ...set_popdown_strings !

  fla_names = g_list_insert_sorted(fla_names,
				   (gpointer)string_new(buf),
				   (GCompareFunc)strcmp);
  gtk_combo_set_popdown_strings(GTK_COMBO(combo),
				fla_names);
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), buf);

  ptr = (flavor_t *)g_malloc(sizeof(flavor_t));

  alloc_flavor_fields(ptr, buf);

  the_flavors = g_list_append(the_flavors, (gpointer)ptr);

  flavor_modified = TRUE;
  flavor_never_saved = TRUE;

  statusbar_concat_message("Done.");

  return ptr;
}

static void update_current(void)
{
  gint was_modified;

  // We must save "flavor_modified" because it will be set to "TRUE"
  // by module_update_with_current_flavor...
  was_modified = flavor_modified;
  module_update_with_current_flavor();

  flavor_modified = was_modified;

  update_the_buttons();

  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry),
		     (cur_flavor ? cur_flavor->name : ""));
}

static void flavor_destroy(char *name)
{
  flavor_t *ptr;
  gpointer data;

  // Remove from the flavor list (if applicable)
  ptr = find_flavor(name);
  if(ptr != NULL) {
    the_flavors = g_list_remove(the_flavors, (gpointer)ptr);

    free_flavor_fields(ptr);
    g_free(ptr);

    if(cur_flavor == ptr) {
      cur_flavor = NULL;
      flavor_modified = FALSE;
      flavor_never_saved = FALSE;
    }
  }

  // Remove from the flavor name list
  data = g_list_find_custom(flavor_list(), name, (GCompareFunc)strcmp)->data;
  fla_names = g_list_remove(fla_names, data);

  gtk_combo_set_popdown_strings(GTK_COMBO(combo),
				fla_names);
}

static void flavor_destroy_current(void)
{
  char buf[1024];

  strcpy(buf, cur_flavor->name);

  flavor_destroy(buf);
}

void flavor_delete(void)
{
  char selected[1024];

  strcpy(selected, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));

  statusbar_set_message("Deleting flavor %s...", selected);

  if(!flavor_never_saved) {
    // We must remove the file on disk
    int ret;

    ret = exec_wait(exec_single_cmd_fmt(NULL,
					"%s/bin/pm2-flavor delete --flavor=%s",
					pm2_root(), selected));

    if(ret != 0) {
      fprintf(stderr, "Error: Delete operation failed\n");
      exit(1);
    }
  }

  flavor_destroy(selected);

  update_current();

  statusbar_concat_message("Done.");
}

static void save_and_proceed(gpointer data)
{
  char name[1024];

  strcpy(name, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));

  if((gint)data == TRUE)
    if(flavor_save() != 0)
      return;

  if(flavor_never_saved)
    flavor_destroy_current();

  flavor_modified = FALSE;

  cur_flavor = find_flavor(name) ? :
    (load_flavor(name) ? : create_new_flavor(name));

  update_current();

  statusbar_set_current_flavor(cur_flavor->name);
}

static void reload_flavor(gpointer data)
{
  save_and_proceed((gpointer)FALSE);
}

void set_current_flavor(char *name)
{
  static char str[1024];

  if(cur_flavor != NULL && !strcmp(name, cur_flavor->name)) { // => reload
    dialog_prompt("Are you sure you want to abandon your modifications?",
		  "Proceed", reload_flavor, NULL,
		  "Cancel", NULL, NULL,
		  NULL, NULL, NULL);
    return;
  }

  // First of all, we must check if the old flavor was saved
  if(flavor_modified) {
    sprintf(str, "Warning: Flavor \"%s\" was modified!", cur_flavor->name);

    dialog_prompt(str,
		  "Save and proceed", save_and_proceed, (gpointer)TRUE,
		  "Proceed without saving", save_and_proceed, (gpointer)FALSE,
		  "Cancel", NULL, NULL);
  } else
    save_and_proceed((gpointer)FALSE);
}

static int flavor_save_on_disk(void)
{
  GList *ptr;
  static char cmd[4096];

  sprintf(cmd, "%s/bin/pm2-flavor set --flavor=%s --builddir=%s --ext=%s",
          pm2_root(),
	  cur_flavor->name,
	  cur_flavor->builddir,
	  cur_flavor->extension);

  for(ptr = g_list_first(cur_flavor->modules);
      ptr != NULL;
      ptr = g_list_next(ptr)) {
    strcat(cmd, " ");
    strcat(cmd, (char *)ptr->data);
  }

  for(ptr = g_list_first(cur_flavor->options);
      ptr != NULL;
      ptr = g_list_next(ptr)) {
    strcat(cmd, " ");
    strcat(cmd, (char *)ptr->data);
  }

  return exec_wait(exec_single_cmd_fmt(NULL, cmd));
}

static int flavor_do_check(void)
{
  flavor_t tmp, *old_cur;
  GList *ptr;
  static char cmd[4096];
  int ret;

  statusbar_set_message("Checking correctness of current flavor...");

  old_cur = cur_flavor;
  cur_flavor = &tmp;

  alloc_flavor_fields(cur_flavor, "foo");

  module_save_to_flavor();

  sprintf(cmd, "%s/bin/pm2-flavor check --builddir=%s --ext=%s",
          pm2_root(),
	  cur_flavor->builddir,
	  cur_flavor->extension);

  for(ptr = g_list_first(cur_flavor->modules);
      ptr != NULL;
      ptr = g_list_next(ptr)) {
    strcat(cmd, " ");
    strcat(cmd, (char *)ptr->data);
  }

  for(ptr = g_list_first(cur_flavor->options);
      ptr != NULL;
      ptr = g_list_next(ptr)) {
    strcat(cmd, " ");
    strcat(cmd, (char *)ptr->data);
  }

  ret = exec_wait(exec_single_cmd_fmt(NULL, cmd));

  if(ret == 0) {
    statusbar_concat_message("OK.");
  } else {
    statusbar_concat_message("FAILURE: see /tmp/ezflavor.errlog.");
  }

  free_flavor_fields(cur_flavor);

  cur_flavor = old_cur;

  return ret;
}

void flavor_check(void)
{
  flavor_do_check();
}

static int flavor_save(void)
{
  int ret;

  if(flavor_do_check() != 0)
    return 1;

  statusbar_set_message("Saving flavor %s...", cur_flavor->name);

  module_save_to_flavor();

  ret = flavor_save_on_disk();

  if(ret == 0) {
    flavor_modified = FALSE;
    flavor_never_saved = FALSE;
    statusbar_concat_message("Done.");
  } else {
    statusbar_concat_message("FAILURE: see /tmp/ezflavor.errlog.");
    return ret;
  }

  update_the_buttons();

  return 0;
}

static void save_and_load(gpointer data)
{
  char new_fla[1024];

  strcpy(new_fla, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));

  if((gint)data == TRUE) { // Override
    if(flavor_never_saved)
      flavor_destroy_current();

    cur_flavor = find_flavor(new_fla);
    if(flavor_save() != 0)
      return;
  } else {
    if(flavor_save() != 0)
      return;
    cur_flavor = find_flavor(new_fla);
    update_current();
  }

  statusbar_set_current_flavor(cur_flavor->name);
}

void flavor_save_as(void)
{
  char new_fla[1024];
  flavor_t *ptr;
  static char str1[1024], str2[1024];

  strcpy(new_fla, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));

  if(strcmp(new_fla,cur_flavor->name)) { // Really a "save _AS_"!

    ptr = (find_flavor(new_fla) ? : load_flavor(new_fla));

    if(ptr) { // new_flavor already exists
       sprintf(str1, "Warning: Flavor \"%s\" already exists!", new_fla);
       sprintf(str2, "Save under \"%s\" and proceed", cur_flavor->name);
       dialog_prompt(str1,
		"Override", save_and_load, (gpointer)TRUE,
		str2, save_and_load, (gpointer)FALSE,
		"Cancel", NULL, NULL);
    } else { // We must first create new_fla, and then save

      if(flavor_never_saved)
	flavor_destroy_current();

      cur_flavor = create_new_flavor(new_fla);
      if(flavor_save() != 0)
	return;

      update_current();
      statusbar_set_current_flavor(cur_flavor->name);
    }
  } else {
    if(flavor_save() != 0)
      return;
  }
}

void flavor_load(void)
{
  set_current_flavor(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));
}

void flavor_create(void)
{
  set_current_flavor(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));
}

static void save_and_quit(gpointer data)
{
  if((int)data == TRUE)
    if(flavor_save() != 0)
      return;

  destroy_phase = TRUE;

  gtk_main_quit();
}

void flavor_check_quit(void)
{
  char str1[128], *new_fla;

  sprintf(str1, "Flavor \"%s\" was modified. Quit anyway?", cur_flavor->name);

  new_fla = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));

  if(strcmp(new_fla, cur_flavor->name)) {
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), cur_flavor->name);
  }

  dialog_prompt(str1,
		"Save and quit", save_and_quit, (gpointer)TRUE,
		"Quit without saving", save_and_quit, (gpointer)FALSE,
		"Cancel", NULL, NULL);
}

void flavor_rescan(void)
{

  intro_begin_step("Getting list of flavors...");

  string_list_destroy(&fla_names);

  parser_start_cmd("%s/bin/pm2-flavor list", pm2_root());

  fla_names = string_list_from_parser();

  parser_stop();

  intro_end_step();
}

GList *flavor_list(void)
{
  if(!fla_names)
    flavor_rescan();

  return fla_names;
}

static void flavor_name_changed(GtkWidget *widget,
				gpointer data )
{
  char *name;

  if(!destroy_phase) {
    name = gtk_entry_get_text(GTK_ENTRY(widget));

    update_the_buttons();
  }
}

static void flavor_load_pressed(GtkWidget *widget,
				gpointer data)
{
  if(!destroy_phase) {
    flavor_load();
  }
}

static void flavor_create_pressed(GtkWidget *widget,
				  gpointer data)
{
  if(!destroy_phase) {
    flavor_create();
  }
}

static void flavor_save_pressed(GtkWidget *widget,
				gpointer data)
{
  if(!destroy_phase) {
    flavor_save_as();
  }
}

static void flavor_build_selector(GtkWidget *vbox)
{
  GtkWidget *frame;
  GtkWidget *int_vbox;

  frame = gtk_frame_new(" Flavors ");
  gtk_container_set_border_width (GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show(frame);

  int_vbox = gtk_vbox_new(FALSE, 0);
  //gtk_container_set_border_width (GTK_CONTAINER(int_vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), int_vbox);
  gtk_widget_show(int_vbox);

  combo = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(combo),
				flavor_list());
  gtk_signal_connect(GTK_OBJECT(GTK_ENTRY(GTK_COMBO(combo)->entry)), "changed",
		     GTK_SIGNAL_FUNC(flavor_name_changed), NULL);
  gtk_container_set_border_width (GTK_CONTAINER(combo), 10);
  gtk_box_pack_start(GTK_BOX(int_vbox), combo, FALSE, TRUE, 0);
  gtk_widget_show(combo);

  the_load_button = gtk_button_new_with_label("Load");
  gtk_box_pack_start(GTK_BOX(int_vbox), the_load_button, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER(the_load_button), 10);
  gtk_signal_connect(GTK_OBJECT(the_load_button), "clicked",
		     GTK_SIGNAL_FUNC(flavor_load_pressed),
		     (gpointer)(GTK_COMBO(combo)->entry));
  gtk_widget_show(the_load_button);

  the_create_button = gtk_button_new_with_label("New");
  gtk_box_pack_start(GTK_BOX(int_vbox), the_create_button, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER(the_create_button), 10);
  gtk_signal_connect(GTK_OBJECT(the_create_button), "clicked",
		     GTK_SIGNAL_FUNC(flavor_create_pressed),
		     (gpointer)(GTK_COMBO(combo)->entry));
  gtk_widget_show(the_create_button);

  the_save_button = gtk_button_new_with_label("Save");
  gtk_box_pack_start(GTK_BOX(int_vbox), the_save_button, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER(the_save_button), 10);
  gtk_signal_connect(GTK_OBJECT(the_save_button), "clicked",
		     GTK_SIGNAL_FUNC(flavor_save_pressed),
		     (gpointer)(GTK_COMBO(combo)->entry));
  gtk_widget_show(the_save_button);

  update_the_buttons();
}

char *flavor_name(void)
{
  return (cur_flavor == NULL) ? "" : cur_flavor->name;
}

char *flavor_builddir(void)
{
  return (cur_flavor == NULL) ? "" : cur_flavor->builddir;
}

char *flavor_extension(void)
{
  return (cur_flavor == NULL) ? "" : cur_flavor->extension;
}

gint flavor_uses_module(char *module)
{
  if(cur_flavor == NULL)
    return FALSE;
  else
    return g_list_find_custom(cur_flavor->modules, module, (GCompareFunc)strcmp) ?
      TRUE : FALSE;
}

gint flavor_uses_option(char *option)
{
  if(cur_flavor == NULL)
    return FALSE;
  else
    return g_list_find_custom(cur_flavor->options, option, (GCompareFunc)strcmp) ?
      TRUE : FALSE;
}

void flavor_init(GtkWidget *vbox)
{
  flavor_rescan();

  if(!fla_names) {
    fprintf(stderr, "Error: no flavor found!\n");
    fprintf(stderr, "Try 'make init' in $PM2_ROOT to create basic initial flavors\n");
    exit(1);
  }

  flavor_build_selector(vbox);
}

void flavor_reset_contents(void)
{
  if(cur_flavor->builddir)
    g_free(cur_flavor->builddir);
  if(cur_flavor->extension)
    g_free(cur_flavor->extension);
  string_list_destroy(&cur_flavor->modules);
  string_list_destroy(&cur_flavor->options);
}

void flavor_set_builddir(char *name)
{
  cur_flavor->builddir = string_new(name);
}

void flavor_set_extension(char *name)
{
  cur_flavor->extension = string_new(name);
}

void flavor_add_module(char *name)
{
  cur_flavor->modules = g_list_append(cur_flavor->modules,
				      (gpointer)string_new(name));
}

void flavor_add_option(char *name)
{
  cur_flavor->options = g_list_append(cur_flavor->options,
				      (gpointer)string_new(name));
}
