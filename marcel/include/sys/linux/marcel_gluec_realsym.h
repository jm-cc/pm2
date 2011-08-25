/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2011 the PM2 team (see AUTHORS file)
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


#ifndef __SYS_LINUX_MARCEL_GLUEC_REALSYM_H__
#define __SYS_LINUX_MARCEL_GLUEC_REALSYM_H__


#ifdef __MARCEL_KERNEL__


/* memory */
extern void *__libc_calloc(size_t nmemb, size_t size);
extern void *__libc_malloc(size_t size);
extern void *__libc_valloc(size_t size);
extern void  __libc_free(void *ptr);
extern void *__libc_realloc(void *ptr, size_t size);
extern void *__libc_memalign(size_t boundary, size_t size);
#define libc_calloc   __libc_calloc
#define libc_malloc   __libc_malloc
#define libc_valloc   __libc_valloc
#define libc_free     __libc_free
#define libc_realloc  __libc_realloc
#define libc_memalign __libc_memalign

/* stdio */
extern int    _IO_vfprintf(FILE * __restrict stream, const char * __restrict format, va_list ap);
extern int    _IO_vsprintf(char * __restrict str, const char * __restrict format, va_list ap);
extern int    _IO_fputs(const char *str, FILE *stream);
extern int    _IO_puts(const char *s);
extern int    _IO_putc(int c, FILE *stream);
extern char  *_IO_fgets(char * __restrict str, int size, FILE * __restrict stream);
extern size_t _IO_fread(void * __restrict ptr, size_t size, size_t nmemb, FILE * __restrict stream);
extern size_t _IO_fwrite(const void * __restrict ptr, size_t size, size_t nmemb, FILE * __restrict stream);
extern FILE  *_IO_fopen(const char * __restrict path, const char * __restrict mode);
extern int    _IO_fclose(FILE *stream);
extern int    _IO_fflush(FILE *stream);
extern int    _IO_feof(FILE *stream);
extern int    __isoc99_vfscanf(FILE * __restrict stream, const char * __restrict format, va_list ap);
extern int    __isoc99_vsscanf(const char * __restrict str, const char * __restrict format, va_list ap);
#define libc_vfprintf _IO_vfprintf
#define libc_vsprintf _IO_vsprintf
#define libc_fputs    _IO_fputs
#define libc_puts     _IO_puts
#define libc_putc     _IO_putc
#define libc_fgets    _IO_fgets
#define libc_fread    _IO_fread
#define libc_fwrite   _IO_fwrite
#define libc_fopen    _IO_fopen
#define libc_fclose   _IO_fclose
#define libc_fflush   _IO_fflush
#define libc_feof     _IO_feof
#define libc_vfscanf  __isoc99_vfscanf
#define libc_vsscanf  __isoc99_vsscanf


#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_LINUX_MARCEL_GLUEC_REALSYM_H__ **/
