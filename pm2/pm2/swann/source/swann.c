
/*
 * swann.c
 * =======
 */ 
#define DEBUG
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "swann.h"

/*
 * Commands
 * --------
 */
static void
swann_command_open_file(p_swann_net_server_t net_server)
{
  p_swann_file_t  file;
  char           *file_name;
  
  LOG_IN();
  file_name = swann_net_receive_string(net_server->controler->client);

  file = malloc(sizeof(swann_file_t));
  CTRL_ALLOC(file);
  file->state = swann_file_state_uninitialized;
  
  swann_file_init(file, file_name);
  swann_file_open(file);

  swann_net_send_long(net_server->controler->client, (long)file);
  free(file_name);
  LOG_OUT();
}

static void
swann_command_create_file(p_swann_net_server_t net_server)
{
  p_swann_file_t  file;
  char           *file_name;
  
  LOG_IN();
  file_name = swann_net_receive_string(net_server->controler->client);

  file = malloc(sizeof(swann_file_t));
  CTRL_ALLOC(file);
  file->state = swann_file_state_uninitialized;
  
  swann_file_init(file, file_name);
  swann_file_create(file);

  swann_net_send_long(net_server->controler->client, (long)file);
  free(file_name);
  LOG_OUT();
}

static void
swann_command_close_file(p_swann_net_server_t net_server)
{
  p_swann_file_t file;
  
  LOG_IN();
  file = (p_swann_file_t)swann_net_receive_long(net_server->controler->client);
  swann_file_close(file);
  LOG_OUT();
}

static void
swann_command_destroy_file(p_swann_net_server_t net_server)
{
  p_swann_file_t file;
  
  LOG_IN();
  file = (p_swann_file_t)swann_net_receive_long(net_server->controler->client);
  swann_file_destroy(file);
  free(file);
  LOG_OUT();
}

static void
swann_command_read_file_block(p_swann_net_server_t net_server)
{
  p_swann_file_t file;
  
  LOG_IN();
  file = (p_swann_file_t)swann_net_receive_long(net_server->controler->client);
  LOG_PTR("File handle", file);
  swann_file_send_block(net_server->controler, file);
  LOG_OUT();
}

static void
swann_command_write_file_block(p_swann_net_server_t net_server)
{
  p_swann_file_t file;
  
  LOG_IN();
  file = (p_swann_file_t)swann_net_receive_long(net_server->controler->client);
  swann_file_receive_block(net_server->controler, file);
  LOG_OUT();
}

static void
swann_command_exec(p_swann_net_server_t net_server)
{
  FAILURE("unimplemented function");
}

static void
swann_command_swann_spawn(p_swann_net_server_t net_server)
{
  FAILURE("unimplemented function");
}

static void
swann_command_mad_spawn(p_swann_net_server_t net_server)
{
  FAILURE("unimplemented function");
}

static void
swann_command_send_data_block(p_swann_net_server_t net_server)
{
  FAILURE("unimplemented function");
}

static void
swann_command_receive_data_block(p_swann_net_server_t net_server)
{
  FAILURE("unimplemented function");
}

/*
 * Command processor
 * -----------------
 */
