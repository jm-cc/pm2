
#ifndef FLAVOR_IS_DEF
#define FLAVOR_IS_DEF

extern gint flavor_modified;

void flavor_init(GtkWidget *vbox);

/* Returns the list of flavor names (GList of strings) using
   pm2-flavor... */
GList *flavor_list(void);

void flavor_create(void);

void flavor_load(void);

void flavor_save_as(void);

void flavor_delete(void);

void flavor_mark_modified(void);

void flavor_check_quit(void);


char *flavor_name(void);

char *flavor_builddir(void);

char *flavor_extension(void);

// Format for module is "--modules=name"
gint flavor_uses_module(char *module);

// Format for option is "--module=option"
gint flavor_uses_option(char *option);

void flavor_reset_contents(void);

void flavor_set_builddir(char *name);

void flavor_set_extension(char *name);

void flavor_add_module(char *name);

void flavor_add_option(char *name);

#endif
