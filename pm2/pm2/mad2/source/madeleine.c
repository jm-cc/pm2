
/*
 * Madeleine.c
 * ===========
 */

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

/*
 * Driver registration
 * -------------------
 */
static void (*mad_driver_registration[])(p_mad_driver_t driver) =
{
#ifdef DRV_TCP
  mad_tcp_register,
#endif /* DRV_TCP */
#ifdef DRV_VIA
  mad_via_register,
#endif /* DRV_VIA */
#ifdef DRV_SISCI
  mad_sisci_register,
#endif /* DRV_SISCI */
#ifdef DRV_SBP
  mad_sbp_register,
#endif /* DRV_SBP */
#ifdef DRV_MPI
  mad_mpi_register,
#endif /* DRV_MPI */
#ifdef DRV_BIP
  mad_bip_register,
#endif /* DRV_BIP */
  NULL
};


/*
 * Constantes
 * ----------
 */
#define MAX_HOSTNAME_LEN  256

/*
 * Objet Madeleine
 * ---------------
 */
static mad_madeleine_t main_madeleine;

/*
 * Functions
 * ---------------
 */
int
mad_protocol_available(p_mad_madeleine_t madeleine, char *name)
{
  mad_driver_id_t drv;
    
  LOG_IN();
  for (drv = 0;
       drv < mad_driver_number;
       drv++)
    {
      p_mad_driver_t driver = &madeleine->driver[drv];
      
      if (!strcmp(name, driver->name))
	{
	  LOG_OUT();
	  return drv;
	}
    }

  LOG_OUT();
  return -1;
}

/* ---- */

p_mad_adapter_set_t 
mad_adapter_set_init(int nb_adapter, ...)
{
  p_mad_adapter_set_t  set;
  int                  i;
  va_list              pa;

  LOG_IN();
  va_start(pa, nb_adapter);
  
  LOG_VAL("nb_adapter", nb_adapter);
  
  set = TBX_MALLOC(sizeof(mad_adapter_set_t));
  CTRL_ALLOC(set);
  set->size = nb_adapter;
  set->description =
    TBX_MALLOC(nb_adapter * sizeof(mad_adapter_description_t));
  CTRL_ALLOC(set->description);
  for (i = 0;
       i < nb_adapter;
       i++)
    {
      p_mad_adapter_description_t description ;

      description                   = &(set->description[i]);
      description->driver_id        = va_arg(pa, mad_driver_id_t);
      description->adapter_selector = va_arg(pa, char *);
    }

  LOG_OUT();
  va_end(pa);
  return set;
}

void 
mad_driver_fill(p_mad_madeleine_t madeleine)
{
  mad_driver_id_t drv;
    
  LOG_IN();
  madeleine->driver = TBX_MALLOC(mad_driver_number * sizeof(mad_driver_t));
  CTRL_ALLOC(madeleine->driver);
  madeleine->nb_driver = mad_driver_number;
  
  for (drv = 0;
       drv < mad_driver_number;
       drv++)
    {
      p_mad_driver_t driver;
      
      driver = &(madeleine->driver[drv]);
      
      TBX_INIT_SHARED(driver);
      driver->madeleine = madeleine;
      driver->id        = drv;
      driver->specific  = NULL;
      driver->name      = NULL;

      tbx_list_init(&(driver->adapter_list));
      mad_driver_registration[drv](driver);
    }
  
  LOG_OUT();
}

void 
mad_driver_init(p_mad_madeleine_t madeleine)
{
  mad_driver_id_t  drv;
    
  LOG_IN();
  for (drv = 0;
       drv < madeleine->nb_driver;
       drv++)
    {
      p_mad_driver_t driver;
      
#ifdef EXTERNAL_SPAWN
      if (drv == madeleine->settings->external_spawn_driver)
	continue;
#endif /* EXTERNAL_SPAWN */

      driver = &(madeleine->driver[drv]);      
      driver->interface.driver_init(driver);
    }
  LOG_OUT();
}

