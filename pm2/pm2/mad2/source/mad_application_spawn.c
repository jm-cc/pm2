
/*
 * Mad_application_spawn.c
 * =======================
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
#include <sys/wait.h>

/* #define DEBUG */
/* #define TIMING */
#include "madeleine.h"

#ifdef APPLICATION_SPAWN

/*
 * Constantes
 * ----------
 */
#define MAX_HOSTNAME_LEN  256
#define MAX_ARG_STR_LEN  1024
#define MAX_ARG_LEN       256

/*
 * Objet Madeleine
 * ---------------
 */
static mad_madeleine_t main_madeleine;

/*
 * Initialisation des drivers
 * --------------------------
 */
static void 
mad_driver_init(p_mad_madeleine_t madeleine)
{
  mad_driver_id_t drv;
    
  LOG_IN();
  for (drv = 0;
       drv < madeleine->nb_driver;
       drv++)
    {
      p_mad_driver_t driver;
      
      driver = &(madeleine->driver[drv]);      
      driver->specific = NULL;
      driver->interface.driver_init(driver);
    }
  LOG_OUT();
}

/*
 * Lecture du fichier de configuration
 * -----------------------------------
 */
static void
mad_read_conf(p_mad_configuration_t   configuration,
	      char                   *configuration_file)
{
  /* configuration retrieval */
  FILE *f;
  int i;

  LOG_IN();
  f = fopen(configuration_file, "r");
  if(f == NULL)
    {
      perror("configuration file");
      exit(1);
    }

    {
      char commande[MAX_ARG_STR_LEN];
      int ret;
      
      sprintf(commande, "exit `cat %s | wc -w`", configuration_file);

      ret = system(commande);

      configuration->size = WEXITSTATUS(ret);
    }

  configuration->host_name = TBX_MALLOC(configuration->size * sizeof(char *));
  CTRL_ALLOC(configuration->host_name);

  for (i = 0;
       i < configuration->size;
       i++)
    {
      configuration->host_name[i] = TBX_MALLOC(MAX_HOSTNAME_LEN);
      CTRL_ALLOC(configuration->host_name[i]);
      fscanf(f, "%s", configuration->host_name[i]);
    }

  fclose(f);
  LOG_OUT();
}

/*
 * Generation de l'URL de connexion
 * --------------------------------
 */
static char *
mad_generate_url(p_mad_madeleine_t madeleine)
{
  char             *url          = NULL;
  char             *cgi_string   = NULL;
  char             *arg          = NULL;
  char             *host_name    = NULL;
  mad_adapter_id_t  ad;
  int               l;
  
  LOG_IN();
  host_name = TBX_MALLOC(MAX_HOSTNAME_LEN);
  CTRL_ALLOC(host_name);  
  url = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(url);
  cgi_string = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cgi_string);
  arg = TBX_MALLOC(MAX_ARG_LEN);
  CTRL_ALLOC(arg);

  SYSCALL(gethostname(host_name, MAX_HOSTNAME_LEN));

  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter = &(madeleine->adapter[ad]);

      sprintf(arg, "device=%s&", adapter->parameter);
      strcat (cgi_string, arg);
    }
  l = strlen(cgi_string);
  cgi_string[l - 1] = 0;
  sprintf(url, "%s?%s", host_name, cgi_string);

  TBX_FREE(host_name);
  TBX_FREE(cgi_string);
  TBX_FREE(arg);

  LOG_OUT();
  return url;
}

/*
 * Premiere phase d'initialisation de Madeleine
 * --------------------------------------------
 */
char *
mad_pre_init(p_mad_adapter_set_t adapter_set)
{
  p_mad_madeleine_t  madeleine  = &main_madeleine;
  int                dummy_argc = 1;
  char              *dummy_argv[0] = "unavailable";

  madeleine->nb_channel = 0;
  TBX_INIT_SHARED(madeleine);
  mad_managers_init(&argc, argv);
  LOG_IN(); /* After pm2debug_init ... */
  mad_driver_fill(madeleine);
  mad_adapter_fill(madeleine, adapter_set);  

  mad_driver_init(madeleine);
  mad_adapter_init(madeleine);
  LOG_OUT();
  return mad_generate_url(madeleine);
}

/*
 * Interpretation de l'URL de connexion
 * ------------------------------------
 */
