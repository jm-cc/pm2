
#ifndef INTRO_IS_DEF
#define INTRO_IS_DEF

#include <stdarg.h>

guchar *pm2_root(void);

void intro_init(void);

void intro_exit(void);

void intro_begin_step(gchar *fmt, ...);

void intro_end_step(void);

#endif
