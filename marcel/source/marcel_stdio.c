
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

#include "marcel.h"

#include <stdarg.h>

#ifdef MARCEL_DONT_USE_POSIX_THREADS
static marcel_mutex_t ma_io_lock=MARCEL_MUTEX_INITIALIZER;
#endif

static __tbx_inline__ void io_lock()
{
#ifdef MARCEL_DONT_USE_POSIX_THREADS
	marcel_mutex_lock(&ma_io_lock);
#else
	marcel_extlib_protect();
#endif
}

static __tbx_inline__ void io_unlock()
{
#ifdef MARCEL_DONT_USE_POSIX_THREADS
	marcel_mutex_unlock(&ma_io_lock);
#else
	marcel_extlib_unprotect();
#endif
}

int marcel_printf(const char *__restrict format, ...)
{
	va_list args;
	int retour;

	io_lock();

	va_start(args, format);
	retour = vprintf(format, args);
	va_end(args);

	io_unlock();

	return retour;
}

int marcel_fprintf(FILE * __restrict stream, const char *__restrict format, ...)
{
	va_list args;
	int retour;

	io_lock();

	va_start(args, format);
	retour = vfprintf(stream, format, args);
	va_end(args);

	io_unlock();
	return retour;
}

int marcel_sprintf(char *__restrict string, const char *__restrict format, ...)
{
	va_list args;
	int retour;

	io_lock();

	va_start(args, format);
	retour = vsprintf(string, format, args);
	va_end(args);

	io_unlock();
	return retour;
}

int marcel_snprintf(char *__restrict string, size_t size,
    const char *__restrict format, ...)
{
	va_list args;
	int retour;

	io_lock();

	va_start(args, format);
	retour = vsnprintf(string, size, format, args);
	va_end(args);

	io_unlock();
	return retour;
}

FILE *marcel_fopen(const char *__restrict path, const char *__restrict mode)
{
	FILE *file;

	io_lock();
	file = fopen(path, mode);
	io_unlock();
	return file;
}

int marcel_fclose(FILE * stream)
{
	int retour;

	io_lock();
	retour = fclose(stream);
	io_unlock();
	return retour;
}

int marcel_fflush(FILE * stream)
{
	int retour;

	io_lock();
	retour = fflush(stream);
	io_unlock();
	return retour;
}

int marcel_scanf(const char *__restrict format, ...) {
	va_list args;
	int retour;

	io_lock();

	va_start(args, format);
	retour = vscanf(format, args);
	va_end(args);

	io_unlock();
	return retour;
}

int marcel_fscanf(FILE * __restrict stream, const char *__restrict format, ...) {
	va_list args;
	int retour;

	io_lock();

	va_start(args, format);
	retour = vfscanf(stream, format, args);
	va_end(args);

	io_unlock();
	return retour;
}
