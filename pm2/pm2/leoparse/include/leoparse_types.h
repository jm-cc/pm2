/*
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

 * leoparse_types.h
 * ----------------
 */

#ifndef __LEOPARSE_TYPES_H
#define __LEOPARSE_TYPES_H

typedef enum e_leoparse_entry_type
{
  leoparse_e_undefined = 0,
  leoparse_e_slist,
  leoparse_e_object,
} leoparse_entry_type_t;

typedef struct s_leoparse_htable_entry
{
  char                  *id;
  p_leoparse_object_t    object;
  p_tbx_slist_t          slist;
  leoparse_entry_type_t  type;
} leoparse_htable_entry_t;

typedef enum e_leoparse_object_type
{
  leoparse_o_undefined = 0,
  leoparse_o_id,
  leoparse_o_string,
  leoparse_o_htable,
} leoparse_object_type_t;

typedef struct s_leoparse_object
{
  leoparse_object_type_t  type;
  p_tbx_htable_t          htable;
  char                   *string;
  char                   *id;
} leoparse_object_t;

#endif /* __LEOPARSE_TYPES_H */
