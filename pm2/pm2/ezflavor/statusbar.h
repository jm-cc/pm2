
#ifndef STATUSBAR_IS_DEF
#define STATUSBAR_IS_DEF

void statusbar_init(GtkWidget *hbox);

void statusbar_set_current_flavor(char *name);

void statusbar_set_message(char *fmt, ...);

void statusbar_concat_message(char *fmt, ...);

#endif
