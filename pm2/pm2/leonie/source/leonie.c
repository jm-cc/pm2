/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: leonie.c,v $
Revision 1.14  2001/01/29 16:59:33  oaumage
- transmission des donnees de configuration


______________________________________________________________________________
*/

/*
 * leonie.c
 * ========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

/*
 * Data Structures
 * ===============
 */
typedef struct s_args
{
  int    argc;
  char **argv;
} args_t, *p_args_t;

typedef struct s_node
{
  pid_t            pid;
  char            *host;
  p_ntbx_client_t  client;
  p_tbx_htable_t   networks;
  p_tbx_slist_t    network_slist;
  p_tbx_htable_t   channels;
  p_tbx_slist_t    channel_slist;
} node_t, *p_node_t;

typedef struct s_device
{
  char          *name;
  p_tbx_slist_t  nodes;
} device_t, *p_device_t;

/*
 * Static variables
 * ================
 */

/*
 * PM2 scripts
 * -----------
 */
static const char *pm2_env                = "env";
static const char *leo_pm2load            = "leo-pm2load";
static const char *pm2_load_flavor_option = "-f";
static const char *pm2_rsh                = NULL;

/*
 * Usage
 * -----
 */
static const char *usage_str = "leonie <net_conf_file> <app_conf_file>";

/*
 * Launched nodes
 * --------------
 */
static p_tbx_htable_t nodes      = NULL;
static p_tbx_slist_t  node_slist = NULL;

/*
 * Topology
 * --------
 */
static p_tbx_htable_t networks      = NULL;
static p_tbx_slist_t  network_slist = NULL;
static p_tbx_htable_t devices       = NULL;
static p_tbx_slist_t  device_slist  = NULL;
static p_tbx_htable_t channels      = NULL;
static p_tbx_slist_t  channel_slist = NULL;

/*
 * Net server
 * ----------
 */
static p_ntbx_server_t net_server = NULL;

/*
 * Application information
 * -----------------------
 */
static char           *application_name     = NULL;
static char           *application_flavor   = NULL;
static p_tbx_htable_t  application_networks = NULL;
static p_tbx_slist_t   include_slist        = NULL;


/*
 * Functions
 * =========
 */
static
void
usage(void)
{
  fprintf(stderr, "Usage: %s\n", usage_str);
  exit(EXIT_FAILURE);
}

static
void
terminate(char *msg)
{
  fprintf(stderr, "error: %s\n", msg);
  exit(EXIT_FAILURE);
}

static
void
error(char *command)
{
  perror(command);
  exit(EXIT_FAILURE);
}

static
void
process_include_file(p_tbx_htable_t net_inc)
{
  p_tbx_slist_t inc_network_slist = NULL;

  LOG_IN();
  inc_network_slist = leoparse_read_slist(net_inc, "networks");
  if (inc_network_slist && !tbx_slist_is_nil(inc_network_slist))
    {
      tbx_slist_merge_after(network_slist, inc_network_slist);

      do
	{
	  p_leoparse_object_t  object        = NULL;
	  p_tbx_htable_t       network_entry = NULL;
	  char                *name          = NULL;
	  
	  object = tbx_slist_remove_from_head(inc_network_slist);
	  network_entry = leoparse_get_htable(object);
	  
	  name = leoparse_read_id(network_entry, "name");
	  tbx_htable_add(networks, name, network_entry);
	}
      while(!tbx_slist_is_nil(inc_network_slist));      
    }
  LOG_OUT();
}

static
void
include_network_files(p_tbx_slist_t include_slist)
{
  LOG_IN();
  tbx_slist_ref_to_head(include_slist);
      
  do
    {
      p_leoparse_object_t  object   = NULL;
      p_tbx_htable_t       net_inc  = NULL;
      char                *filename = NULL;
      
      object = tbx_slist_ref_get(include_slist);
      filename = leoparse_get_id(object);

      net_inc = leoparse_parse_local_file(filename);
      process_include_file(net_inc);
      free(net_inc);
      net_inc = NULL;
    }
  while (tbx_slist_ref_forward(include_slist));
  LOG_OUT();
}

