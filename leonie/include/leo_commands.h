#ifndef LEO_COMMANDS_H
#define LEO_COMMANDS_H

// Commands
//----------///////////////////////////////////////////////////////////////

typedef enum e_leo_command
{
  leo_command_end = 0,
  leo_command_end_ack,
  leo_command_print,
  leo_command_barrier,
  leo_command_barrier_passed,
} leo_command_t, *p_leo_command_t;

#endif // LEO_COMMANDS_H
