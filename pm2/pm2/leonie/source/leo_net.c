
/*
 * Leo_net.c
 * =========
 */ 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "leonie.h"

/*
 * Network raw data transfer
 * -------------------------
 */

void
leo_send_block(p_leo_swann_module_t  module,
	       void                 *ptr,
	       size_t                length)
{
  LOG_IN();
  while (!ntbx_tcp_write_poll(1, &module->net_client));
  ntbx_tcp_write_block(module->net_client, ptr, length);
  LOG_OUT();
}

void
leo_receive_block(p_leo_swann_module_t  module,
		  void                 *ptr,
		  size_t                length)
{
  LOG_IN();
  while (!ntbx_tcp_read_poll(1, &module->net_client));
  ntbx_tcp_read_block(module->net_client, ptr, length);
  LOG_OUT();
}

void
leo_send_string(p_leo_swann_module_t  module,
		char                 *data)
{
  ntbx_pack_buffer_t pack_buffer;
  int                len;
  
  LOG_IN();
  len = strlen(data) + 1;
  ntbx_pack_int(len, &pack_buffer);
  leo_send_block(module, pack_buffer.buffer, sizeof(ntbx_pack_buffer_t));
  leo_send_block(module, data, len);
  LOG_OUT();
}

char *
leo_receive_string(p_leo_swann_module_t module)
{
  ntbx_pack_buffer_t  pack_buffer;
  int                 len;
  char               *buffer;
  
  LOG_IN();
  leo_receive_block(module, pack_buffer.buffer, sizeof(ntbx_pack_buffer_t));
  len = ntbx_unpack_int(&pack_buffer);

  buffer = malloc(len);
  CTRL_ALLOC(buffer);
  
  leo_receive_block(module, buffer, len);
  LOG_OUT();
  return buffer;
}

void
leo_send_raw_int(p_leo_swann_module_t module,
		 int                  data)
{
  ntbx_pack_buffer_t  pack_buffer;

  LOG_IN();
  ntbx_pack_int(data, &pack_buffer);
  leo_send_block(module, pack_buffer.buffer, sizeof(ntbx_pack_buffer_t));
  LOG_OUT();
}

int
leo_receive_raw_int(p_leo_swann_module_t module)
{
  ntbx_pack_buffer_t pack_buffer;
  int                data;
  
  LOG_IN();
  leo_receive_block(module, pack_buffer.buffer, sizeof(ntbx_pack_buffer_t));
  data = ntbx_unpack_int(&pack_buffer);
  LOG_OUT();

  return data;
}

void
leo_send_raw_long(p_leo_swann_module_t module,
		  long                 data)
{
  ntbx_pack_buffer_t  pack_buffer;

  LOG_IN();
  ntbx_pack_long(data, &pack_buffer);
  leo_send_block(module, pack_buffer.buffer, sizeof(ntbx_pack_buffer_t));
  LOG_OUT();
}

long
leo_receive_raw_long(p_leo_swann_module_t module)
{
  ntbx_pack_buffer_t pack_buffer;
  long               data;
  
  LOG_IN();
  leo_receive_block(module, pack_buffer.buffer, sizeof(ntbx_pack_buffer_t));
  data = ntbx_unpack_long(&pack_buffer);
  LOG_OUT();

  return data;
}

/*
 * Network data transfer
 * ---------------------
 */
void
leo_send_int(p_leo_swann_module_t module,
	     int                  data)
{
  LOG_IN();
  leo_send_raw_int(module, sizeof(ntbx_pack_buffer_t));
  leo_send_raw_int(module, data);
  LOG_OUT();
}

int
leo_receive_int(p_leo_swann_module_t module)
{
  int check;
  int data;

  LOG_IN();
  check = leo_receive_raw_int(module);
  if (check != sizeof(ntbx_pack_buffer_t))
    FAILURE("invalid data size");

  data = leo_receive_raw_int(module);

  LOG_OUT();
  return data;
}

void
leo_send_long(p_leo_swann_module_t module,
	      long                 data)
{
  LOG_IN();
  leo_send_raw_int(module, sizeof(ntbx_pack_buffer_t));
  leo_send_raw_long(module, data);
  LOG_OUT();
}

long
leo_receive_long(p_leo_swann_module_t module)
{
  int check;
  int data;

  LOG_IN();
  check = leo_receive_raw_int(module);
  if (check != sizeof(ntbx_pack_buffer_t))
    FAILURE("invalid data size");

  data = leo_receive_raw_long(module);

  LOG_OUT();
  return data;
}

void
leo_send_command(p_leo_swann_module_t module,
		 ntbx_command_code_t  code)
{
  LOG_IN();
  leo_send_raw_int(module, module->id);
  leo_send_int(module, code);
  LOG_OUT();
}

/*
 * Net server
 * ----------
 */
p_ntbx_server_t
leo_net_server_init(void)
{
  p_ntbx_server_t net_server = NULL;
 
  net_server = malloc(sizeof(ntbx_server_t));
  CTRL_ALLOC(net_server);
  net_server->state = ntbx_server_state_uninitialized;
  ntbx_tcp_server_init(net_server);
  DISP("net server ready and listening at %s",
       net_server->connection_data.data);

  return net_server;
}
