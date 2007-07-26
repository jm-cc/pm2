/*! \file tbx_darray.h
 *  \brief TBX dynamic array structures
 *
 *  This file defines TBX dynamic uni-dimentionnal arrays data structures
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
 * Tbx_darray.h : string of characters
 * ============-----------------------
 */

#ifndef TBX_DARRAY_H
#define TBX_DARRAY_H

/** \defgroup darray_interface dynamic array interface
 *
 * This is the dynamic array interface
 *
 * @{
 */

/*
 * Data structures 
 * ---------------
 */
typedef struct s_tbx_darray
{
  TBX_SHARED;
  tbx_darray_index_t   length;
  tbx_darray_index_t   allocated_length;
  void               **data;
} tbx_darray_t;

/* @} */

#endif /* TBX_DARRAY_H */