static
pid_t
call_execvp(p_args_t args)
{
  pid_t pid = -1;

  LOG_IN();

  LOG("Fork");
  pid = fork();

  if (pid == -1)
    {
      error("fork");
    }

  if (!pid)
    {
      LOG_STR("execvp: ", args->argv[0]);
#ifdef DEBUG
      
#endif /* DEBUG */
      execvp(args->argv[0], args->argv);
      error("execvp");
    }
  
  LOG_OUT();
  return pid;
}

static
void
leo_args_init(p_args_t    args,
	      const char *command)
{
  const char *ptr = NULL;

  LOG_IN();
  args->argc = 1;
  args->argv = malloc ((1 + args->argc) * sizeof(char *));
  CTRL_ALLOC(args->argv);

  while ((ptr = strchr(command, ' ')))
    {
      size_t len = ptr - command;

      args->argv = realloc (args->argv, (2 + args->argc) * sizeof(char *));
      CTRL_ALLOC(args->argv);
      
      args->argv[args->argc - 1] = malloc(len + 1);
      CTRL_ALLOC(args->argv[args->argc - 1]);
      
      strncpy(args->argv[args->argc - 1], command, len);
      args->argv[args->argc - 1][len] = 0;
      
      command = ptr + 1;
      args->argc++;
    }

  args->argv[args->argc - 1] = malloc(strlen(command) + 1);
  CTRL_ALLOC(args->argv[args->argc - 1]);
  strcpy(args->argv[args->argc - 1], command);
  
  args->argv[args->argc] = NULL;
  LOG_OUT();
}

static
void
leo_args_append(p_args_t    args,
		const char *arg)
{
  LOG_IN();
  args->argc++;
  args->argv = realloc (args->argv, (1 + args->argc) * sizeof(char *));
  CTRL_ALLOC(args->argv);

  args->argv[args->argc - 1] = malloc(strlen(arg) + 1);
  CTRL_ALLOC(args->argv[args->argc - 1]);
  strcpy(args->argv[args->argc - 1], arg);
  
  args->argv[args->argc] = NULL;
  LOG_OUT();
}

static
void
leo_args_append_environment(p_args_t  args,
			    char     *variable_name)
{
  char *variable_content = NULL;

  LOG_IN();
  if ((variable_content = getenv(variable_name)))
    {
      char *arg = NULL;
      
      arg = malloc(strlen(variable_content) + strlen(variable_name) + 4);
      CTRL_ALLOC(arg);
      
      sprintf(arg, "%s='%s'", variable_name, variable_content);
      leo_args_append(args, arg);
    }
  LOG_OUT();
}

void
launch(char *name,
       char *flavor,
       char *host)
{
  p_args_t args = NULL;
  p_node_t node = NULL;

  LOG_IN();
  args = malloc(sizeof(args_t));
  CTRL_ALLOC(args);
  
  leo_args_init(args, pm2_rsh);
  leo_args_append(args, host);
  leo_args_append(args, pm2_env);
  leo_args_append_environment(args, "PATH");
  leo_args_append_environment(args, "PM2_RSH");
  leo_args_append_environment(args, "PM2_ROOT");
  leo_args_append(args, leo_pm2load);
  leo_args_append(args, pm2_load_flavor_option);
  leo_args_append(args, flavor);
  leo_args_append(args, name);
  leo_args_append(args, "-leonie");
  leo_args_append(args, net_server->local_host);
  leo_args_append(args, "-link");
  leo_args_append(args, net_server->connection_data.data);

  node = malloc(sizeof(node_t));
  CTRL_ALLOC(node);
  
  node->pid           =   -1;
  node->host          = NULL;
  node->client        = NULL;
  node->networks      = NULL;
  node->network_slist = NULL;
  node->channels      = NULL;
  node->channel_slist = NULL;

  node->pid = call_execvp(args);
  node->host = malloc(strlen(host) + 1);
  CTRL_ALLOC(node->host);

  strcpy(node->host, host);
  
  node->client = malloc(sizeof(ntbx_client_t));
  CTRL_ALLOC(node->client);

  node->client->state = ntbx_client_state_uninitialized;

  node->networks = malloc(sizeof(tbx_htable_t));
  CTRL_ALLOC(node->networks);
  tbx_htable_init(node->networks, 0);
  
  node->network_slist = tbx_slist_nil();

  node->channels = malloc(sizeof(tbx_htable_t));
  CTRL_ALLOC(node->channels);
  tbx_htable_init(node->channels, 0);
  
  node->channel_slist = tbx_slist_nil();

  {
    int status = ntbx_failure;
    
    ntbx_tcp_client_init(node->client);
    status = ntbx_tcp_server_accept(net_server, node->client);
    if (status == ntbx_failure)
      FAILURE("client node failed to connect");
  }
  DISP_STR("Received node connection from", node->host);

  tbx_htable_add(nodes, host, node);
  tbx_slist_append(node_slist, node);
  LOG_OUT();
}

