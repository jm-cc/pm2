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
#define MAX_HOSTNAME_LEN 256
#define MAX_ARG_STR_LEN 1024
#define MAX_ARG_LEN 256

/* ---- */

static mad_madeleine_t      main_madeleine;

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

static void 
mad_managers_init(void)
{
  LOG_IN();
  tbx_init();
  ntbx_init();
  mad_memory_manager_init();
  LOG_OUT();
}

static void 
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
      
#ifdef EXTERNAL_SPAWN
      if (drv == EXTERNAL_SPAWN)
	continue;
#endif /* EXTERNAL_SPAWN */
      driver = &(madeleine->driver[drv]);      
      driver->specific = NULL;
      driver->interface.driver_init(driver);
    }
  LOG_OUT();
}

static void 
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
	  adapter->id                      = description->driver_id;
	  adapter->name                    = description->adapter_selector;
	  adapter->master_parameter        = NULL;
	  adapter->parameter               = NULL;
	  adapter->specific                = NULL;
	}
    }
  LOG_OUT();
}

static void
mad_adapter_init(p_mad_madeleine_t madeleine)
{
  mad_adapter_id_t ad;

  LOG_IN();
  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter;
      
      adapter = &(madeleine->adapter[ad]);
      adapter->specific = NULL;
      if (adapter->driver->interface.adapter_init)
	adapter->driver->interface.adapter_init(adapter);
    }
  LOG_OUT();
}

static void
mad_adapter_configuration_init(p_mad_madeleine_t madeleine)
{
  mad_adapter_id_t ad;

  LOG_IN();
  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter;
      
      adapter = &(madeleine->adapter[ad]);
      if (adapter->driver->interface.adapter_configuration_init)
	adapter->driver->interface.adapter_configuration_init(adapter);
    }
  LOG_OUT();
}

#ifdef APPLICATION_SPAWN

char *
mad_generate_url(p_mad_madeleine_t madeleine)
{
  char             *url          = NULL;
  char             *cgi_string   = NULL;
  char             *arg          = NULL;
  char             *host_name    = NULL;
  char             *cwd          = NULL;
  /* char             *program_name = NULL;
     char             *realp        = NULL;
     const char       *exec_name     = NULL; */
  mad_adapter_id_t  ad;
  int               l;
  
  LOG_IN();
  /* realp = TBX_MALLOC(PATH_MAX);
     CTRL_ALLOC(realp);  */
  host_name = TBX_MALLOC(MAX_HOSTNAME_LEN);
  CTRL_ALLOC(host_name);  
  url = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(url);
  cgi_string = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cgi_string);
  arg = TBX_MALLOC(MAX_ARG_LEN);
  CTRL_ALLOC(arg);

  SYSCALL(gethostname(host_name, MAX_HOSTNAME_LEN));

  while (!(cwd = getcwd(NULL, MAX_ARG_LEN)))
    {    
      if(errno != EINTR)
	{
	  perror("getcwd");
	  FAILURE("syscall failed");
	}
    }

  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter = &(madeleine->adapter[ad]);

      sprintf(arg,
	      "device=%s&",
	      adapter->parameter);
      strcat (cgi_string, arg);
    }
  l = strlen(cgi_string);
  l--;
  cgi_string[l] = 0;
  /*
  exec_name = getexecname();
  if (exec_name[0] != '/')
    {
      strcat(cwd, "/");
      strcat(cwd, exec_name);
      realpath(cwd, realp);
      exec_name = realp;
    }

  program_name = strrchr(exec_name, '/');
  *program_name = '\0';
  program_name++;
  
  sprintf(url, "x-nexus://%s:mad2%s/%s?%s",
	  host_name,
	  exec_name,
	  program_name,
	  cgi_string);
  */

  sprintf(url, "x-nexus://%s:mad2/?%s",
	  host_name,
	  cgi_string);

  TBX_FREE(host_name);
  TBX_FREE(cgi_string);
  TBX_FREE(arg);
  /* TBX_FREE(realp); */
  LOG_OUT();
  return url;
}

char *
mad_pre_init(p_mad_adapter_set_t adapter_set)
{
  p_mad_madeleine_t madeleine = &main_madeleine;

  LOG_IN();
#ifdef PM2  
  marcel_init(argc, argv);
#endif /* PM2 */
  tbx_init();
  
  madeleine->nb_channel = 0;
  TBX_INIT_SHARED(madeleine);
  mad_managers_init();
  mad_driver_fill(madeleine);
  mad_adapter_fill(madeleine, adapter_set);  

  mad_driver_init(madeleine);
  mad_adapter_init(madeleine);
  LOG_OUT();
  return mad_generate_url(madeleine);
}

