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


#include <stdarg.h>
#include <stdio.h>
#include "marcel.h"


DEF_MARCEL_PMARCEL_VAARGS(int, printf, (const char *__restrict format, ...),
{
	int status;
	va_list arg;

	va_start(arg, format);
	status = marcel_vprintf(format, arg);
	va_end(arg);

	return status;
})
DEF_STRONG_ALIAS(marcel_printf, printf)

DEF_MARCEL_PMARCEL_VAARGS(int, sprintf, (char *__restrict s, const char *__restrict format, ...), 
{
	int status;
	va_list arg;

	va_start(arg, format);
	status = marcel_vsprintf(s, format, arg);
	va_end(arg);

	return status;
})
DEF_STRONG_ALIAS(marcel_sprintf, sprintf)

DEF_MARCEL_PMARCEL_VAARGS(int, fprintf, (FILE *__restrict stream, const char *__restrict format, ...), 
{
	int status;
	va_list arg;

	va_start(arg, format);
	status = marcel_vfprintf(stream, format, arg);
	va_end(arg);

	return status;
})
DEF_STRONG_ALIAS(marcel_fprintf, fprintf)

DEF_MARCEL_PMARCEL(int, vfprintf, (FILE *__restrict stream, const char *__restrict format, va_list arg),
	   (stream, format, arg),
{
	MA_CALL_LIBC_FUNC(int, libc_vfprintf, stream, format, arg);
})
DEF_C(int, vfprintf, (FILE *__restrict stream, const char *__restrict format, va_list arg), (stream, format, arg))

DEF_MARCEL_PMARCEL(int, vprintf, (const char *__restrict format, va_list arg),
	   (format, arg),
{
	return marcel_vfprintf(stdout, format, arg);
})
DEF_C(int, vprintf, (const char *__restrict format, va_list arg), (format, arg))

DEF_MARCEL_PMARCEL(int, vsprintf, (char *__restrict s, const char *__restrict format, va_list arg),
	   (s, format, arg),
{
	MA_CALL_LIBC_FUNC(int, libc_vsprintf, s, format, arg);
})
DEF_C(int, vsprintf, (char *__restrict s, const char *__restrict format, va_list arg), (s, format, arg))



DEF_MARCEL_PMARCEL(int, puts, (const char *s), (s),
{
	/** puts: call puts instead of fputs(s, stdout) because
	 *        it prints a '\n' after s */
	MA_CALL_LIBC_FUNC(int, libc_puts, s);
})
DEF_C(int, puts, (const char *s), (s))

DEF_MARCEL_PMARCEL(int, fputs, (const char *s, FILE *stream), (s, stream),
{
	MA_CALL_LIBC_FUNC(int, libc_fputs, s, stream);
})
DEF_C(int, fputs, (const char *s, FILE *stream), (s, stream))

DEF_MARCEL_PMARCEL(int, putchar, (int c), (c),
{	   
	return marcel_putc(c, stdout);
})
DEF_C(int, putchar, (int c), (c))

DEF_MARCEL_PMARCEL(int, putc,(int c, FILE *stream), (c, stream),
{
	MA_CALL_LIBC_FUNC(int, libc_putc, c, stream);
})
DEF_C(int, putc,(int c, FILE *stream), (c, stream))

DEF_MARCEL_PMARCEL(int, fputc, (int c, FILE *stream), (c, stream),
{
	return marcel_putc(c, stream);
})
DEF_C(int, fputc, (int c, FILE *stream), (c, stream))

DEF_MARCEL_PMARCEL(char*, fgets, (char *__restrict s, int n, FILE *__restrict stream), (s, n, stream),
{
	MA_CALL_LIBC_FUNC(char*, libc_fgets, s, n, stream);
})
DEF_C(char*, fgets, (char *__restrict s, int n, FILE *__restrict stream), (s, n, stream))