void
populate_channel(char           *name,
		 char           *flavor,
		 p_tbx_htable_t  channel)
{
  p_tbx_slist_t host_slist = NULL;

  LOG_IN();
  host_slist = leoparse_read_slist(channel, "hosts");

  if (host_slist && !tbx_slist_is_nil(host_slist))
    {
      tbx_slist_ref_to_head(host_slist);

      do
	{
	  p_leoparse_object_t  object = NULL;
	  char                *host   = NULL;
	    
	  object = tbx_slist_ref_get(host_slist);
	  host = leoparse_get_id(object);

	  if (!tbx_htable_get(nodes, host))
	    {
	      launch(name, flavor, host);
	    }
	}
      while (tbx_slist_ref_forward(host_slist));
    }
  else
    {
      terminate("parse error : no channel list\n");
    }
  LOG_OUT();
}

void
spawn(p_tbx_slist_t channel_slist)
{
  LOG_IN();
  tbx_slist_ref_to_head(channel_slist);
      
  do
    {
      p_leoparse_object_t  object  = NULL;
      p_tbx_htable_t       channel = NULL;
      char                *name    = NULL;
      
      object = tbx_slist_ref_get(channel_slist);
      channel = leoparse_get_htable(object);
      name = leoparse_read_id(channel, "name");
      
      tbx_htable_add(channels, name, channel);
      populate_channel(application_name, application_flavor, channel);
    }
  while (tbx_slist_ref_forward(channel_slist));
  LOG_OUT();
}

void
process(p_tbx_htable_t application)
{
  LOG_IN();
  application_name     = leoparse_read_id(application, "name");  
  application_flavor   = leoparse_read_id(application, "flavor");
  application_networks = leoparse_read_htable(application, "networks");

  channel_slist = leoparse_read_slist(application_networks, "channels");
  if (channel_slist && !tbx_slist_is_nil(channel_slist))
    {
      spawn(channel_slist);
    }
  else
    {
      terminate("parse error : no channel list");
    }

  include_slist = leoparse_read_slist(application_networks, "include");
  if (include_slist && !tbx_slist_is_nil(include_slist))
    {
      include_network_files(include_slist);
    }
  else
    {
      terminate("parse error : no network include list");      
    }
  LOG_OUT();
}

void
inform(void)
{
  int configuration_size = -1;
  int rank               =  0;
  
  LOG_IN();
  configuration_size = tbx_slist_get_length(node_slist);
  tbx_slist_ref_to_head(node_slist);
  
  do
    {
      p_node_t           node = NULL;
      int                status = ntbx_failure;
      ntbx_pack_buffer_t buffer;

      node = tbx_slist_ref_get(node_slist);

      ntbx_pack_int(configuration_size, &buffer);
      status = ntbx_btcp_write_pack_buffer(node->client, &buffer);

      if (status == ntbx_failure)
	FAILURE("master link failure");
      
      ntbx_pack_int(rank, &buffer);
      status = ntbx_btcp_write_pack_buffer(node->client, &buffer);
      
      if (status == ntbx_failure)
	FAILURE("master link failure");

      rank++;
    }
  while (tbx_slist_ref_forward(node_slist));
  LOG_OUT();
}