static void
mad_parse_url(p_mad_madeleine_t  madeleine,
	      char              *url,
	      p_tbx_bool_t       master)
{
  mad_adapter_id_t  ad = 0;
  char             *tag;
  char             *param;
  char             *c;
  
  LOG_IN();
  if (!url)
    {
      *master = tbx_true;
      LOG_OUT();
      return;
    }
  *master = tbx_false;

  c = strchr(url, '?');
  if (!c)
    {
      LOG_OUT();
      return;
    }
  c++;

  do
    {
      p_mad_adapter_t current_adapter = NULL;
      
      if (ad >= madeleine->nb_adapter)
	FAILURE("too much parameters");

      current_adapter = &madeleine->adapter[ad];

      tag = c;
      c = strchr(tag, '=');
      
      if (!c)
	FAILURE("invalid cgi string");

      *c = '\0';
      c++;
      param = c;
      c = strchr(param, '&');
      
      if (c)
	{
	  *c='\0';
	  c++;
	}

      if (strcmp(tag, "device"))
	FAILURE("invalid parameter tag");

      current_adapter->master_parameter = TBX_MALLOC(strlen(param) + 1);
      CTRL_ALLOC(current_adapter->master_parameter);
      strcpy(current_adapter->master_parameter, param);
      ad++;      
    }
  while(c);
  
  if (ad != madeleine->nb_adapter)
    FAILURE("not enough parameters");
  LOG_OUT();
}


/*
 * mad_init
 * --------
 */
p_mad_madeleine_t
mad_init(
	 ntbx_host_id_t  rank,
	 char           *configuration_file,
	 char           *url
	 )
{
  p_mad_madeleine_t          madeleine       = &(main_madeleine);
  p_mad_configuration_t      configuration   =
    &(madeleine->configuration);
  tbx_bool_t                 master          = tbx_false;
  tbx_bool_t                 slave           = tbx_false;
  char                       conf_file[128];

  LOG_IN(); 
  if (!configuration_file)
    {    
      if (getenv("PM2_CONF_FILE"))
	{
	  configuration_file = conf_file;
	  sprintf(configuration_file, "%s", getenv("PM2_CONF_FILE"));
	}
      else
	FAILURE("configuration file not specified");
    }  

  mad_read_conf(configuration, configuration_file);
  mad_parse_url(madeleine, url, &master);
  slave = !master;
  if (master)
    {
      if (rank == -1)
	{
	  rank = 0;
	}
      else if (rank != 0)
	FAILURE("invalid rank specified for master node");
    }
  else
    {
      if (rank == -1)
	{
	  int   i         = 1;
	  char *host_name = NULL;
	  
	  host_name = TBX_MALLOC(MAX_HOSTNAME_LEN);
	  CTRL_ALLOC(host_name);  

	  SYSCALL(gethostname(host_name, MAX_HOSTNAME_LEN));
	  
	  while(i < configuration->size)
	    {
	      if (!strcmp(configuration->host_name[i],
			  host_name))
		break;
	      i++;
	    }

	  if (i >= configuration->size)
	    FAILURE("unable to determine rank from host name");

	  rank = i;
	}
      else if (rank <= 0)
	FAILURE("invalid rank specified for slave node");
    }

  configuration->local_host_id = rank;

  mad_adapter_configuration_init(madeleine);
  tbx_list_init(&(madeleine->channel));

  LOG_OUT();
  return madeleine;
}


char *
new_mad_pre_init(p_mad_adapter_set_t adapter_set)
{
  p_mad_madeleine_t  madeleine     =    NULL;
  int                dummy_argc    =       1;
  char              *dummy_argv[0] = "<N/A>";

  tbx_init(*dummy_argc, dummy_argv);
  ntbx_init(*dummy_argc, dummy_argv);
  mad_memory_manager_init(*dummy_argc, dummy_argv);

  madeleine = mad_object_init(*dummy_argc, dummy_argv, NULL, adapter_set);
  mad_network_components_init(madeleine, *dummy_argc, dummy_argv);

  return mad_generate_url(madeleine);
}

p_mad_madeleine_t
new_mad_init(
	 ntbx_host_id_t  rank,
	 char           *configuration_file,
	 char           *url
	 )
{
  int                dummy_argc    =        1;
  char              *dummy_argv[0] = "<N/A>";

  madeleine->configuration->local_host_id = rank;

  madeleine->settings->configuration_file =
    TBX_MALLOC(strlen(configuration_file) + 1);
  CTRL_ALLOC(madeleine->settings->configuration_file);
  strcpy(madeleine->settings->configuration_file, configuration_file);

  madeleine->settings->url = TBX_MALLOC(strlen(url) + 1);
  CTRL_ALLOC(madeleine->settings->url);
  strcpy(madeleine->settings->url, url);

  mad_output_redirection_init(madeleine, *dummy_argc, dummy_argv);
  mad_configuration_init(madeleine, *dummy_argc, dummy_argv);
  mad_parse_url(madeleine, *dummy_argc, dummy_argv);
  mad_connect(madeleine, *dummy_argc, dummy_argv);
  
}


#endif /* APPLICATION_SPAWN */

