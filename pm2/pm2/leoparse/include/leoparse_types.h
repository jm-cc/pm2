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
 * leoparse_types.h
 * ----------------
 */

#ifndef LEOPARSE_TYPES_H
#define LEOPARSE_TYPES_H

typedef struct s_leoparse_range
{
  int begin;
  int end;
} leoparse_range_t;

typedef enum e_leoparse_object_type
{
  leoparse_o_undefined = 0,
  leoparse_o_id,
  leoparse_o_string,
  leoparse_o_htable,
  leoparse_o_integer,
  leoparse_o_range,
  leoparse_o_slist,
} leoparse_object_type_t;

typedef struct s_leoparse_object
{
  leoparse_object_type_t  type;
  p_tbx_htable_t          htable;
  char                   *string;
  char                   *id;
  int                     val;
  p_leoparse_range_t      range;
  p_leoparse_modifier_t   modifier;
  p_tbx_slist_t           slist;
} leoparse_object_t;

typedef enum e_leoparse_modifier_type
{
  leoparse_m_undefined = 0,
  leoparse_m_sbracket,
  leoparse_m_parenthesis,
} leoparse_modifier_type_t;

typedef struct s_leoparse_modifier
{
  leoparse_modifier_type_t type;
  p_tbx_slist_t            sbracket;
  p_tbx_slist_t            parenthesis;
} leoparse_modifier_t;

#endif /* LEOPARSE_TYPES_H */
