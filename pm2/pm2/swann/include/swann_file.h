
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
 * swann_file.h
 * ------------
 */

#ifndef SWANN_FILE_H
#define SWANN_FILE_H

typedef enum
{
  swann_file_state_uninitialized = 0,
  swann_file_state_initialized,
  swann_file_state_open,
  swann_file_state_closed,
} swann_file_state_t, *p_swann_file_state_t;

typedef enum
{
  swann_file_mode_uninitialized,
  swann_file_mode_read,
  swann_file_mode_write,
  swann_file_mode_exec,
} swann_file_mode_t, *p_swann_file_mode_t;

typedef struct s_swann_file
{
  char               *pathname;
  int                 descriptor;
  swann_file_state_t  state;
  swann_file_mode_t   mode;
  tbx_bool_t          exist;
  tbx_bool_t          readable;
  tbx_bool_t          writeable;
  tbx_bool_t          executable;
} swann_file_t;


#endif /* __SWANN_FILE_H */
