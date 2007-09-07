
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
 * Mad_leonie_commands.h
 * =====================
 */

#ifndef MAD_LEONIE_COMMANDS_H
#define MAD_LEONIE_COMMANDS_H

// Commands
//----------///////////////////////////////////////////////////////////////

typedef enum e_mad_leo_command
{
  mad_leo_command_end		= 0,
  mad_leo_command_end_ack	= 1,
  mad_leo_command_print		= 2,
  mad_leo_command_barrier	= 3,
  mad_leo_command_barrier_passed= 4,
  mad_leo_command_beat		= 5,
  mad_leo_command_beat_ack	= 6,
  mad_leo_command_session_added,
  mad_leo_command_update_dir,
  mad_leo_command_merge_channel,
  mad_leo_command_split_channel,
  mad_leo_command_shutdown_channel,
} mad_leo_command_t, *p_mad_leo_command_t;

#endif // MAD_LEONIE_COMMANDS_H