void
analyse(void)
{
  LOG_IN();
  tbx_slist_ref_to_head(channel_slist);

  do
    {
      p_leoparse_object_t  object     = NULL;
      p_tbx_htable_t       channel    = NULL;
      p_tbx_slist_t        host_slist = NULL;
      p_tbx_htable_t       network    = NULL;
      char                *name       = NULL;
      char                *net        = NULL;
      
      object     = tbx_slist_ref_get(channel_slist);
      channel    = leoparse_get_htable(object);
      name       = leoparse_read_id(channel, "name");
      DISP_STR("name", name);
      net        = leoparse_read_id(channel, "net");
      DISP_STR("net", net);
      host_slist = leoparse_read_slist(channel, "hosts");
      DISP_PTR("hosts", host_slist);
      network    = tbx_htable_get(networks, net);

      if (!network)
	{
	  terminate("unknown network");
	}
      
      tbx_slist_ref_to_head(host_slist);
      
      do
	{
	  p_leoparse_object_t  object2 = NULL;
	  p_node_t             node    = NULL;
	  char                *host    = NULL;	  
	  
	  object2 = tbx_slist_ref_get(host_slist);
	  host = leoparse_get_id(object2);

	  node = tbx_htable_get(nodes, host);
	  if (!tbx_htable_get(node->channels, name))
	    {
	      tbx_htable_add(node->channels, name, channel);
	      tbx_slist_append(node->channel_slist, channel);
	    }
	  
	  if (!tbx_htable_get(node->networks, net))
	    {
	      tbx_htable_add(node->networks, net, network);
	      tbx_slist_append(node->network_slist, network);
	    }
	}
      while (tbx_slist_ref_forward(host_slist));
    }
  while (tbx_slist_ref_forward(channel_slist));
  LOG_OUT();
}

void
adapter_init(void)
{
  LOG_IN();
  tbx_slist_ref_to_head(node_slist);
  
  do
    {
      p_node_t           node        = NULL;
      int                status      = ntbx_failure;
      int                nb_adapters = 0;
      ntbx_pack_buffer_t buffer;

      node = tbx_slist_ref_get(node_slist);

      nb_adapters = tbx_slist_get_length(node->network_slist);

      ntbx_pack_int(nb_adapters, &buffer);
      status = ntbx_btcp_write_pack_buffer(node->client, &buffer);

      if (status == ntbx_failure)
	FAILURE("master link failure");

      tbx_slist_ref_to_head(node->network_slist);
      
      do
	{
	  p_tbx_htable_t  network   = NULL;
	  p_device_t      device    = NULL;
	  char           *dev       = NULL;
	  int             status    = ntbx_failure;
	  
	  network = tbx_slist_ref_get(node->network_slist);	  
	  dev     = leoparse_read_id(network, "dev");
	  
	  status = ntbx_btcp_write_string(node->client, dev);
	  if (status != ntbx_success)
	    FAILURE("master link failure");

	  device = tbx_htable_get(devices, dev);

	  if (!device)
	    {
	      device = malloc(sizeof(device_t));
	      CTRL_ALLOC(device);
	      device->name  = NULL;
	      device->nodes = NULL;
	      
	      device->name = malloc(strlen(dev) + 1);
	      CTRL_ALLOC(device->name);
	      strcpy(device->name, dev);
	      
	      device->nodes = tbx_slist_nil();
	      tbx_htable_add(devices, dev, device);
	      tbx_slist_append(device_slist, device);
	    }
	  
	  tbx_slist_append(device->nodes, node);
	}
      while (tbx_slist_ref_forward(node->network_slist));
    }
  while (tbx_slist_ref_forward(node_slist));
  LOG_OUT();
}

