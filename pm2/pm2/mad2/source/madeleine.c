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
$Log: madeleine.c,v $
Revision 1.43  2001/01/16 09:55:22  oaumage
- integration du mecanisme de forwarding
- modification de l'usage des flags

Revision 1.42  2000/11/20 17:17:45  oaumage
- suppression de certains fprintf

Revision 1.41  2000/11/20 14:40:29  rnamyst
Fixed the '_argv' bug again. Thanks Olivier ;-)

Revision 1.40  2000/11/20 10:39:37  oaumage
- suppression d'un warning

Revision 1.39  2000/11/20 10:26:45  oaumage
- initialisation, nouvelle version

Revision 1.38  2000/11/16 14:09:13  oaumage
- corrections diverses

Revision 1.37  2000/11/16 13:24:07  oaumage
- mise a jour initialisation

Revision 1.36  2000/11/10 14:17:57  oaumage
- nouvelle procedure d'initialisation

Revision 1.35  2000/11/07 18:30:22  oaumage
*** empty log message ***

Revision 1.34  2000/09/12 09:39:40  oaumage
- correction des problemes de log

Revision 1.33  2000/07/13 10:04:20  oaumage
- Correction de l'identificateur d'adaptateur

Revision 1.32  2000/06/16 14:03:51  oaumage
- Mise a jour par rapport au nouveau fonctionnement de pm2conf

Revision 1.31  2000/05/25 00:23:39  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.30  2000/05/17 14:34:16  oaumage
- Reorganisation des sources au niveau de mad_init

Revision 1.29  2000/05/16 14:17:26  oaumage
- correction de marcel_init dans mad_pre_init

Revision 1.28  2000/05/16 09:50:44  oaumage
- suppression du DEBUG

Revision 1.27  2000/05/15 08:08:08  oaumage
- corrections diverses

Revision 1.26  2000/04/20 13:32:07  oaumage
- modifications diverses

Revision 1.25  2000/04/05 16:08:14  oaumage
- retablissement du rsh supplementaire

Revision 1.24  2000/03/27 08:50:55  oaumage
- pre-support decoupage de groupes
- correction au niveau du support du demarrage manuel

Revision 1.23  2000/03/15 16:59:16  oaumage
- support APPLICATION_SPAWN

Revision 1.22  2000/03/08 17:19:17  oaumage
- support de compilation avec Marcel sans PM2
- pre-support de packages de Threads != Marcel
- utilisation de TBX_MALLOC

Revision 1.21  2000/03/02 15:45:48  oaumage
- support du polling Nexus

Revision 1.20  2000/03/02 14:50:54  oaumage
- support de detection des protocoles au niveau runtime

Revision 1.19  2000/03/02 09:52:17  jfmehaut
pilote Madeleine II/BIP

Revision 1.18  2000/03/01 16:44:54  oaumage
- get_mad_root --> mad_get_mad_root
- correction du passage du chemin de l'application

Revision 1.17  2000/03/01 14:15:50  oaumage
- correction sur get_mad_root

Revision 1.16  2000/03/01 14:09:07  oaumage
- prise en compte d'un fichier de configuration par defaut

Revision 1.15  2000/03/01 11:00:53  oaumage
- suppression de mad_get_root en compilation standalone

Revision 1.14  2000/02/28 11:06:17  rnamyst
Changed #include "" into #include <>.

Revision 1.13  2000/02/10 11:18:01  rnamyst
Modified to use default values for environment variables...

Revision 1.12  2000/02/08 17:49:07  oaumage
- support de la net toolbox

Revision 1.11  2000/02/01 17:26:33  rnamyst
PM2 compatibility functions have moved to pm2_mad.

Revision 1.10  2000/01/31 15:53:37  oaumage
- mad_channel.c : verrouillage au niveau des canaux au lieu des drivers
- madeleine.c : deplacement de aligned_malloc vers la toolbox

Revision 1.9  2000/01/21 17:27:28  oaumage
- mise a jour de l'interface de compatibilite /mad1 et renommage des anciennes
  fonctions

Revision 1.8  2000/01/13 14:45:58  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant
- mad_channel.c, madeleine.c: amelioration des fonctions `par defaut' au niveau
  du support des drivers

Revision 1.7  2000/01/05 09:42:58  oaumage
- mad_communication.c: support du nouveau `group_mode' (a surveiller !)
- madeleine.c: external_spawn_init n'est plus indispensable
- mad_list_management: rien de particulier, mais l'ajout d'une fonction
  d'acces a la longueur de la liste est a l'etude, pour permettre aux drivers
  de faire une optimisation simple au niveau des groupes de buffers de
  taille 1

Revision 1.6  2000/01/04 16:49:10  oaumage
- madeleine: corrections au niveau du support `external spawn'
- support des fonctions non definies dans les drivers

Revision 1.5  1999/12/15 17:31:28  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

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
      madeleine->adapter = TBX_MALLOC(adapter_set->size * sizeof(mad_adapter_t));
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
      settings->configuration_file = TBX_MALLOC(1 + strlen(configuration_file));
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

  /* Network components pre-initialization */
  mad_driver_fill(madeleine);
  mad_adapter_fill(madeleine, adapter_set);

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

  if (configuration->local_host_id == -1)
    FAILURE("could not determine the node rank");

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
      else if (!strcmp(*_argv, "-cwd"))
	{
	  _argv++; (*_argc)--; argc--;

	  if (!argc)
	    FAILURE("device argument disappeared");
	  
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
