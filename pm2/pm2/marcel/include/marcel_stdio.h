
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

#ifndef MARCEL_STDIO_EST_DEF
#define MARCEL_STDIO_EST_DEF

#include <stdio.h>
#include <sys/time.h>

// For compatibility purposes :
#define tprintf  marcel_printf
#define tfprintf marcel_fprintf

int marcel_printf(char *format, ...);
int marcel_fprintf(FILE *stream, char *format, ...);
int marcel_sprintf(char *string, char *format, ...);
int marcel_snprintf(char *string, size_t size, char *format,...);
FILE *marcel_fopen(char *path, char *mode);
int marcel_fclose(FILE *stream);
int marcel_fflush(FILE *stream);
#endif