static void
swann_command_processor(p_swann_net_server_t net_server)
{
  p_ntbx_client_t controler = net_server->controler->client;
  tbx_bool_t      fin       = tbx_false;

  LOG_IN();
  while(!fin)
    {
      ntbx_pack_buffer_t    pack_buffer;
      ntbx_command_code_t   command;
      size_t                length;
      swann_net_server_id_t address;
      
      while (!ntbx_tcp_read_poll(1, &controler));
      ntbx_tcp_read_block(controler, &pack_buffer, sizeof(ntbx_pack_buffer_t));
      address = (swann_net_server_id_t)ntbx_unpack_int(&pack_buffer);

      DISP_VAL("Address", address);

      while (!ntbx_tcp_read_poll(1, &controler));
      ntbx_tcp_read_block(controler, &pack_buffer, sizeof(ntbx_pack_buffer_t));
      length = (size_t)ntbx_unpack_int(&pack_buffer);

      if (address == net_server->id)
	{
	  if (length != sizeof(ntbx_pack_buffer_t))
	    FAILURE("invalid command packet size");
	  
	  while (!ntbx_tcp_read_poll(1, &controler));
	  ntbx_tcp_read_block(controler,
			      &pack_buffer,
			      sizeof(ntbx_pack_buffer_t));
	  command = (ntbx_command_code_t)ntbx_unpack_int(&pack_buffer);

	  switch (command)
	    {
	      /* synchronization */
	    case ntbx_command_terminate:
	      {
		fin = tbx_true;
		break;
	      }
	    case ntbx_command_completed:
	      {
		break;
	      }

	      /* file management */
	    case ntbx_command_open_file:
	      {
		swann_command_open_file(net_server);
		break;
	      }
	    case ntbx_command_create_file:
	      {
		swann_command_create_file(net_server);
		break;
	      }
	    case ntbx_command_close_file:
	      {
		swann_command_close_file(net_server);
		break;
	      }
	    case ntbx_command_destroy_file:
	      {
		swann_command_destroy_file(net_server);
		break;
	      }
	    case ntbx_command_read_file_block:
	      {
		swann_command_read_file_block(net_server);
		break;
	      }
	    case ntbx_command_write_file_block:
	      {
		swann_command_write_file_block(net_server);
		break;
	      }
	  
	      /* remote command execution */
	    case ntbx_command_exec:
	      {
		swann_command_exec(net_server);
		FAILURE("unimplemented feature");
		break;
	      }
	    case ntbx_command_swann_spawn:
	      {
		swann_command_swann_spawn(net_server);
		break;
	      }
	    case ntbx_command_mad_spawn:
	      {
		swann_command_mad_spawn(net_server);
		break;
	      }
	  
	      /* data transfer */
	    case ntbx_command_send_data_block:
	      {
		swann_command_send_data_block(net_server);
		break;
	      }
	    case ntbx_command_receive_data_block:
	      {
		swann_command_receive_data_block(net_server);
		break;
	      }

	      /* default */
	    default:
	      {
		FAILURE("unknown command code");
	      }
	    }
	}
      else
	{
	  /* Routing/forwarding needed */
	  FAILURE("Routing/forwarding not implemented");
	}
    }
  LOG_OUT();
}


/*
 * Initialization
 * --------------
 */
int 
main(int   argc   __attribute__((unused)),
     char *argv[] __attribute__((unused)))
{
  p_swann_net_server_t   net_server         = NULL;
  int                    param_id           = 0;
  swann_net_server_id_t  id                 = -1;
  int                    param_master       = 0;
  char                  *master_host_name   = NULL;
  int                    param_cnx          = 0;
  ntbx_connection_data_t connection_data    = {0};

  /* Toolboxes initialization */
  tbx_init();
  ntbx_init();

  /* Command line parsing */
  {
    int i;

    i = 1;
    
    while (i < argc)
      {
	if(!strcmp(argv[i], "-id"))
	{
	  if (param_id)
	    FAILURE("id parameter already specified");

	  param_id++;

	  if (i == (argc - 1))
	    FAILURE("-id option must be followed "
		    "by the id of the process");

	  id = atoi(argv[i + 1]);
	  i++;
	}
      else if (!strcmp(argv[i], "-cnx"))
	{
	  if (param_cnx)
	    FAILURE("cnx parameter already specified");
	  
	  param_cnx++;

	  if (i == (argc - 1))
	    FAILURE("-cnx must be followed "
		    "by the master node connection parameter");

	  strcpy(connection_data, argv[i + 1]);
	  i++;
	}
      else if (!strcmp(argv[i], "-master"))
	{
	  if (param_master)
	    FAILURE("master parameter already specified");

	  param_master++;
	  if(i == (argc - 1))
	    FAILURE(" -master must be followed by the master host name");

	  master_host_name = TBX_MALLOC(strlen(argv[i + 1]) + 1);
	  CTRL_ALLOC(master_host_name);
	    
	  strcpy(master_host_name,
		 argv[i + 1]);
	  i++;
	}
      else
	FAILURE("invalid parameter");
	
      i++;
    }
  }

    {
      char   output[1024];
      int    f;

      sprintf(output, "/tmp/%s-swannlog-%d", getenv("USER"), (int)id);
      f = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
      dup2(f, STDOUT_FILENO);
      dup2(STDOUT_FILENO, STDERR_FILENO);
    }

  /* Net server initialization */
  net_server = swann_net_server_init(id,
				     master_host_name,
				     &connection_data);
  swann_command_processor(net_server);

  return 0;
}
