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
#ifdef MAD2_MAD1
#include <sys/wait.h>
#endif /* MAD2_MAD1 */

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

#ifdef MAD2_MAD1
marcel_key_t         mad2_send_key, mad2_recv_key;
#define ALIGNMASK        (MAD_ALIGNMENT-1)
/* semaphore used to block sends during slot negociations: */
_PRIVATE_ extern marcel_key_t        _pm2_isomalloc_nego_key;
                 marcel_sem_t         sem_nego;
#endif /* MAD2_MAD1 */

/* ---- */

void *mad_aligned_malloc(size_t   size,
			 int      align)
{
  char      *ptr;
  char      *ini;
  unsigned   mask = align - 1;

  ini = ptr = malloc (size + 2 * align - 1);

  if (ptr != NULL && ((unsigned) ptr & mask) != 0)
    {
      ptr = (char *) (((unsigned) ptr + mask) & ~mask);
    }

  if (ptr != NULL)
    {
      *(char **) ptr = ini;
      ptr += align;
    }

  return (void *)ptr;
}

void mad_aligned_free (void  *ptr,
		       int    align)
{
  free (*(char **) ((char *) ptr - align));
}

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
mad_manager_init(p_mad_madeleine_t madeleine)
{
  LOG_IN();
  mad_memory_manager_init();
  mad_list_manager_init();
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

      mad_list_init(&(driver->adapter_list));
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
      mad_append_list(&(driver->adapter_list), adapter);
      
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
#ifdef MAD2_MAD1
		       char               *conf_file,
#endif /* MAD2_MAD1 */
		       p_mad_bool_t        master,
		       p_mad_bool_t        slave)
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
	  *master = mad_true;
	}
      else if(!strcmp(argv[i], "-slave"))
	{
	  *slave = mad_true;
	}
      else if(!strcmp(argv[i], "-rank"))
	{
	  if (i == ((*argc) - 1))
	    FAILURE("-rank option must be followed "
		    "by the rank of the process");

	  madeleine->configuration.local_host_id = atoi(argv[i + 1]);
	  i++;
	}
#ifdef MAD2_MAD1
      else if (!strcmp(argv[i], "-conf"))
	{
	  if (i == ((*argc) - 1))
	    FAILURE("-conf must be followed "
		    "by the path of mad2 root directory");

	  sprintf(conf_file, "%s/.mad2_conf", argv[i + 1]);
	  i++;
	}
