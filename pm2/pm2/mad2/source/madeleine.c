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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <errno.h>
#ifdef PM2
#include <sys/wait.h>
#endif /* PM2 */

/* #define DEBUG */
/* #define TIMING */
#include <madeleine.h>


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
  NULL
};

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
  
  set = malloc(sizeof(mad_adapter_set_t));
  CTRL_ALLOC(set);
  set->size = nb_adapter;
  set->description =
    malloc(nb_adapter * sizeof(mad_adapter_description_t));
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
  madeleine->driver = malloc(mad_driver_number * sizeof(mad_driver_t));
  CTRL_ALLOC(madeleine->driver);
  madeleine->nb_driver = mad_driver_number;
  
  for (drv = 0;
       drv < mad_driver_number;
       drv++)
    {
      p_mad_driver_t driver;
      
      driver = &(madeleine->driver[drv]);
      
      PM2_INIT_SHARED(driver);
      driver->madeleine = madeleine;
      driver->id        = drv;
      driver->specific  = NULL;

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
  madeleine->adapter = malloc(adapter_set->size * sizeof(mad_adapter_t));
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

      PM2_INIT_SHARED(adapter);
      tbx_append_list(&(driver->adapter_list), adapter);
      
      adapter->driver                  = driver;
      adapter->id                      = description->driver_id;
      adapter->name                    = description->adapter_selector;
      adapter->master_parameter        = NULL;
      adapter->parameter               = NULL;
      adapter->specific                = NULL;
      
      /* interface->init_adapter(adapter); */
    }
  LOG_OUT();
}