void
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
#endif /* APPLICATION_SPAWN */

#ifndef APPLICATION_SPAWN
static void
mad_parse_command_line(int                *argc,
		       char              **argv,
		       p_mad_madeleine_t   madeleine,
		       char               *conf_file,
		       p_tbx_bool_t        master,
		       p_tbx_bool_t        slave)
{
#ifndef EXTERNAL_SPAWN
  p_mad_adapter_t  current_adapter = madeleine->adapter;
#endif /* EXTERNAL_SPAWN */
  int              i;
  int              j;
  
  LOG_IN();
  i = j = 1;    
  
  while (i < (*argc))
    {
      if(!strcmp(argv[i], "-master"))
	{
	  *master = tbx_true;
	}
      else if(!strcmp(argv[i], "-slave"))
	{
	  *slave = tbx_true;
	}
      else if(!strcmp(argv[i], "-rank"))
	{
	  if (i == ((*argc) - 1))
	    FAILURE("-rank option must be followed "
		    "by the rank of the process");

	  madeleine->configuration.local_host_id = atoi(argv[i + 1]);
	  i++;
	}
      else if (!strcmp(argv[i], "-conf"))
	{
	  if (i == ((*argc) - 1))
	    FAILURE("-conf must be followed "
		    "by the path of mad2 root directory");

	  if (!conf_file)
	    FAILURE("configuration file already specified");
	  
	  sprintf(conf_file, "%s/.mad2_conf", argv[i + 1]);
	  i++;
	}
#ifndef EXTERNAL_SPAWN
      else if (!strcmp(argv[i], "-device"))
	{
	  if(i == ((*argc) - 1))
	    FAILURE(" -device must be followed by a master device parameter");

	  current_adapter->master_parameter
	    = TBX_MALLOC(strlen(argv[i + 1]) + 1);
	  CTRL_ALLOC(current_adapter->master_parameter);
	    
	  strcpy(current_adapter->master_parameter,
		 argv[i + 1]);
	  LOG_STR("mad_init: master_parameter",
		  current_adapter->master_parameter);
	  current_adapter++;
	  i++;
	}
#endif /* EXTERNAL_SPAWN */
      else if (!strcmp(argv[i], "-cwd"))
	{
	  if(i == ((*argc) - 1))
	    FAILURE("-cwd must be followed "
		    "by the current working directory of the master process");
	  chdir(argv[i + 1]);
	  i++;
	}
      else
	{
	  argv[j++] = argv[i];
	}
      i++;
    }
  *argc = j;
  LOG_OUT();
}
#endif /* APPLICATION_SPAWN */

static char *mad_get_mad_root(void)
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

#if (!defined EXTERNAL_SPAWN) && (!defined APPLICATION_SPAWN)
static void
mad_master_spawn(int                    *argc,
		 char                  **argv,
		 p_mad_configuration_t   configuration,
		 tbx_bool_t              conf_spec)
{
  /* Spawn the master mad2 process */
  int    i;
  char  *cmd     = NULL;
  char  *cwd     = NULL;
  char  *arg_str = NULL;
  char  *arg     = NULL;
  
  LOG_IN();
  cmd = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);
  arg_str = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(arg_str);
  arg = TBX_MALLOC(MAX_ARG_LEN);
  CTRL_ALLOC(arg);
  
  while (!(cwd = getcwd(NULL, MAX_ARG_LEN)))
    {    
      if (errno != EINTR)
	{
	  perror("getcwd");
	  exit(1);
	}
    }
  
  for (i = 1;
       i < *argc;
       i++)
    {
      sprintf(arg, " %s ", argv[i]);
      strcat(arg_str, arg);
    }
  
  for (i = 1;
	   i < *argc;
       i++)
    {
      sprintf(arg, " %s ", argv[i]);
      strcat(arg_str, arg);
    }
  
  if (argv[0][0] != '/')
    {
      if (conf_spec)
	{ 
	  sprintf(cmd,
		  "rsh %s %s/%s -master -cwd %s -rank %d -conf %s %s",
		  configuration->host_name[0],
		  cwd, argv[0], cwd, 0, mad_get_mad_root(), arg_str);
	}
      else
	{ 
	  sprintf(cmd,
		  "rsh %s %s/%s -master -cwd %s -rank %d %s",
		  configuration->host_name[0],
		  cwd, argv[0], cwd, 0, arg_str);
	}
    }
  else
    {
      if (conf_spec)
	{
	  sprintf(cmd,
		  "rsh %s %s -master -rank %d -conf %s %s",
		  configuration->host_name[0],
		  argv[0], 0, mad_get_mad_root(), arg_str);
	}
      else
	{ 
	  sprintf(cmd,
		  "rsh %s %s -master -rank %d %s",
		  configuration->host_name[0], argv[0], 0, arg_str);
	}
    }
  LOG_STR("Loader cmd", cmd);
  system(cmd);
  LOG_OUT();
  exit(EXIT_SUCCESS);  
}