void 
mad_adapter_fill(p_mad_madeleine_t     madeleine,
		 p_mad_adapter_set_t   adapter_set)
{
  mad_adapter_id_t ad;

  LOG_IN();

  if (!adapter_set)
    {      
      madeleine->adapter = TBX_MALLOC(mad_driver_number * sizeof(mad_adapter_t));
      CTRL_ALLOC(madeleine->adapter);

      madeleine->nb_adapter = mad_driver_number;

      /* Local Init */
      for (ad = 0;
	   ad < madeleine->nb_adapter;
	   ad++)
	{
	  p_mad_driver_t                driver;
	  p_mad_driver_interface_t      interface;
	  p_mad_adapter_t               adapter;
      
	  driver      = &(madeleine->driver[ad]);
	  interface   = &(driver->interface);
	  adapter     = &(madeleine->adapter[ad]);

	  TBX_INIT_SHARED(adapter);
	  tbx_append_list(&(driver->adapter_list), adapter);
      
	  adapter->driver                  = driver;
	  adapter->id                      = ad;
	  adapter->name                    = NULL;
	  adapter->master_parameter        = NULL;
	  adapter->parameter               = NULL;
	  adapter->specific                = NULL;
	}
    }
  else
    {
      madeleine->adapter =
	TBX_MALLOC(adapter_set->size * sizeof(mad_adapter_t));
      CTRL_ALLOC(madeleine->adapter);

      madeleine->nb_adapter = adapter_set->size;
  
      /* Local Init */
      for (ad = 0;
	   ad < madeleine->nb_adapter;
	   ad++)
	{
	  p_mad_adapter_description_t   description;
	  p_mad_driver_t                driver;
	  p_mad_driver_interface_t      interface;
	  p_mad_adapter_t               adapter;
      
	  description = &(adapter_set->description[ad]);
	  driver      = &(madeleine->driver[description->driver_id]);
	  interface   = &(driver->interface);
	  adapter     = &(madeleine->adapter[ad]);

	  TBX_INIT_SHARED(adapter);
	  tbx_append_list(&(driver->adapter_list), adapter);
      
	  adapter->driver                  = driver;
	  adapter->id                      = ad;
	  adapter->name                    = description->adapter_selector;
	  adapter->master_parameter        = NULL;
	  adapter->parameter               = NULL;
	  adapter->specific                = NULL;
	}
    }
  LOG_OUT();
}

void
mad_adapter_init(p_mad_madeleine_t madeleine)
{
#ifdef EXTERNAL_SPAWN
  p_mad_settings_t settings = madeleine->settings;
#endif /* EXTERNAL_SPAWN */
  mad_adapter_id_t ad;

  LOG_IN();
  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter;
      
      adapter = &(madeleine->adapter[ad]);

#ifdef EXTERNAL_SPAWN
      if (&(madeleine->driver[settings->external_spawn_driver]) ==
	  adapter->driver)
	continue;
#endif /* EXTERNAL_SPAWN */

      if (adapter->driver->interface.adapter_init)
	adapter->driver->interface.adapter_init(adapter);
    }
  LOG_OUT();
}

void
mad_adapter_configuration_init(p_mad_madeleine_t madeleine)
{
#ifdef EXTERNAL_SPAWN
  p_mad_settings_t settings = madeleine->settings;
#endif /* EXTERNAL_SPAWN */
  mad_adapter_id_t ad;

  LOG_IN();
  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter;
      
      adapter = &(madeleine->adapter[ad]);

#ifdef EXTERNAL_SPAWN
      if (&(madeleine->driver[settings->external_spawn_driver]) ==
	  adapter->driver)
	continue;
#endif /* EXTERNAL_SPAWN */

      if (adapter->driver->interface.adapter_configuration_init)
	adapter->driver->interface.adapter_configuration_init(adapter);
    }
  LOG_OUT();
}

char *
mad_get_mad_root(void)
{
  static char buf[1024];
  char *ptr;

  if((ptr = getenv("MAD2_ROOT")) != NULL)
    return ptr;
  else {
    sprintf(buf, "%s/mad2", getenv("PM2_ROOT"));
    return buf;
  }
}

