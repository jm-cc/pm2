
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
