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
