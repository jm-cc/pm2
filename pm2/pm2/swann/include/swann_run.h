
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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
 * swann_run.h
 * -----------
 */

#ifndef __SWANN_RUN_H
#define __SWANN_RUN_H

typedef enum
{
  swann_command_state_running = 0,
  swann_command_state_finished
} swann_command_state_t, *p_swann_command_state_t;

typedef struct s_swann_command
{
  swann_command_state_t  command_state;
  p_swann_file_t         file;
  char                 **argv;
  char                 **envp;
  pid_t                  pid;
  int                    return_value;
  p_swann_file_t         file_in;
  p_swann_file_t         file_out;
  p_swann_file_t         file_err;
} swann_command_t;

#endif /* __SWANN_RUN_H */