p_mad_madeleine_t
mad_object_init(int                   argc,
		char                **argv,
		char                 *configuration_file,
		p_mad_adapter_set_t   adapter_set)
{
  p_mad_madeleine_t     madeleine     = &(main_madeleine);
  p_mad_settings_t      settings      = NULL;
  p_mad_configuration_t configuration = NULL;
  p_ntbx_client_t       client        = NULL;

  LOG_IN();
 
  /* Structure initialization */
  TBX_INIT_SHARED(madeleine);
  madeleine->nb_driver     =    0;
  madeleine->driver        = NULL;
  madeleine->nb_adapter    =    0;
  madeleine->adapter       = NULL;
  madeleine->nb_channel    =    0;
  tbx_list_init(&(madeleine->channel));
  madeleine->settings      = NULL;
  madeleine->configuration = NULL;
#ifdef MAD_LEONIE_SPAWN
  madeleine->master_link   = NULL;
#endif /* MAD_LEONIE_SPAWN */

  settings = TBX_MALLOC(sizeof(mad_settings_t));
  CTRL_ALLOC(settings);

  settings->rsh_cmd               =      NULL;
  settings->configuration_file    =      NULL;
  settings->debug_mode            = tbx_false;
  settings->external_spawn_driver =        -1;

  settings->rsh_cmd = getenv("PM2_RSH");
  if (!settings->rsh_cmd)
    {
      settings->rsh_cmd = "rsh";
    }  

  if (configuration_file)
    {
      settings->configuration_file =
	TBX_MALLOC(1 + strlen(configuration_file));
      CTRL_ALLOC(settings->configuration_file);
      strcpy(settings->configuration_file, configuration_file);
    }
  else if (getenv("PM2_CONF_FILE"))
    {
      settings->configuration_file =
	TBX_MALLOC(1 + strlen(getenv("PM2_CONF_FILE")));
      CTRL_ALLOC(settings->configuration_file);
      strcpy(settings->configuration_file, getenv("PM2_CONF_FILE"));
    }
  else
    {
      settings->configuration_file = NULL;
    }

#ifdef EXTERNAL_SPAWN
  if (adapter_set)
    {
      settings->external_spawn_driver = 0;
    }
  else
    {
      settings->external_spawn_driver = EXTERNAL_SPAWN;
    }  
#endif /* EXTERNAL_SPAWN */

  madeleine->settings = settings;
  
  configuration = TBX_MALLOC(sizeof(mad_configuration_t));
  CTRL_ALLOC(configuration);
  
  configuration->size          =    0;
  configuration->local_host_id =   -1;
  configuration->host_name     = NULL;
  configuration->program_name  = NULL;
  
  madeleine->configuration = configuration;

#ifdef LEONIE_SPAWN
  client = TBX_MALLOC(sizeof(ntbx_client_t));
  CTRL_ALLOC(client);
  client->state = ntbx_client_state_uninitialized;
  ntbx_tcp_client_init(client);
  madeleine->master_link = client;
#endif /* LEONIE_SPAWN */

  /* Network components pre-initialization */
  mad_driver_fill(madeleine);
#ifndef LEONIE_SPAWN
  mad_adapter_fill(madeleine, adapter_set);
#endif /* LEONIE_SPAWN */
  LOG_OUT();
  
  return madeleine;
}