static void
mad_slave_spawn(int                *argc,
		char              **argv,
		tbx_bool_t          conf_spec,
		p_mad_madeleine_t   madeleine)
{
  p_mad_configuration_t   configuration = &(madeleine->configuration);
  int                     i;
  char                   *cmd           = NULL;
  char                   *arg_str       = NULL;
  char                   *arg           = NULL;
  char                   *cwd           = NULL;
  mad_adapter_id_t        ad;
	
  LOG_IN();
  cmd = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);
  arg_str = TBX_MALLOC(MAX_ARG_STR_LEN);
  CTRL_ALLOC(arg_str);
  arg = TBX_MALLOC(MAX_ARG_LEN);
  CTRL_ALLOC(arg);

  while (!(cwd = getcwd(NULL, MAX_ARG_LEN)))
    {    
      if(errno != EINTR)
	{
	  perror("getcwd");
	  FAILURE("syscall failed");
	}
    }

  arg_str[0] = 0;

  /* 1: adapters' connection parameter */
  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter = &(madeleine->adapter[ad]);

      sprintf(arg,
	      " -device %s ",
	      adapter->parameter);
      strcat (arg_str, arg);
    }

  /* 2: application specific args */
  for (i = 1;
       i < *argc;
       i++)
    {
      sprintf(arg, " %s ", argv[i]);
      strcat(arg_str, arg);
    }

  for (i = 1;
       i < configuration->size;
       i++)
    {
      if (argv[0][0] != '/')
	{
	  if (conf_spec)
	    {
	      sprintf(cmd,
		      "rsh %s %s/%s -slave -cwd %s -rank %d -conf %s %s &",
		      configuration->host_name[i],
		      cwd,
		      argv[0],
		      cwd,
		      i,  /* rank */
		      mad_get_mad_root(),
		      arg_str);
	    }
	  else
	    {
	      sprintf(cmd,
		      "rsh %s %s/%s -slave -cwd %s -rank %d %s &",
		      configuration->host_name[i],
		      cwd,
		      argv[0],
		      cwd,
		      i,  /* rank */
		      arg_str);
	    }	  
	}
      else
	{
	  if (conf_spec)
	    {
	      sprintf(cmd,
		      "rsh %s %s -slave -cwd %s -rank %d -conf %s %s &",
		      configuration->host_name[i],
		      argv[0],
		      cwd,
		      i,  /* rank */
		      mad_get_mad_root(),
		      arg_str);
	    }
	  else
	    {
	      
	      sprintf(cmd,
		      "rsh %s %s -slave -cwd %s -rank %d %s &",
		      configuration->host_name[i],
		      argv[0],
		      cwd,
		      i,  /* rank */
		      arg_str);
	    }
	}
	    
      LOG_STR("mad_init: Spawn", cmd);
      system(cmd);	
    }
  TBX_FREE(cwd);
  TBX_FREE(cmd);
  TBX_FREE(arg_str);
  TBX_FREE(arg);  
  LOG_OUT();
}
#endif /* !EXTERNAL_SPAWN && !APPLICATION_SPAWN */

