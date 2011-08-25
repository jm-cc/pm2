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


#ifndef __SYS_MARCEL_GLUEC_STDIO_H__
#define __SYS_MARCEL_GLUEC_STDIO_H__


#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include "marcel_config.h"
#include "marcel_alias.h"
#include "tbx_compiler.h"


#ifndef MARCEL_LIBPTHREAD
#  define libc_vfprintf vfprintf
#  define libc_vsprintf vsprintf
#  define libc_fputs    fputs
#  define libc_puts     puts
#  define libc_putc     putc
#  define libc_fgets    fgets
#  define libc_fread    fread
#  define libc_fwrite   fwrite
#  define libc_fopen    fopen
#  define libc_fclose   fclose
#  define libc_fflush   fflush
#  define libc_feof     feof
#  define libc_vfscanf  vfscanf
#  define libc_vsscanf  vsscanf
#endif


/* For compatibility purposes */
#define tprintf  marcel_printf
#define tfprintf marcel_fprintf
#define marcel_snprintf tbx_snprintf
#define marcel_vsnprintf tbx_vsnprintf


/** Public functions **/
DEC_MARCEL(int, printf, (const char *__restrict format, ...));
DEC_MARCEL(int, sprintf, (char *__restrict s, const char *__restrict format, ...));
DEC_MARCEL(int, fprintf, (FILE *__restrict stream, const char *__restrict format, ...));
DEC_MARCEL(int, vfprintf, (FILE *__restrict stream, const char *__restrict format, va_list ap));
DEC_MARCEL(int, vprintf, (const char *__restrict format, va_list ap));
DEC_MARCEL(int, vsprintf, (char *__restrict s, const char *__restrict format, va_list ap));

DEC_MARCEL(int, puts, (const char *s));
DEC_MARCEL(int, putchar, (int c));
DEC_MARCEL(int, putc,(int c, FILE *stream));
DEC_MARCEL(int, fputc, (int c, FILE *stream));
DEC_MARCEL(int, fputs, (const char *s, FILE *stream));
DEC_MARCEL(char*, fgets, (char *__restrict s, int n, FILE *__restrict stream));

DEC_MARCEL(size_t, fread, (void *ptr, size_t size, size_t nmemb, FILE *stream));
DEC_MARCEL(size_t, fwrite, (const void *ptr, size_t size, size_t nmemb, FILE *stream));
DEC_MARCEL(FILE*, fopen, (const char *__restrict filename, const char *__restrict mode));
DEC_MARCEL(int, fclose, (FILE *stream));
DEC_MARCEL(int, fflush, (FILE *stream));
DEC_MARCEL(int, feof, (FILE *stream));

DEC_MARCEL(int, fscanf, (FILE *__restrict stream, const char *__restrict format, ... ));
DEC_MARCEL(int, vfscanf, (FILE *__restrict stream, const char *__restrict format, va_list arg));
DEC_MARCEL(int, scanf, (const char *__restrict format, ... ));
DEC_MARCEL(int, sscanf, (const char *__restrict s, const char *__restrict format, ... ));
DEC_MARCEL(int, vscanf, (const char *__restrict format, va_list arg ));
DEC_MARCEL(int, vsscanf, (const char *__restrict s, const char *__restrict format, va_list arg ));


#endif /** __SYS_MARCEL_GLUEC_STDIO_H__ **/
