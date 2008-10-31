
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

#include "tbx_compiler.h"

/*  For compatibility purposes : */
#define tprintf  marcel_printf
#define tfprintf marcel_fprintf

TBX_FORMAT(printf,1,2)
int marcel_printf(const char * __restrict format, ...);
TBX_FORMAT(printf,2,3)
int marcel_fprintf(FILE * __restrict stream, const char * __restrict format, ...);
TBX_FORMAT(printf,2,3)
int marcel_sprintf(char * __restrict string, const char * __restrict format, ...);
TBX_FORMAT(printf,3,4)
int marcel_snprintf(char * __restrict string, size_t size, const char * __restrict format,...);
FILE *marcel_fopen(const char * __restrict path, const char * __restrict mode);
int marcel_fclose(FILE *stream);
int marcel_fflush(FILE *stream);
TBX_FORMAT(scanf,1,2)
int marcel_scanf(const char *__restrict format, ...);
TBX_FORMAT(scanf,2,3)
int marcel_fscanf(FILE * __restrict stream, const char *__restrict format, ...);
#endif
