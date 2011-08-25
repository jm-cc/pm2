/*! \file tbx_string.h
 *  \brief TBX string object data structures.
 *
 *  This file contains the TBX string object data structures.
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
 * Tbx_string.h : string of characters
 * ===========------------------------
 */

#ifndef TBX_STRING_H
#define TBX_STRING_H

/** \defgroup string_interface string list interface
 *
 * This is the string list interface
 *
 * @{
 */

/*
 * Data structures
 * ---------------
 */
typedef struct s_tbx_string {
	size_t length;
	size_t allocated_length;
	char *data;
} tbx_string_t;

/* @} */

#endif				/* TBX_STRING_H */