void
mad_cmd_line_init(p_mad_madeleine_t   madeleine,
		  int                 argc,
		  char              **argv)
{
  p_mad_adapter_t       adapter       = madeleine->adapter;
  p_mad_configuration_t configuration = madeleine->configuration;
  p_mad_settings_t      settings      = madeleine->settings;

  LOG_IN();

  argc--; argv++;

  while (argc)
    {
      if (!strcmp(*argv, "-d"))
	{
	  settings->debug_mode = tbx_true;
	}
      else if (!strcmp(*argv, "-rank"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("rank argument not found");

	  configuration->local_host_id = atoi(*argv);
	}
      else if (!strcmp(*argv, "-conf"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("conf argument not found");

	  if (settings->configuration_file)
	    {
	      TBX_FREE(settings->configuration_file);
	    }

	  settings->configuration_file = TBX_MALLOC(strlen(*argv) + 1);
	  CTRL_ALLOC(settings->configuration_file);
	      
	  strcpy(settings->configuration_file, *argv);
	}
      else if (!strcmp(*argv, "-device"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("device argument not found");

	  adapter->master_parameter = TBX_MALLOC(strlen(*argv) + 1);
	  CTRL_ALLOC(adapter->master_parameter);
	  
	  strcpy(adapter->master_parameter, *argv);
	  adapter++;
	}
      else if (!strcmp(*argv, "-cwd"))
	{
	  argc--; argv++;

	  if (!argc)
	    FAILURE("cwd argument not found");

	  chdir(*argv);
	}

      argc--; argv++;
    }

#ifndef LEONIE_SPAWN
  if (configuration->local_host_id == -1)
    FAILURE("could not determine the node rank");
#endif /* LEONIE_SPAWN */

  LOG_OUT();
}

void
mad_configuration_init(p_mad_madeleine_t   madeleine,
		       int                 argc,
		       char              **argv)
{
  p_mad_configuration_t  configuration = madeleine->configuration;
  p_mad_settings_t       settings      = madeleine->settings;

  LOG_IN();

  if (settings->configuration_file)
    {
      FILE           *f      = NULL;
      size_t          status = 0;
      tbx_flag_t      state  = tbx_flag_clear;
      ntbx_host_id_t  i;
      char            c;

      configuration->size = 0;
      
      f = fopen(settings->configuration_file, "r");
    
      if (!f)
	ERROR("fopen");

      for (;;)
	{
	  status = fread(&c, 1, 1, f);
	    
	  if (!status)
	    {
	      if (ferror(f))
		{
		  ERROR("fread");
		}
	      else if (feof(f))
		{
		  break;
		}
	      else
		FAILURE("fread: unknown state");
	    }

	  if (tbx_test(&state))
	    {
	      if (c <= ' ')
		{
		  tbx_toggle(&state);
		}
	    }
	  else
	    {
	      if (c > ' ')
		{
		  configuration->size++;
		  tbx_toggle(&state);
		}
	    }
	}

      rewind(f);
      
      configuration->host_name =
	TBX_MALLOC(configuration->size * sizeof(char *));
      CTRL_ALLOC(configuration->host_name);

      for (i = 0; i < configuration->size; i++)
	{
	  configuration->host_name[i] = TBX_MALLOC(MAX_HOSTNAME_LEN + 1);
	  CTRL_ALLOC(configuration->host_name[i]);
	  fscanf(f, "%s", configuration->host_name[i]);
	}

      fclose(f);
    }
#ifdef EXTERNAL_SPAWN
	  mad_exchange_configuration_info(madeleine);
#else EXTERNAL_SPAWN
  else
    {
      if (configuration->size)
	{
	  ntbx_host_id_t rank = configuration->local_host_id;
	  ntbx_host_id_t i;

	  configuration->host_name =
	    TBX_CALLOC(configuration->size, sizeof(char *));
	  CTRL_ALLOC(configuration->host_name);

	  configuration->host_name[rank] = TBX_MALLOC(MAX_HOSTNAME_LEN + 1);
	  CTRL_ALLOC(configuration->host_name[rank]);
	  
	  gethostname(configuration->host_name[rank], MAX_HOSTNAME_LEN);

	  for (i = 0; i < configuration->size; i++)
	    {
	      configuration->host_name[i] = TBX_MALLOC(MAX_HOSTNAME_LEN + 1);
	      CTRL_ALLOC(configuration->host_name[i]);
	      strcpy(configuration->host_name[i], "<unknown>");
	    }
	}
      else
	FAILURE("undefined configuration size");
    }
#endif /* EXTERNAL_SPAWN */  
  LOG_OUT();
}

void
mad_output_redirection_init(p_mad_madeleine_t   madeleine,
			    int                 argc,
			    char              **argv)
{
  p_mad_configuration_t  configuration = madeleine->configuration;
  p_mad_settings_t       settings      = madeleine->settings;

  LOG_IN();
  if (configuration->local_host_id && !settings->debug_mode)
    {
      char *fmt  = "/tmp/%s-%s-%d";
      char *user = NULL;
      int   len  = 0;

      user = getenv("USER");
      if (!user)
	FAILURE("USER environment variable not defined");

      len = strlen(fmt) + strlen(user) + strlen(MAD2_LOGNAME);
      
      {
	char output[len];
	int  f;

	sprintf(output, "/tmp/%s-%s-%d",
		user, MAD2_LOGNAME, (int)configuration->local_host_id);

	f = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if (f < 0)
	  ERROR("open");
      
	if (dup2(f, STDOUT_FILENO) < 0)
	  ERROR("dup2");

	if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
	  ERROR("dup2");
      }
    }
  LOG_OUT();
}

void
mad_network_components_init(p_mad_madeleine_t   madeleine,
			    int                 argc,
			    char              **argv)
{
  LOG_IN();
  mad_driver_init(madeleine);
  mad_adapter_init(madeleine);
  LOG_OUT();
}

void  
mad_connect(p_mad_madeleine_t   madeleine,
	    int                 argc,
	    char              **argv)
{
  LOG_IN();
#ifdef EXTERNAL_SPAWN
  mad_exchange_connection_info(madeleine);
#endif /* EXTERNAL_SPAWN */

  mad_adapter_configuration_init(madeleine);
  LOG_OUT();
}

void
mad_purge_command_line(p_mad_madeleine_t   madeleine,
		       int                *_argc,
		       char              **_argv)
{
  int     argc = *_argc;
  char ** argv =  _argv;
  LOG_IN();

  argv++; _argv++; argc--;
  
  while (argc)
    {
      if (!strcmp(*_argv, "-d"))
	{
	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "-rank"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("rank argument disappeared");
	  
	  _argv++; (*_argc)--;

	}
      else if (!strcmp(*_argv, "-conf"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("conf argument disappeared");
	  
	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "-device"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("device argument disappeared");
	  
	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "-leonie"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("leonie argument disappeared");
	  
	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "-link"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("link argument disappeared");
	  
	  _argv++; (*_argc)--;
	}
      else if (!strcmp(*_argv, "-cwd"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("cwd argument disappeared");
	  
	  _argv++; (*_argc)--;
	}
      else
	{
	  *argv++ = *_argv++;
	}

      argc--;
    }
  
  LOG_OUT();
}