#endif /* MAD2_MAD1 */
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
  
  cwd = getcwd(NULL, MAX_ARG_LEN);
  if(cwd == NULL)
    {
      perror("getcwd");
      exit(1);
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
#ifdef MAD2_MAD1
      sprintf(cmd,
	      "rsh %s %s/%s -master -cwd %s -rank %d -conf %s %s",
	      configuration->host_name[0],
	      cwd, argv[0], cwd, 0, getenv("MAD2_ROOT"), arg_str);
#else /* MAD2_MAD1 */
      sprintf(cmd,
	      "rsh %s %s/%s -master -cwd %s -rank %d %s",
	      configuration->host_name[0],
	      cwd, argv[0], cwd, 0, arg_str);
#endif /* MAD2_MAD1 */
    }
  else
    {
#ifdef MAD2_MAD1
      sprintf(cmd,
	      "rsh %s %s -master -rank %d -conf %s %s",
	      configuration->host_name[0],
	      argv[0], 0, getenv("MAD2_ROOT"), arg_str);
#else /* MAD2_MAD1 */
      sprintf(cmd,
	      "rsh %s %s -master -rank %d %s",
	      configuration->host_name[0], argv[0], 0, arg_str);
#endif /* MAD2_MAD1 */
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

  cwd = getcwd(NULL, MAX_ARG_LEN);
  if(cwd == NULL)
    {
      perror("getcwd");
      exit(1);
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
#ifdef MAD2_MAD1
	  sprintf(cmd,
		  "rsh %s %s/%s -slave -cwd %s -rank %d -conf %s %s &",
		  configuration->host_name[i],
		  cwd,
		  argv[0],
		  cwd,
		  i,  /* rank */
		  getenv("MAD2_ROOT"),
		  arg_str);
#else /* MAD2_MAD1 */
	  sprintf(cmd,
		  "rsh %s %s/%s -slave -cwd %s -rank %d %s &",
		  configuration->host_name[i],
		  cwd,
		  argv[0],
		  cwd,
		  i,  /* rank */
		  arg_str);
#endif /* MAD2_MAD1 */
	}
      else
	{
#ifdef MAD2_MAD1
	  sprintf(cmd,
		  "rsh %s %s -slave -cwd %s -rank %d -conf %s %s &",
		  configuration->host_name[i],
		  argv[0],
		  cwd,
		  i,  /* rank */
		  getenv("MAD2_ROOT"),
		  arg_str);
#else /* MAD2_MAD1 */
	  sprintf(cmd,
		  "rsh %s %s -slave -cwd %s -rank %d %s &",
		  configuration->host_name[i],
		  argv[0],
		  cwd,
		  i,  /* rank */
		  arg_str);
#endif /* MAD2_MAD1 */
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
mad_connect_hosts(p_mad_madeleine_t   madeleine,
		  int                *argc,
		  char              **argv)
{
  p_mad_driver_interface_t spawn_interface;
  p_mad_adapter_t          spawn_adapter;
  mad_adapter_id_t ad;
  mad_host_id_t rank;

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
      if (adapter->driver->interface.adapter_init)
	adapter->driver->interface.adapter_init(adapter);
      if (!rank)
	{
	  mad_host_id_t host_id;

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
  mad_host_id_t rank;

  LOG_IN();
  rank = madeleine->configuration.local_host_id;

  for (ad = 0;
       ad < madeleine->nb_adapter;
       ad++)
    {
      p_mad_adapter_t adapter;
      
      adapter = &(madeleine->adapter[ad]);
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

#ifdef MAD2_MAD1
  {
    int ret; 
    char command[128];

    sprintf(command, "exit `cat %s | wc -w`", configuration_file);     
    ret = system(command);
    configuration->size = WEXITSTATUS(ret);
  }
#else /* MAD2_MAD1 */
  fscanf(f, "%d", &(configuration->size));
#endif /* MAD2_MAD1 */
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
  interface->adapter_exit(adapter);
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

      mad_foreach_destroy_list(&(madeleine->channel),
			       mad_foreach_adapter_exit);
      interface->driver_exit(driver);
    }

  free(madeleine->adapter);
  free(madeleine->driver);
  LOG_OUT();
}

/* ---- */

#ifdef MAD2_MAD1
p_mad_madeleine_t
mad2_init(int                  *argc,
	  char                **argv,
	  char                 *configuration_file,
	  p_mad_adapter_set_t   adapter_set)
#else /* MAD2_MAD1 */
p_mad_madeleine_t
mad_init(int                   *argc,
	 char                 **argv,
	 char                  *configuration_file,
	 p_mad_adapter_set_t    adapter_set)
#endif /* MAD2_MAD1 */
{
  p_mad_madeleine_t          madeleine       = &(main_madeleine);
  p_mad_configuration_t      configuration   =
    &(madeleine->configuration);
  mad_host_id_t              rank            = -1;
  mad_bool_t                 master          = mad_false;
  mad_bool_t                 slave           = mad_false;
#ifdef EXTERNAL_SPAWN
  p_mad_driver_t             spawn_driver    = NULL;
  p_mad_driver_interface_t   spawn_interface = NULL;
  p_mad_adapter_t            spawn_adapter   = NULL;
#endif /* EXTERNAL_SPAWN */
#ifdef MAD2_MAD1
  char                       conf_file[128];
  
  configuration_file = conf_file;
  sprintf(conf_file, "%s/.mad2_conf", getenv("MAD2_ROOT"));
#endif /* MAD2_MAD1 */

  LOG_IN(); 
#ifdef TIMING
  mad_timing_init();
#endif /* TIMING */

#ifdef PM2  
  marcel_init(argc, argv);
#endif /* PM2 */

  madeleine->nb_channel = 0;
  PM2_INIT_SHARED(madeleine);
  mad_manager_init(madeleine);
  mad_driver_fill(madeleine);
  mad_adapter_fill(madeleine, adapter_set);  

#ifdef EXTERNAL_SPAWN
  spawn_driver = &(madeleine->driver[EXTERNAL_SPAWN]);
  spawn_interface = &(madeleine->driver[EXTERNAL_SPAWN].interface);
  spawn_interface->driver_init(spawn_driver);
  spawn_adapter = &(madeleine->adapter[0]);
  if (spawn_interface->adapter_init)
    spawn_interface->adapter_init(spawn_adapter);
  spawn_interface->external_spawn_init(spawn_adapter, argc, argv);
#endif /* EXTERNAL_SPAWN */
  
  mad_parse_command_line(argc,
			 argv,
			 madeleine,
#ifdef MAD2_MAD1
			 conf_file,
#endif /* MAD2_MAD1 */
			 &master,
			 &slave);
  
#ifdef EXTERNAL_SPAWN
  spawn_interface->configuration_init(spawn_adapter, configuration);
  if (spawn_interface->adapter_configuration_init)
    spawn_interface->adapter_configuration_init(spawn_adapter);
  rank = configuration->local_host_id;

  if (rank == 0)
    {
      master = mad_true;
      slave  = mad_false;
    }
  else
    {
      master = mad_false;
      slave  = mad_true;
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
  mad_connect_hosts(madeleine, argc, argv);
  mad_list_init(&(madeleine->channel));

  LOG_OUT();
  return madeleine;
}

#ifdef MAD2_MAD1
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
  mad_list_manager_exit();
  PM2_UNLOCK_SHARED(madeleine);
  LOG_OUT();
}

p_mad_channel_t
mad_get_channel(mad_channel_id_t id)
{
  mad_list_reference_t ref;
  
  if (mad_empty_list(&(main_madeleine.channel)))
    return NULL;
  
  mad_list_reference_init(&ref, &(main_madeleine.channel));

  do
    {
      p_mad_channel_t tmp_channel = mad_get_list_reference_object(&ref);
      
      if (tmp_channel->id == id)
	{
	  return tmp_channel;
	}    
    }
  while(mad_forward_list_reference(&ref));

  return NULL;
}


/* ========================================================================
 *
 * Interface de compatibilite ascendante Mad2/Mad1
 *
 * ========================================================================
 */
#ifdef MAD2_MAD1
void
mad_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  /* Note:
   *  - l'objet global madeleine est utilise
   *    directement par la suite, il n'est donc
   *    pas utile de renvoyer un pointeur sur cet objet
   */
  p_mad_madeleine_t     madeleine;
  p_mad_adapter_set_t   adapter_set;
  
  LOG_IN();

  adapter_set =
    mad_adapter_set_init(1, MAD2_MAD1_MAIN_PROTO, MAD2_MAD1_MAIN_PROTO_PARAM);

  madeleine    = mad2_init(argc, argv, NULL, adapter_set);

  *nb = madeleine->configuration.size;
  *whoami = (int)madeleine->configuration.local_host_id;

  LOG_VAL("Who am I:", *whoami);
  {
    int i;

    for (i = 0; i < *nb; i++)
      tids[i] = i;
  }

  LOG_OUT();
}

void
mad_buffers_init(void)
{
  marcel_key_create(&mad2_send_key, NULL);
  marcel_key_create(&mad2_recv_key, NULL);
  marcel_sem_init(&sem_nego, 1);
}

void
mad_exit(void)
{
  p_mad_madeleine_t madeleine = &main_madeleine;
  
  LOG_IN();
  mad2_exit(madeleine);
  LOG_OUT();
}

char *
mad_arch_name(void)
{
  LOG_IN();
  LOG_OUT();
  return NET_ARCH;
}

boolean
mad_can_send_to_self(void)
{
  LOG_IN();
  LOG_OUT();
  return 0;
}

void
mad_sendbuf_init(p_mad_channel_t channel, int dest_node)
{
  LOG_IN();
  marcel_setspecific(mad2_send_key, mad_begin_packing(channel, dest_node));
  LOG_OUT();
}

void
mad_sendbuf_send(void)
{
  LOG_IN();
  mad_end_packing(marcel_getspecific(mad2_send_key));
  LOG_OUT();
}

void
mad_sendbuf_free(void)
{
  /* rien a priori */
}

void
mad_receive(p_mad_channel_t channel)
{
  LOG_IN();
  marcel_setspecific(mad2_recv_key, mad_begin_unpacking(channel));
  LOG_OUT();
}

void
mad_recvbuf_receive(void)
{
  LOG_IN();
  mad_end_unpacking(marcel_getspecific(mad2_recv_key));
  LOG_OUT();
}

void
mad_pack_byte(madeleine_part where, char *data, size_t nb)
{
  LOG_IN();
  
   switch(where)
    {
       case MAD_IN_HEADER :
	 {
	   mad_pack(marcel_getspecific(mad2_send_key), data, nb,
		    mad_send_SAFER, mad_receive_EXPRESS);
	   break;
	 }
       case MAD_IN_PLACE :
	 {
	   mad_pack(marcel_getspecific(mad2_send_key), data, nb,
		    mad_send_CHEAPER, mad_receive_CHEAPER);
	   break;
	 }
       case MAD_BY_COPY :
	 {
	   mad_pack(marcel_getspecific(mad2_send_key), data, nb,
		    mad_send_SAFER, mad_receive_CHEAPER);
	   break;
	 }
    default: FAILURE("Unknown pack mode");
    }

  LOG_OUT();
}

void
mad_unpack_byte(madeleine_part where, char *data, size_t nb)
{
  LOG_IN();
  
  switch(where)
    {
       case MAD_IN_HEADER :
	 {
	   mad_unpack(marcel_getspecific(mad2_recv_key), data, nb,
		      mad_send_SAFER, mad_receive_EXPRESS);
	   break;
	 }
       case MAD_IN_PLACE :
	 {
	   mad_unpack(marcel_getspecific(mad2_recv_key), data, nb,
		      mad_send_CHEAPER, mad_receive_CHEAPER);
	   break;
	 }
       case MAD_BY_COPY :
	 {
	   mad_unpack(marcel_getspecific(mad2_recv_key), data, nb,
		      mad_send_SAFER, mad_receive_CHEAPER);
	   break;
	 }
    default: FAILURE("Unknown pack mode");
    }

  LOG_OUT();
}

void
mad_pack_short(madeleine_part where, short *data, size_t nb)
{
  LOG_IN();
  mad_pack_byte(where, (char *)data, nb*sizeof(short));
  LOG_OUT();
}

void
mad_unpack_short(madeleine_part where, short *data, size_t nb)
{
  LOG_IN();
  mad_unpack_byte(where, (char *)data, nb*sizeof(short));
  LOG_OUT();
}

void
mad_pack_int(madeleine_part where, int *data, size_t nb)
{
  LOG_IN();
  mad_pack_byte(where, (char *)data, nb*sizeof(int));
  LOG_OUT();
}

void
mad_unpack_int(madeleine_part where, int *data, size_t nb)
{
  LOG_IN();
  mad_unpack_byte(where, (char *)data, nb*sizeof(int));
  LOG_OUT();
}

void
mad_pack_long(madeleine_part where, long *data, size_t nb)
{
  LOG_IN();
  mad_pack_byte(where, (char *)data, nb*sizeof(long));
  LOG_OUT();
}

void
mad_unpack_long(madeleine_part where, long *data, size_t nb)
{
  LOG_IN();
  mad_unpack_byte(where, (char *)data, nb*sizeof(long));
  LOG_OUT();
}

void
mad_pack_float(madeleine_part where, float *data, size_t nb)
{
  LOG_IN();
  mad_pack_byte(where, (char *)data, nb*sizeof(float));
  LOG_OUT();
}

void
mad_unpack_float(madeleine_part where, float *data, size_t nb)
{
  LOG_IN();
  mad_unpack_byte(where, (char *)data, nb*sizeof(float));
  LOG_OUT();
}

void
mad_pack_double(madeleine_part where, double *data, size_t nb)
{
  LOG_IN();
  mad_pack_byte(where, (char *)data, nb*sizeof(double));
  LOG_OUT();
}

void
mad_unpack_double(madeleine_part where, double *data, size_t nb)
{
  LOG_IN();
  mad_unpack_byte(where, (char *)data, nb*sizeof(double));
  LOG_OUT();
}

void
mad_pack_pointer(madeleine_part where, pointer *data, size_t nb)
{
  LOG_IN();
  mad_pack_byte(where, (char *)data, nb*sizeof(pointer));
  LOG_OUT();
}

void
mad_unpack_pointer(madeleine_part where, pointer *data, size_t nb)
{
  LOG_IN();
  mad_unpack_byte(where, (char *)data, nb*sizeof(pointer));
  LOG_OUT();
}

void
mad_pack_str(madeleine_part where, char *data)
{
  int len = strlen(data);
  LOG_IN();
  mad_pack(marcel_getspecific(mad2_send_key), (char *)&len, sizeof(int),
	   mad_send_SAFER, mad_receive_EXPRESS);
  mad_pack_byte(where, data, len + 1);
  LOG_OUT();
}

void
mad_unpack_str(madeleine_part where, char *data)
{
  int len;
  LOG_IN();
  mad_unpack(marcel_getspecific(mad2_recv_key), (char *)&len, sizeof(int),
	     mad_send_SAFER, mad_receive_EXPRESS);
  mad_unpack_byte(where, data, len + 1);
  LOG_OUT();
}

/* This function is supposed to be called after pm2_init/mad_init. */
p_mad_madeleine_t mad_get_madeleine()
{
  return &main_madeleine;
}

#endif /* MAD2_MAD1 */
