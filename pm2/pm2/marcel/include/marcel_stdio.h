
#ifndef MARCEL_STDIO_EST_DEF
#define MARCEL_STDIO_EST_DEF

#include <stdio.h>
#include <sys/time.h>

// For compatibility purposes :
#define tprintf  marcel_printf
#define tfprintf marcel_fprintf

int marcel_printf(char *format, ...);
int marcel_fprintf(FILE *stream, char *format, ...);

// Still here, but do not use it!
int tselect(int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);

#endif