#ifdef EXTERNAL_SPAWN
static void
mad_connect_hosts(p_mad_madeleine_t   madeleine)
{
  p_mad_driver_interface_t spawn_interface;
  p_mad_adapter_t          spawn_adapter;
  mad_adapter_id_t ad;
  ntbx_host_id_t rank;

  LOG_IN();
  spawn_adapter   = &(madeleine->adapter[0]);
  spawn_interface = &(madeleine->driver[EXTERNAL_SPAWN].interface);
  rank            = madeleine->configuration.local_host_id;

  for (ad = 1;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter;
      
      adapter = &(madeleine->adapter[ad]);
      adapter->specific = NULL;      

      if (adapter->driver->interface.adapter_init)
	adapter->driver->interface.adapter_init(adapter);
      if (!rank)
	{
	  ntbx_host_id_t host_id;

	  for (host_id = 1;
	       host_id < madeleine->configuration.size;
	       host_id++)
	    {
	      spawn_interface->send_adapter_parameter(spawn_adapter,
						      host_id,
						      adapter->parameter);
	    }
	}
      else
	{
	  spawn_interface->receive_adapter_parameter(spawn_adapter,
						     &(adapter->
						       master_parameter));
	}
      if (adapter->driver->interface.adapter_configuration_init)
	adapter->driver->interface.adapter_configuration_init(adapter);
    }
  LOG_OUT();
}
#else /* EXTERNAL SPAWN */
#ifndef APPLICATION_SPAWN
static void
mad_connect_hosts(p_mad_madeleine_t   madeleine,
		  tbx_bool_t          conf_spec,
		  int                *argc,
		  char              **argv)
{
  LOG_IN();
  mad_adapter_init(madeleine);
  if (madeleine->configuration.local_host_id == 0)
    {
      mad_slave_spawn(argc, argv, conf_spec, madeleine);
    }
  mad_adapter_configuration_init(madeleine);
  LOG_OUT();
}
#endif /* APPLICATION_SPAWN */
#endif /* EXTERNAL SPAWN */

/* ---- */
#ifndef EXTERNAL_SPAWN
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

  fscanf(f, "%d", &(configuration->size));
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
#endif /* EXTERNAL_SPAWN */

static void
mad_foreach_adapter_exit(void *object)
{
  p_mad_adapter_t            adapter = object;
  p_mad_driver_t             driver  = adapter->driver;
  p_mad_driver_interface_t   interface = &(driver->interface);
  
  LOG_IN();
  if (interface->adapter_exit)
    {
      interface->adapter_exit(adapter);
    }
  else
    {
      if (adapter->specific)
	{
	  TBX_FREE(adapter->specific);
	}
    }
  LOG_OUT();
}

static void
mad_driver_exit(p_mad_madeleine_t madeleine)
{
  mad_driver_id_t drv;
    
  LOG_IN();
  for (drv = 0;
       drv < madeleine->nb_driver;
       drv++)
    {
      p_mad_driver_t             driver;
      p_mad_driver_interface_t   interface;
      
      driver    = &(madeleine->driver[drv]);      
      interface = &(driver->interface);

      tbx_foreach_destroy_list(&(madeleine->channel),
			       mad_foreach_adapter_exit);
      if (interface->driver_exit)
	{
	  interface->driver_exit(driver);
	}
      else
	{
	  if (driver->specific)
	    {
	      TBX_FREE(driver->specific);
	    }
	}
    }

  TBX_FREE(madeleine->adapter);
  TBX_FREE(madeleine->driver);
  LOG_OUT();
}

/* ---- */

#ifdef PM2
p_mad_madeleine_t
mad2_init(int                  *argc,
	  char                **argv,
	  char                 *configuration_file,
	  p_mad_adapter_set_t   adapter_set)
#else /* PM2 */
p_mad_madeleine_t
mad_init(
#ifndef APPLICATION_SPAWN
	 int                   *argc,
	 char                 **argv,
#else /* APPLICATION SPAWN */
	 ntbx_host_id_t         rank,
#endif /* APPLICATION_SPAWN */
	 char                  *configuration_file __attribute__ ((unused)),
#ifdef APPLICATION_SPAWN
	 char                  *url
#else /* APPLICATION_SPAWN */
	 p_mad_adapter_set_t    adapter_set
#endif /* APPLICATION_SPAWN */
	 )
