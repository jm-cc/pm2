#ifndef MAD_LEONIE_COMMANDS_H
#define MAD_LEONIE_COMMANDS_H

// Commands
//----------///////////////////////////////////////////////////////////////

typedef enum e_mad_leo_command
{
  mad_leo_command_end = 0,
  mad_leo_command_end_ack,
  mad_leo_command_print,
  mad_leo_command_barrier,
  mad_leo_command_barrier_passed,
} mad_leo_command_t, *p_mad_leo_command_t;

#endif // MAD_LEONIE_COMMANDS_H
