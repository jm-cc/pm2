
#ifndef MODULE_IS_DEF
#define MODULE_IS_DEF

#include <glib.h>

void module_init(GtkWidget *leftbox, GtkWidget *rightbox);

void module_select(void);

void module_deselect(void);

void module_update_with_current_flavor(void);

void module_save_to_flavor(void);

#endif