DEF_MARCEL_PMARCEL(size_t, fread, (void *ptr, size_t size, size_t nmemb, FILE *stream), 
	   (ptr, size, nmemb, stream),
{
	MA_CALL_LIBC_FUNC(size_t, libc_fread, ptr, size, nmemb, stream);
})
DEF_C(size_t, fread, (void *ptr, size_t size, size_t nmemb, FILE *stream), (ptr, size, nmemb, stream))

DEF_MARCEL_PMARCEL(size_t, fwrite, (const void *ptr, size_t size, size_t nmemb, FILE *stream),
	   (ptr, size, nmemb, stream),
{
	MA_CALL_LIBC_FUNC(size_t, libc_fwrite, ptr, size, nmemb, stream);
})
DEF_C(size_t, fwrite, (const void *ptr, size_t size, size_t nmemb, FILE *stream), (ptr, size, nmemb, stream))

DEF_MARCEL_PMARCEL(FILE*, fopen, (const char *__restrict filename, const char *__restrict mode),
	   (filename, mode),
{
	MA_CALL_LIBC_FUNC(FILE*, libc_fopen, filename, mode);
})
DEF_C(FILE*, fopen, (const char *__restrict filename, const char *__restrict mode), (filename, mode))

DEF_MARCEL_PMARCEL(int, fclose, (FILE *stream), (stream),
{
	MA_CALL_LIBC_FUNC(int, libc_fclose, stream);
})
DEF_C(int, fclose, (FILE *stream), (stream))

DEF_MARCEL_PMARCEL(int, fflush, (FILE *stream), (stream),
{
	MA_CALL_LIBC_FUNC(int, libc_fflush, stream);
})
DEF_C(int, fflush, (FILE *stream), (stream))

DEF_MARCEL_PMARCEL(int, feof, (FILE *stream), (stream),
{
	MA_CALL_LIBC_FUNC(int, libc_feof, stream);
})
DEF_C(int, feof, (FILE *stream), (stream))



DEF_MARCEL_PMARCEL_VAARGS(int, fscanf, (FILE *__restrict stream, const char *__restrict format, ...),
{
	int status;
	va_list arg;

	va_start(arg, format);
	status = marcel_vfscanf(stream, format, arg);
	va_end(arg);

	return status;
})
DEF_STRONG_ALIAS(marcel_fscanf, fscanf)

DEF_MARCEL_PMARCEL(int, vfscanf, (FILE *__restrict stream, const char *__restrict format, va_list arg),
	   (stream, format, arg),
{
	MA_CALL_LIBC_FUNC(int, libc_vfscanf, stream, format, arg);
})
DEF_C(int, vfscanf, (FILE *__restrict stream, const char *__restrict format, va_list arg), (stream, format, arg))

DEF_MARCEL_PMARCEL(int, vscanf, (const char *__restrict format, va_list arg ), (format, arg),
{
	return marcel_vfscanf(stdout, format, arg);
})
DEF_C(int, vscanf, (const char *__restrict format, va_list arg ), (format, arg))

DEF_MARCEL_PMARCEL_VAARGS(int, scanf, (const char *__restrict format, ... ),
{
	int status;
	va_list arg;

	va_start(arg, format);
	status = marcel_vscanf(format, arg);
	va_end(arg);

	return status;
})
DEF_STRONG_ALIAS(marcel_scanf, scanf)

DEF_MARCEL_PMARCEL_VAARGS(int, sscanf, (const char *__restrict s, const char *__restrict format, ... ),
{
	int status;
	va_list arg;

	va_start(arg, format);
	status = marcel_vsscanf(s, format, arg);
	va_end(arg);

	return status;
})
DEF_STRONG_ALIAS(marcel_sscanf, sscanf)

DEF_MARCEL_PMARCEL(int, vsscanf, (const char *__restrict s, const char *__restrict format, va_list arg ), 
	   (s, format, arg),
{
	MA_CALL_LIBC_FUNC(int, libc_vsscanf, s, format, arg);
})
DEF_C(int, vsscanf, (const char *__restrict s, const char *__restrict format, va_list arg ), (s, format, arg))

