/*! \file tbx_snprintf.h
 *  \brief TBX own simple and recursive snprintf
 *
 *  This file contains a simple and recursive snprintf, for internal debugging
 *  use, to avoid using libc's one.
 * 
 */


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

/*
 * Tbx_list.h
 * ==========
 */

#ifndef TBX_SNPRINTF_H
#define TBX_SNPRINTF_H

#include <stdarg.h>		/* Pour va_list */
#include <stdlib.h>		/* Pour size_t */

#include "tbx_compiler.h"

/** \defgroup s(n)printf_interface s(n)printf interface
 *
 * This is the s(n)printf interface
 */

int tbx_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int tbx_vsprintf(char *buf, const char *fmt, va_list args);
int tbx_snprintf(char *buf, size_t size, const char *fmt, ...);
int tbx_sprintf(char *buf, const char *fmt, ...);


#endif				/* TBX_SNPRINTF_H */