static void
mad_parse_command_line(int                *argc,
		       char              **argv,
		       p_mad_madeleine_t   madeleine,
#ifdef PM2
		       char               *conf_file,
#endif /* PM2 */
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
#ifdef PM2
      else if (!strcmp(argv[i], "-conf"))
	{
	  if (i == ((*argc) - 1))
	    FAILURE("-conf must be followed "
		    "by the path of mad2 root directory");

	  sprintf(conf_file, "%s/.mad2_conf", argv[i + 1]);
	  i++;
	}
#endif /* PM2 */
#ifndef EXTERNAL_SPAWN
      else if (!strcmp(argv[i], "-device"))
	{
	  if(i == ((*argc) - 1))
	    FAILURE(" -device must be followed by a master device parameter");

	  current_adapter->master_parameter
	    = malloc(strlen(argv[i + 1]) + 1);
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

#ifndef EXTERNAL_SPAWN
static void
mad_master_spawn(int                    *argc,
		 char                  **argv,
		 p_mad_configuration_t   configuration)
{
  /* Spawn the master mad2 process */
  int    i;
  char  *cmd     = NULL;
  char  *cwd     = NULL;
  char  *arg_str = NULL;
  char  *arg     = NULL;
  
  LOG_IN();
  cmd = malloc(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);
  arg_str = malloc(MAX_ARG_STR_LEN);
  CTRL_ALLOC(arg_str);
  arg = malloc(MAX_ARG_LEN);
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
#ifdef PM2
      sprintf(cmd,
	      "rsh %s %s/%s -master -cwd %s -rank %d -conf %s %s",
	      configuration->host_name[0],
	      cwd, argv[0], cwd, 0, getenv("MAD2_ROOT"), arg_str);
#else /* PM2 */
      sprintf(cmd,
	      "rsh %s %s/%s -master -cwd %s -rank %d %s",
	      configuration->host_name[0],
	      cwd, argv[0], cwd, 0, arg_str);
#endif /* PM2 */
    }
  else
    {
#ifdef PM2
      sprintf(cmd,
	      "rsh %s %s -master -rank %d -conf %s %s",
	      configuration->host_name[0],
	      argv[0], 0, getenv("MAD2_ROOT"), arg_str);
#else /* PM2 */
      sprintf(cmd,
	      "rsh %s %s -master -rank %d %s",
	      configuration->host_name[0], argv[0], 0, arg_str);
#endif /* PM2 */
    }
  LOG_STR("Loader cmd", cmd);
  system(cmd);
  LOG_OUT();
  exit(EXIT_SUCCESS);  
}

static void
mad_slave_spawn(int                *argc,
		char              **argv,
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
  cmd = malloc(MAX_ARG_STR_LEN);
  CTRL_ALLOC(cmd);
  arg_str = malloc(MAX_ARG_STR_LEN);
  CTRL_ALLOC(arg_str);
  arg = malloc(MAX_ARG_LEN);
  CTRL_ALLOC(arg);

  while (!(cwd = getcwd(NULL, MAX_ARG_LEN)))
    {    
      if(errno != EINTR)
	{
	  perror("getcwd");
	  exit(1);
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
#ifdef PM2
	  sprintf(cmd,
		  "rsh %s %s/%s -slave -cwd %s -rank %d -conf %s %s &",
		  configuration->host_name[i],
		  cwd,
		  argv[0],
		  cwd,
		  i,  /* rank */
		  getenv("MAD2_ROOT"),
		  arg_str);
#else /* PM2 */
	  sprintf(cmd,
		  "rsh %s %s/%s -slave -cwd %s -rank %d %s &",
		  configuration->host_name[i],
		  cwd,
		  argv[0],
		  cwd,
		  i,  /* rank */
		  arg_str);
#endif /* PM2 */
	}
      else
	{
#ifdef PM2
	  sprintf(cmd,
		  "rsh %s %s -slave -cwd %s -rank %d -conf %s %s &",
		  configuration->host_name[i],
		  argv[0],
		  cwd,
		  i,  /* rank */
		  getenv("MAD2_ROOT"),
		  arg_str);
#else /* PM2 */
	  sprintf(cmd,
		  "rsh %s %s -slave -cwd %s -rank %d %s &",
		  configuration->host_name[i],
		  argv[0],
		  cwd,
		  i,  /* rank */
		  arg_str);
#endif /* PM2 */
	}
	    
      LOG_STR("mad_init: Spawn", cmd);
      system(cmd);
	
    }
  free(cwd);
  free(cmd);
  free(arg_str);
  free(arg);  
  LOG_OUT();
}
#endif /* !EXTERNAL_SPAWN */

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
static void
mad_connect_hosts(p_mad_madeleine_t   madeleine,
		  int                *argc,
		  char              **argv)
{
  mad_adapter_id_t ad;
  ntbx_host_id_t rank;

  LOG_IN();
  rank = madeleine->configuration.local_host_id;

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

  if (rank == 0)
    {
      mad_slave_spawn(argc, argv, madeleine);
    }
 
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
#endif /* EXTERNAL SPAWN*/

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
      LOG_STR("mad_init: configuration file", configuration_file);
      perror("configuration file");
      exit(1);
    }

#ifdef PM2
  {
    int ret; 
    char command[128];

    sprintf(command, "exit `cat %s | wc -w`", configuration_file);     
    ret = system(command);
    configuration->size = WEXITSTATUS(ret);
  }
#else /* PM2 */
  fscanf(f, "%d", &(configuration->size));
#endif /* PM2 */
  configuration->host_name = malloc(configuration->size * sizeof(char *));
  CTRL_ALLOC(configuration->host_name);

  for (i = 0;
       i < configuration->size;
       i++)
    {
      configuration->host_name[i] = malloc(MAX_HOSTNAME_LEN);
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
	  free(adapter->specific);
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
	      free(driver->specific);
	    }
	}
    }

  free(madeleine->adapter);
  free(madeleine->driver);
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
mad_init(int                   *argc,
	 char                 **argv,
	 char                  *configuration_file __attribute__ ((unused)),
	 p_mad_adapter_set_t    adapter_set)
