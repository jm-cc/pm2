/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_STDIO_H__
#define __MARCEL_STDIO_H__


#include <sys/time.h>
#include <unistd.h>
#include "sys/marcel_flags.h"
#include "tbx_compiler.h"
#if defined(PUK) && defined(PUK_ENABLE_PUKABI)
#include <Padico/Puk.h>
#include <Padico/Puk-ABI.h>
#else
#include <stdio.h>
#endif /* PUKABI */


/*  For compatibility purposes : */
#define tprintf  marcel_printf
#define tfprintf marcel_fprintf


#ifdef PUK_ENABLE_PUKABI


#define marcel_printf printf
#define marcel_fprintf fprintf
#define marcel_sprintf sprintf
#define marcel_snprintf snprintf
#define marcel_fopen open
#define marcel_fclose close
#define marcel_fflush fflush
#define marcel_scanf scanf
#define marcel_fscanf fscanf
#define marcel_fgets fgets
#define marcel_feof feof
#define marcel_getdelim getdelim


#else


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
char *marcel_fgets(char *s, int size, FILE *stream);
int marcel_feof(FILE *stream);
ssize_t marcel_getdelim(char **lineptr, size_t *n, int delim, FILE *stream);


#endif /** PUK_ABI **/


#endif /** __MARCEL_STDIO_H__ **/
