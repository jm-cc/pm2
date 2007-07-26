/*! \file tbx_htable_management.c
 *  \brief TBX hash-table management routines
 *
 *  This file implements hash-table TBX associative arrays.
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
 * Tbx_htable.h : hash-table
 * ============----------------
 */

#ifndef TBX_HTABLE_H
#define TBX_HTABLE_H

/** \defgroup htable_interface hash-table interface
 *
 * This is the hash-table interface
 *
 * @{
 */

/*
 * Data types
 * ----------
 */
typedef int                        tbx_htable_bucket_count_t;
typedef tbx_htable_bucket_count_t *p_tbx_htable_bucket_count_t;

typedef int                         tbx_htable_element_count_t;
typedef tbx_htable_element_count_t *p_tbx_htable_element_count_t;

/*
 * Data structures
 * ---------------
 */
typedef struct s_tbx_htable_element
{
  char                   *key;
  size_t		  key_len;
  void                   *object;
  p_tbx_htable_element_t  next;
} tbx_htable_element_t;

typedef struct s_tbx_htable
{
  TBX_SHARED;
  tbx_htable_bucket_count_t   nb_bucket;
  p_tbx_htable_element_t     *bucket_array;
  tbx_htable_element_count_t  nb_element;
} tbx_htable_t;

/* @} */

#endif /* TBX_HTABLE_H */