#endif /* PM2 */
{
  p_mad_madeleine_t          madeleine       = &(main_madeleine);
  p_mad_configuration_t      configuration   =
    &(madeleine->configuration);
#ifndef APPLICATION_SPAWN
  ntbx_host_id_t             rank            = -1;
#endif /* APPLICATION_SPAWN */
  tbx_bool_t                 master          = tbx_false;
  tbx_bool_t                 slave           = tbx_false;
#ifdef EXTERNAL_SPAWN
  p_mad_driver_t             spawn_driver    = NULL;
  p_mad_driver_interface_t   spawn_interface = NULL;
  p_mad_adapter_t            spawn_adapter   = NULL;
#endif /* EXTERNAL_SPAWN */
  char                       conf_file[128];
  tbx_bool_t                 conf_spec = tbx_false;

  LOG_IN(); 
  if (!configuration_file)
    {    
      configuration_file = conf_file;
      sprintf(conf_file, "%s/.mad2_conf", mad_get_mad_root());
      conf_spec = tbx_true;
    }  

#ifndef APPLICATION_SPAWN
#ifdef PM2  
  marcel_init(argc, argv);
#endif /* PM2 */
  tbx_init();
  
  madeleine->nb_channel = 0;
  TBX_INIT_SHARED(madeleine);
  mad_managers_init();
  mad_driver_fill(madeleine);
  mad_adapter_fill(madeleine, adapter_set);  
#endif /* APPLICATION_SPAWN */

#ifdef EXTERNAL_SPAWN
  spawn_driver = &(madeleine->driver[EXTERNAL_SPAWN]);
  spawn_interface = &(madeleine->driver[EXTERNAL_SPAWN].interface);
  spawn_driver->specific = NULL;
  spawn_interface->driver_init(spawn_driver);
  if (adapter_set)
    {
      spawn_adapter = &(madeleine->adapter[0]);
    }
  else
    {
      spawn_adapter = &(madeleine->adapter[EXTERNAL_SPAWN]);
    }  
  spawn_adapter->specific = NULL;
  if (spawn_interface->adapter_init)
    spawn_interface->adapter_init(spawn_adapter);
  if (spawn_interface->external_spawn_init)
    spawn_interface->external_spawn_init(spawn_adapter, argc, argv);
#endif /* EXTERNAL_SPAWN */
#ifndef APPLICATION_SPAWN  
  if (conf_spec)
    {      
      mad_parse_command_line(argc,
			     argv,
			     madeleine,
			     conf_file,
			     &master,
			     &slave);
    }
  else
    {
      mad_parse_command_line(argc,
			     argv,
			     madeleine,
			     NULL,
			     &master,
			     &slave);
    }
#endif /* MAD_APPLICATION_SPAWN */
  
#ifdef EXTERNAL_SPAWN
  spawn_interface->configuration_init(spawn_adapter, configuration);
  if (spawn_interface->adapter_configuration_init)
    spawn_interface->adapter_configuration_init(spawn_adapter);
  rank = configuration->local_host_id;

  if (rank == 0)
    {
      master = tbx_true;
      slave  = tbx_false;
    }
  else
    {
      master = tbx_false;
      slave  = tbx_true;
    }
#else
  mad_read_conf(configuration, configuration_file);
#ifdef APPLICATION_SPAWN
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
	  int   i = 1;
	  char *host_name    = NULL;
	  
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
  
#else /* APPLICATION_SPAWN */
  rank = configuration->local_host_id;  

  /* Uncomment to disable the first `rsh' step 
     master = !slave; */
  
  if (!master && !slave)
    {
      mad_master_spawn(argc,
		       argv,
		       configuration,
		       conf_spec);
    }
#endif /* APPLICATION_SPAWN */
#endif /* EXTERNAL SPAWN */

  /* output redirection */
#ifndef APPLICATION_SPAWN
  if (rank > 0)
    {
      char   output[MAX_ARG_LEN];
      int    f;

      sprintf(output, "/tmp/%s-mad2log-%d", getenv("USER"), (int)rank);
      f = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
      dup2(f, STDOUT_FILENO);
      dup2(STDOUT_FILENO, STDERR_FILENO);
    }
#endif /* APPLICATION_SPAWN  */

#ifdef APPLICATION_SPAWN 
  mad_adapter_configuration_init(madeleine);
#else /* APPLICATION_SPAWN */
  mad_driver_init(madeleine);
#ifdef EXTERNAL_SPAWN
  mad_connect_hosts(madeleine);
#else /* EXTERNAL_SPAWN */
  mad_connect_hosts(madeleine, conf_spec, argc, argv);
#endif /* EXTERNAL_SPAWN */
#endif /* APPLICATION_SPAWN */
  tbx_list_init(&(madeleine->channel));

  LOG_OUT();
  return madeleine;
}

#ifdef PM2
void
mad2_exit(p_mad_madeleine_t madeleine)
#else
void
mad_exit(p_mad_madeleine_t madeleine)
#endif
{
  LOG_IN();
  TBX_LOCK_SHARED(madeleine);
  mad_close_channels(madeleine);
  mad_driver_exit(madeleine);
  tbx_list_manager_exit();
  TBX_UNLOCK_SHARED(madeleine);
  LOG_OUT();
}

