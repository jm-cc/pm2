
#ifndef COMMONOPT_IS_DEF
#define COMMONOPT_IS_DEF

#include <gtk/gtk.h>

void common_opt_init(void);

void common_opt_display_panel(void);

void common_opt_hide_panel(void);

void common_opt_register_option(char *name,
				GtkWidget *button);

#endif