void
adapter_configuration_init(void)
{
  LOG_IN();
  tbx_slist_ref_to_head(device_slist);
  do
    {
      p_device_t  device                    = NULL;
      int         device_configuration_size =    0;
      int         rank                      =    0;

      device                    = tbx_slist_ref_get(device_slist);
      device_configuration_size = tbx_slist_get_length(device->nodes);

      tbx_slist_ref_to_head(device->nodes);
      do
	{
	  p_node_t node   = NULL;
	  int      status = ntbx_failure;
	  ntbx_pack_buffer_t buffer;
      
	  node = tbx_slist_ref_get(device->nodes);

	  status = ntbx_btcp_write_string(node->client, device->name);
	  if (status == ntbx_failure)
	    FAILURE("master link failure");

	  ntbx_pack_int(device_configuration_size, &buffer);
	  status = ntbx_btcp_write_pack_buffer(node->client, &buffer);

	  if (status == ntbx_failure)
	    FAILURE("master link failure");
	  
	  ntbx_pack_int(rank, &buffer);
	  status = ntbx_btcp_write_pack_buffer(node->client, &buffer);
	  
	  if (status == ntbx_failure)
	    FAILURE("master link failure");
	  
	  rank++;
	}
      while (tbx_slist_ref_forward(device->nodes));

      rank = 0;

      tbx_slist_ref_to_head(device->nodes);
      do
	{
	  p_node_t node   =         NULL;
	  int      data   =           -1;
	  int      status = ntbx_failure;
	  ntbx_pack_buffer_t buffer;
      
	  node = tbx_slist_ref_get(device->nodes);
	  status = ntbx_btcp_read_pack_buffer(node->client, &buffer);
	  if (status == ntbx_failure)
	    FAILURE("master link failure");

	  data = ntbx_unpack_int(&buffer);
	  if (data != rank)
	    FAILURE("node synchronisation error");

	  rank++;
	}
      while (tbx_slist_ref_forward(device->nodes));
    }
  while (tbx_slist_ref_forward(device_slist));
  LOG_OUT();
}

void
channel_init(void)
{
  LOG_IN();
  tbx_slist_ref_to_head(node_slist);
  
  do
    {
      p_node_t           node        = NULL;
      int                status      = ntbx_failure;
      int                nb_channels = 0;
      ntbx_pack_buffer_t buffer;

      node = tbx_slist_ref_get(node_slist);

      nb_channels = tbx_slist_get_length(node->channel_slist);

      ntbx_pack_int(nb_channels, &buffer);
      status = ntbx_btcp_write_pack_buffer(node->client, &buffer);

      if (status == ntbx_failure)
	FAILURE("master link failure");

      tbx_slist_ref_to_head(node->channel_slist);

      do
	{
	  p_tbx_htable_t  channel    =         NULL;
	  p_tbx_slist_t   chan_nodes =         NULL;
	  char           *name       =         NULL;
	  int             status     = ntbx_failure;
  
	  channel = tbx_slist_ref_get(node->channel_slist);
	  name    = leoparse_read_id(channel, "name");

	  status = ntbx_btcp_write_string(node->client, name);
	  if (status != ntbx_success)
	    FAILURE("master link failure");

	  chan_nodes = tbx_htable_get(channel, "nodes");
	  
	  if (!chan_nodes)
	    {
	      chan_nodes = tbx_slist_nil();
	      tbx_htable_add(channel, "nodes", chan_nodes);
	    }
	  
	  tbx_slist_append(chan_nodes, node);
	}
      while (tbx_slist_ref_forward(node->channel_slist));
      
    }
  while (tbx_slist_ref_forward(node_slist));
  LOG_OUT();
}