#endif /* PM2 */
{
  p_mad_madeleine_t          madeleine       = &(main_madeleine);
  p_mad_configuration_t      configuration   =
    &(madeleine->configuration);
  ntbx_host_id_t             rank            = -1;
  tbx_bool_t                 master          = tbx_false;
  tbx_bool_t                 slave           = tbx_false;
#ifdef EXTERNAL_SPAWN
  p_mad_driver_t             spawn_driver    = NULL;
  p_mad_driver_interface_t   spawn_interface = NULL;
  p_mad_adapter_t            spawn_adapter   = NULL;
#endif /* EXTERNAL_SPAWN */
#ifdef PM2
  char                       conf_file[128];
  
  configuration_file = conf_file;
  sprintf(conf_file, "%s/.mad2_conf", getenv("MAD2_ROOT"));
#endif /* PM2 */

  LOG_IN(); 

#ifdef PM2  
  marcel_init(argc, argv);
#endif /* PM2 */
  tbx_init();
  
  madeleine->nb_channel = 0;
  PM2_INIT_SHARED(madeleine);
  mad_managers_init();
  mad_driver_fill(madeleine);
  mad_adapter_fill(madeleine, adapter_set);  

#ifdef EXTERNAL_SPAWN
  spawn_driver = &(madeleine->driver[EXTERNAL_SPAWN]);
  spawn_interface = &(madeleine->driver[EXTERNAL_SPAWN].interface);
  spawn_driver->specific = NULL;
  spawn_interface->driver_init(spawn_driver);
  spawn_adapter = &(madeleine->adapter[0]);
  spawn_adapter->specific = NULL;
  if (spawn_interface->adapter_init)
    spawn_interface->adapter_init(spawn_adapter);
  if (spawn_interface->external_spawn_init)
    spawn_interface->external_spawn_init(spawn_adapter, argc, argv);
#endif /* EXTERNAL_SPAWN */
  
  mad_parse_command_line(argc,
			 argv,
			 madeleine,
#ifdef PM2
			 conf_file,
#endif /* PM2 */
			 &master,
			 &slave);
  
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
#else /* EXTERNAL_SPAWN */
  mad_read_conf(configuration, configuration_file);
  rank = configuration->local_host_id;  

  master = !slave;
  
  if (!master && !slave)
    {
      mad_master_spawn(argc,
		       argv,
		       configuration);
    }
#endif /* EXTERNAL SPAWN */

  /* output redirection */
  if (rank > 0)
    {
      char   output[MAX_ARG_LEN];
      int    f;

      sprintf(output, "/tmp/%s-mad2log-%d", getenv("USER"), (int)rank);
      f = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
      dup2(f, STDOUT_FILENO);
      dup2(STDOUT_FILENO, STDERR_FILENO);
    }
  
  mad_driver_init(madeleine);
#ifdef EXTERNAL_SPAWN
  mad_connect_hosts(madeleine);
#else /* EXTERNAL_SPAWN */
  mad_connect_hosts(madeleine, argc, argv);
#endif /* EXTERNAL_SPAWN */
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
  PM2_LOCK_SHARED(madeleine);
  mad_close_channels(madeleine);
  mad_driver_exit(madeleine);
  tbx_list_manager_exit();
  PM2_UNLOCK_SHARED(madeleine);
  LOG_OUT();
}

/*
p_mad_channel_t
mad_get_channel(mad_channel_id_t id)
{
  tbx_list_reference_t ref;
  
  if (tbx_empty_list(&(main_madeleine.channel)))
    return NULL;
  
  tbx_list_reference_init(&ref, &(main_madeleine.channel));

  do
    {
      p_mad_channel_t tmp_channel = tbx_get_list_reference_object(&ref);
      
      if (tmp_channel->id == id)
	{
	  return tmp_channel;
	}    
    }
  while(tbx_forward_list_reference(&ref));

  return NULL;
}
*/

