
/*
 * Mad_leonie_spawn.c
 * ==================
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <errno.h>
#include <netdb.h>
#ifdef PM2
#include <sys/wait.h>
#endif /* PM2 */

/* #define DEBUG */
/* #define TIMING */
#include "madeleine.h"

#ifdef LEONIE_SPAWN

void
mad_master_link_init(p_mad_madeleine_t   madeleine,
		     int                 argc,
		     char              **argv)
{
  char            *leonie_server_host_name = NULL;
  char            *leonie_server_port      = NULL;
  p_ntbx_client_t  client                  = madeleine->master_link;

  LOG_IN();
  argc--; argv++;

  while (argc)
    {
      if (!strcmp(*argv, "-leonie"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("leonie argument not found");

	  if (!leonie_server_host_name)
	    {
	      leonie_server_host_name = *argv;
	    }
	  else
	    FAILURE("too much leonie arguments");
	}
      else if (!strcmp(*argv, "-link"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("link argument not found");

	  if (!leonie_server_port)
	    {
	      leonie_server_port = *argv;
	    }
	  else
	    FAILURE("too much link arguments");
	}
      
      argc--; argv++;
    }

  if (!leonie_server_host_name && !leonie_server_port)
    FAILURE("leonie server connect information unavailable");
  
  // Master link setup
  {
    int status = ntbx_failure;
    ntbx_connection_data_t data;

    strcpy(&(data.data), leonie_server_port);

    status = ntbx_tcp_client_connect(client,
				     leonie_server_host_name,
				     &data);

    if (status == ntbx_failure)
      FAILURE("could not setup the master link");

    DISP("Master link is up");
  }
  
  LOG_OUT();
}

void
mad_master_link_get_info(p_mad_madeleine_t   madeleine,
			 int                 argc,
			 char              **argv)
{
  p_mad_configuration_t configuration = madeleine->configuration;
  p_ntbx_client_t       client        = madeleine->master_link;
  int                   status        = ntbx_failure;
  ntbx_pack_buffer_t    buffer;

  LOG_IN();
  // Get the configuration size
  status = ntbx_btcp_read_pack_buffer(client, &buffer);
  if (status == ntbx_failure)
    FAILURE("master link failure");

  configuration->size = ntbx_unpack_int(&buffer);
  DISP_VAL("Received configuration size", configuration->size);
  
  // Get the node rank
  status = ntbx_btcp_read_pack_buffer(client, &buffer);
  if (status == ntbx_failure)
    FAILURE("master link failure");
  
  configuration->local_host_id = ntbx_unpack_int(&buffer);
  DISP_VAL("Received configuration rank", configuration->local_host_id);
  LOG_OUT();
}

void
mad_master_link_get_adapters(p_mad_madeleine_t   madeleine,
			     int                 argc,
			     char              **argv)
{
  int             nb_adapters = 0;
  p_ntbx_client_t client      = madeleine->master_link;
  int             status      = ntbx_failure;
  ntbx_pack_buffer_t buffer;
  int                i;

  LOG_IN();
  status = ntbx_btcp_read_pack_buffer(client, &buffer);
  if (status == ntbx_failure)
    FAILURE("master link failure");

  nb_adapters = ntbx_unpack_int(&buffer);
  DISP_VAL("Received adapter number", nb_adapters);

  i = nb_adapters;
  while (i--)
    {
      char *name = NULL;

      status = ntbx_btcp_read_string(client, &name);
      
      DISP_STR("Received an adapter name", name);
      TBX_FREE(name);
    }

  i = nb_adapters;
  while (i--)
    {
      char *name = NULL;
      int   size =    0;
      int   rank =   -1;

      status = ntbx_btcp_read_string(client, &name);
      if (status == ntbx_failure)
	FAILURE("master link failure");
      
      DISP_STR("Received an adapter name", name);
      TBX_FREE(name);

      status = ntbx_btcp_read_pack_buffer(client, &buffer);
      if (status == ntbx_failure)
	FAILURE("master link failure");

      size = ntbx_unpack_int(&buffer);
      DISP_VAL("Adapter - configuration size", size);

      status = ntbx_btcp_read_pack_buffer(client, &buffer);
      if (status == ntbx_failure)
	FAILURE("master link failure");

      rank = ntbx_unpack_int(&buffer);
      DISP_VAL("Adapter - node rank", rank);

      ntbx_pack_int(rank, &buffer);
      status = ntbx_btcp_write_pack_buffer(client, &buffer);

      if (status == ntbx_failure)
	FAILURE("master link failure");
    }
  LOG_OUT();
}

void
mad_master_link_get_channels(p_mad_madeleine_t   madeleine,
			     int                 argc,
			     char              **argv)
{
  int             nb_channels = 0;
  p_ntbx_client_t client      = madeleine->master_link;
  int             status      = ntbx_failure;
  ntbx_pack_buffer_t buffer;
  int                i;

  LOG_IN();
  status = ntbx_btcp_read_pack_buffer(client, &buffer);
  if (status == ntbx_failure)
    FAILURE("master link failure");

  nb_channels = ntbx_unpack_int(&buffer);
  DISP_VAL("Received channels number", nb_channels);

  i = nb_channels;
  while (i--)
    {
      char *name = NULL;

      status = ntbx_btcp_read_string(client, &name);
      
      DISP_STR("Received an channel name", name);
      TBX_FREE(name);
    }

  i = nb_channels;
  while (i--)
    {
      char *name = NULL;
      char *dev  = NULL;
      int   size =    0;
      int   rank =   -1;

      status = ntbx_btcp_read_string(client, &name);
      if (status == ntbx_failure)
	FAILURE("master link failure");      
      
      DISP_STR("Received the channel name", name);
      TBX_FREE(name);

      status = ntbx_btcp_read_pack_buffer(client, &buffer);
      if (status == ntbx_failure)
	FAILURE("master link failure");

      size = ntbx_unpack_int(&buffer);
      DISP_VAL("Channel - configuration size", size);

      status = ntbx_btcp_read_pack_buffer(client, &buffer);
      if (status == ntbx_failure)
	FAILURE("master link failure");

      rank = ntbx_unpack_int(&buffer);
      DISP_VAL("Channel - node rank", rank);

      status = ntbx_btcp_read_string(client, &dev);
      if (status == ntbx_failure)
	FAILURE("master link failure");      
      
      DISP_STR("Received the channel device", dev);
      TBX_FREE(name);

      ntbx_pack_int(rank, &buffer);
      status = ntbx_btcp_write_pack_buffer(client, &buffer);

      if (status == ntbx_failure)
	FAILURE("master link failure");
    }
  LOG_OUT();
}

p_mad_madeleine_t
mad_init(int   *argc,
	 char **argv)
{
  p_mad_madeleine_t madeleine = NULL;
  
  LOG_IN();
  fprintf(stderr, "Leonie spawn initialization\n");
  
#ifdef PM2_DEBUG
  pm2debug_init_ext(argc, argv, PM2DEBUG_DO_OPT);  
#endif /* PM2_DEBUG */

  tbx_init(*argc, argv);
  ntbx_init(*argc, argv);
  
  mad_memory_manager_init(*argc, argv);

  /*
   * BEGIN: Madeleine initialisation
   * -------------------------------
   */

  madeleine = mad_object_init(*argc, argv, NULL, NULL);

  mad_cmd_line_init(madeleine, *argc, argv);
  mad_master_link_init(madeleine, *argc, argv);
  mad_master_link_get_info(madeleine, *argc, argv);
  mad_master_link_get_adapters(madeleine, *argc, argv);
  mad_master_link_get_channels(madeleine, *argc, argv);

  {
    ntbx_tcp_client_disconnect(madeleine->master_link);
    exit(7);
  }
  
  mad_output_redirection_init(madeleine, *argc, argv);
  mad_configuration_init(madeleine, *argc, argv);
  mad_network_components_init(madeleine, *argc, argv);

  /*
   * END: Madeleine initialisation
   * -----------------------------
   */

  mad_purge_command_line(madeleine, argc, argv);

  ntbx_purge_cmd_line(argc, argv);
  tbx_purge_cmd_line(argc, argv);

#ifdef PM2_DEBUG
  pm2debug_init_ext(argc, argv, PM2DEBUG_CLEAROPT);  
#endif /* PM2_DEBUG */

  LOG_OUT();

  return madeleine;
}

#endif /* LEONIE_SPAWN */