void
channel_configuration_init(void)
{
  LOG_IN();
  tbx_slist_ref_to_head(channel_slist);
  do
    {
      p_leoparse_object_t  object                     = NULL;
      p_tbx_htable_t       channel                    = NULL;
      p_tbx_slist_t        chan_nodes                 = NULL;
      p_tbx_htable_t       network                    = NULL;
      int                  channel_configuration_size =    0;
      int                  rank                       =    0;
      char                *name                       = NULL;
      char                *net                        = NULL;
      char                *dev                        = NULL;
      
      object     = tbx_slist_ref_get(channel_slist);
      channel    = leoparse_get_htable(object);
      chan_nodes = tbx_htable_get(channel, "nodes");
      channel_configuration_size = tbx_slist_get_length(chan_nodes);

      name    = leoparse_read_id(channel, "name");
      DISP_STR("name", name);
      net     = leoparse_read_id(channel, "net");
      DISP_STR("net", net);
      network = tbx_htable_get(networks, net);
      dev     = leoparse_read_id(network, "dev");
      DISP_STR("dev", dev);

      tbx_slist_ref_to_head(chan_nodes);
      do
	{
	  p_node_t node    = NULL;
	  int      status  = ntbx_failure;
	  ntbx_pack_buffer_t buffer;
	  
	  node = tbx_slist_ref_get(chan_nodes);	  
	  
	  status = ntbx_btcp_write_string(node->client, name);
	  if (status != ntbx_success)
	    FAILURE("master link failure");

	  ntbx_pack_int(channel_configuration_size, &buffer);
	  status = ntbx_btcp_write_pack_buffer(node->client, &buffer);

	  if (status == ntbx_failure)
	    FAILURE("master link failure");
	  
	  ntbx_pack_int(rank, &buffer);
	  status = ntbx_btcp_write_pack_buffer(node->client, &buffer);
	  
	  if (status == ntbx_failure)
	    FAILURE("master link failure");
	  
	  status = ntbx_btcp_write_string(node->client, dev);
	  if (status != ntbx_success)
	    FAILURE("master link failure");

	  DISP_STR("Channel synchronized", name);

	  rank++;
	}
      while (tbx_slist_ref_forward(chan_nodes));

      rank = 0;

      tbx_slist_ref_to_head(chan_nodes);
      do
	{
	  p_node_t node   =         NULL;
	  int      data   =           -1;
	  int      status = ntbx_failure;
	  ntbx_pack_buffer_t buffer;
      
	  node   = tbx_slist_ref_get(chan_nodes);
	  status = ntbx_btcp_read_pack_buffer(node->client, &buffer);
	  if (status == ntbx_failure)
	    FAILURE("master link failure");

	  data = ntbx_unpack_int(&buffer);
	  if (data != rank)
	    FAILURE("node synchronisation error");

	  rank++;
	}
      while (tbx_slist_ref_forward(chan_nodes));
      
    }
  while (tbx_slist_ref_forward(channel_slist));
  LOG_OUT();
}

void
close_links(void)
{
  LOG_IN();
  tbx_slist_ref_to_head(node_slist);
  
  do
    {
      p_node_t node   = NULL;

      node = tbx_slist_ref_get(node_slist);
      ntbx_tcp_client_disconnect(node->client);
    }
  while (tbx_slist_ref_forward(node_slist));

  ntbx_tcp_server_disconnect(net_server);
  free(net_server);
  net_server = NULL;
  LOG_OUT();
}

int
main(int    argc,
     char **argv)
{
  p_tbx_htable_t            app_result = NULL;
  p_leoparse_htable_entry_t app_entry  = NULL;

  LOG_IN();
  common_init(&argc, argv);
  
  tbx_init(argc, argv);
  leoparse_init(argc, argv);

  tbx_purge_cmd_line(&argc, argv);
  leoparse_purge_cmd_line(&argc, argv);

  argc--; argv++;
  if (argc != 1)
    usage();

  if (!(pm2_rsh = getenv("PM2_RSH")))
    {
      pm2_rsh = "rsh";
    }
  
  net_server = leo_net_server_init();

  nodes = malloc(sizeof(tbx_htable_t));
  CTRL_ALLOC(nodes);
  tbx_htable_init(nodes, 0);

  node_slist = tbx_slist_nil();

  networks = malloc(sizeof(tbx_htable_t));
  CTRL_ALLOC(networks);
  tbx_htable_init(networks, 0);
  
  network_slist = tbx_slist_nil();

  devices = malloc(sizeof(tbx_htable_t));
  CTRL_ALLOC(devices);
  tbx_htable_init(devices, 0);
  
  device_slist = tbx_slist_nil();

  channels = malloc(sizeof(tbx_htable_t));
  CTRL_ALLOC(channels);
  tbx_htable_init(channels, 0);
  
  app_result = leoparse_parse_local_file(*argv);
  app_entry  = tbx_htable_get(app_result, "application");
  
  process(leoparse_get_htable(leoparse_get_object(app_entry)));
  inform();
  analyse();
  adapter_init();
  adapter_configuration_init();
  channel_init();
  channel_configuration_init();
  DISP("leonie server ready");
  close_links();
  LOG_OUT();

  DISP("leonie server shutdown");
  return 0;
}

