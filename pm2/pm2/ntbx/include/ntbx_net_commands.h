
/*
 * ntbx_net_commands.h
 * ===================
 */

#ifndef NTBX_NET_COMMANDS_H
#define NTBX_NET_COMMANDS_H

typedef enum
{
  /* synchronization */
  ntbx_command_terminate = 0,
  ntbx_command_completed,

  /* file management */
  ntbx_command_open_file,
  ntbx_command_create_file,
  ntbx_command_close_file,
  ntbx_command_destroy_file,
  ntbx_command_read_file_block,
  ntbx_command_write_file_block,

  /* remote command execution */
  ntbx_command_exec,
  ntbx_command_swann_spawn,
  ntbx_command_mad_spawn,

  /* data transfer */
  ntbx_command_send_data_block,
  ntbx_command_receive_data_block,
} ntbx_command_code_t, *p_ntbx_command_code_t;

#endif /* NTBX_NET_COMMANDS_H */
