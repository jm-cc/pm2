/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <nm_public.h>
#include <nm_private.h>
#include <nm_core_interface.h>
#include <nm_session_interface.h>
#include <nm_session_private.h>
#include <nm_sendrecv_interface.h>
#include <Padico/Puk.h>
#include <pm2_common.h>

#include <string.h>
#include <assert.h>

static struct
{
  struct nm_core*p_core;    /**< the current nmad object. */
  int n_drivers;            /**< number of drivers */
  struct nm_drv**drivers;   /**< array of drivers */
  struct nm_drv*p_drv;      /**< the driver used for all connections */
  const char*local_url;     /**< the local nmad driver url */
  puk_hashtable_t gates;    /**< list of connected gates */
  puk_hashtable_t sessions; /**< table of active sessions, hashed by session hashcode */
  int ref_count;            /**< ref. counter for core = number of active sessions */
} nm_session =
  {
    .p_core    = NULL,
    .n_drivers = 0,
    .drivers   = NULL,
    .local_url = NULL,
    .gates     = NULL,
    .sessions  = NULL,
    .ref_count = 0
  };


/* ********************************************************* */

#define RAIL_MAX 4

static void usage(void)
{
  fprintf(stderr, "usage: <prog> [-R <rail1+rail2...>] [<remote url> ...]\n");
  fprintf(stderr, "  The number of rails must be less than or equal to %d, and there must be as many rails followed by the server's URLs on the sender's side\n", RAIL_MAX);
  fprintf(stderr, "  Available rails are:\n");
#if defined CONFIG_MX
  fprintf(stderr, "    mx    to use      any MX board\n");
  fprintf(stderr, "    mx:N  to use      the Nth MX board\n");
#endif
#if defined CONFIG_QSNET
  fprintf(stderr, "    qsnet to use      the QsNet board\n");
#endif
#if defined CONFIG_IBVERBS
  fprintf(stderr, "    ib    to use      any InfiniBand device\n");
  fprintf(stderr, "    ib:N  to use      the Nth InfiniBand device\n");
#endif
#if defined CONFIG_GM
  fprintf(stderr, "    gm    to use      any GM board\n");
#endif
#if defined CONFIG_TCP
  fprintf(stderr, "    tcp   to use      any TCP connection\n");
#endif
  exit(EXIT_FAILURE);
}

static inline puk_component_t load_driver(const char *driver_name)
{
  return nm_core_component_load("driver", driver_name);
}

#if defined(CONFIG_MX) || defined(CONFIG_IBVERBS)
/** look for a driver string in a rail and check whether the board id is forced  */
static int lookup_rail_driver_and_board_id(const char *rail, const char *driver,
					   struct nm_driver_query_param *param)
{
  int len = strlen(driver);

  if (!strcmp(rail, driver))
    return 1;

  /* look for driver:N */
  if (strncmp(rail, driver, len))
    return 0;
  if (rail[len] != ':')
    return 0;

  param->key = NM_DRIVER_QUERY_BY_INDEX;
  param->value.index = atoi(rail+len+1);
  return 1;
}
#endif

/** handle one rail from the railstring */
static int handle_one_rail(char *token, int index,
			   puk_component_t*assembly,
			   struct nm_driver_query_param *param)
{
#if defined CONFIG_MX
  if (lookup_rail_driver_and_board_id(token, "mx", param)) {
    if (param->key == NM_DRIVER_QUERY_BY_INDEX)
      printf("# launcher: using MX board #%d for rail #%d\n",
	     param->value.index, index);
    else
      printf("# launcher: using any MX board for rail #%d\n",
	     index);
    *assembly = load_driver("mx");
  } else
#endif
#ifdef CONFIG_QSNET
  if (!strcmp("qsnet", token)) {
    printf("# launcher: using QsNet for rail #%d\n", index);
    *assembly = load_driver("qsnet");
  } else
#endif
#if defined CONFIG_IBVERBS
  if (lookup_rail_driver_and_board_id(token, "ib", param)
      || lookup_rail_driver_and_board_id(token, "ibv", param)
      || lookup_rail_driver_and_board_id(token, "ibverbs", param)) {
    if (param->key == NM_DRIVER_QUERY_BY_INDEX)
      printf("# launcher: using IB device #%d for rail #%d\n",
	     param->value.index, index);
    else
      printf("# launcher: using any IB device for rail #%d\n",
	     index);
    *assembly = load_driver("ibverbs");
  } else
#endif
#if defined CONFIG_GM
  if (!strcmp("gm", token)) {
    printf("# launcher: using GM for rail #%d\n", index);
    *assembly = load_driver("gm");
  } else
#endif
#if defined CONFIG_TCP
  if (!strcmp("tcp", token)) {
    printf("# launcher: using TCP for rail #%d\n", index);
    *assembly = load_driver("tcp");
  } else 
#endif
    {
      fprintf(stderr, "launcher: unrecognized rail \"%s\"\n", token);
      return -1;
    }

  return 0;
}

/** get drivers from the railstring from the command line */
static int get_railstring_driver_assemblies(puk_component_t*assemblies,
					    struct nm_driver_query_param *params,
					    char * railstring)
{
  int cur_nr_drivers = 0;
  char * token;
  int err;

  token = strtok(railstring, "+");
  while (token) {
    err = handle_one_rail(token, cur_nr_drivers,
			  &assemblies[cur_nr_drivers],
			  &params[cur_nr_drivers]);
    if (err < 0)
      usage();
    cur_nr_drivers++;

    if (cur_nr_drivers > RAIL_MAX) {
      fprintf(stderr, "launcher: found too many drivers, only %d are supported\n", RAIL_MAX);
      usage();
    }

    token = strtok(NULL, "+");
  }

  return cur_nr_drivers;
}

/* legacy init from cmdline launcher
 * to be backported to the nm_session_do_init()
 */
#if 0
static void nm_session_legacy_init(int*argc, char**argv)
{
  int i,j, err;
  /* rails */
  char *railstring = NULL;
  int nr_r_urls = 0;
  /* per rail arrays */
  puk_component_t driver_assemblies[RAIL_MAX];
  struct nm_driver_query_param params[RAIL_MAX];
  struct nm_driver_query_param *params_array[RAIL_MAX];
  int nparam_array[RAIL_MAX];
  const char *r_url[RAIL_MAX];
  nm_drv_t drvs[RAIL_MAX];

  for(i=0; i<RAIL_MAX; i++) {
    params[i].key = NM_DRIVER_QUERY_BY_NOTHING;
    params_array[i] = &params[i];
    nparam_array[i] = 1;
  }

  err = nm_core_init(argc, argv, &status->p_core);
  if (err != NM_ESUCCESS) {
    fprintf(stderr, "launcher: nm_core_init returned err = %d\n", err);
    goto out_err;
  }

  i=1;
  while (i<*argc) {
    /* handle -R for rails */
    if (!strcmp(argv[i], "-R")) {
      if (*argc <= i+1)
	usage();
      railstring = argv[i+1];
      i += 2;
      continue;
    }
    /* other options have to be passed after rails and url */
    else if (!strncmp(argv[i], "-", 1)) {
      break;
    }
    /* handle urls */
    else {
      if (nr_r_urls == RAIL_MAX) {
	fprintf(stderr, "launcher: found too many url, only %d are supported\n", RAIL_MAX);
	usage();
      }
      r_url[nr_r_urls++] = argv[i];
      i++;
    }
  }

  /* update command line before returning to the program */
  i--;
  *argc -= i;
  for(j=1; j<*argc; j++) {
    argv[j] = argv[j+i];
  }

  if (railstring) 
    {
      /* parse railstring to get drivers */
      status->nr_rails = get_railstring_driver_assemblies(driver_assemblies, params, railstring);
    }
  else
    {
      /* use default drivers */
      status->nr_rails = get_default_driver_assemblies(driver_assemblies);
      if (status->nr_rails < 0)
	{
	  fprintf(stderr, "launcher: failed to select default drivers automatically.\n");
	  usage();
	}
      if(status->nr_rails == 0)
	{
	  fprintf(stderr, "launcher: no driver enabled.\n");
	  usage();
	}
    }
  

  /* if client, we need as many url as drivers */
  if (!status->is_server && nr_r_urls < status->nr_rails) {
    fprintf(stderr, "launcher: need %d url for these %d rails\n", nr_r_urls, status->nr_rails);
    usage();
  }

  if (status->is_server) {
    printf("# launcher: running as server\n");
  } else {
    printf("# launcher: running as client using remote url:");
    for(j = 0; j < status->nr_rails; j++)
      printf(" %s", r_url[j]);
    printf("\n");
  }

  err = nm_core_driver_load_init_some_with_params(status->p_core, status->nr_rails, driver_assemblies, params_array, nparam_array, drvs, status->l_url);
  if (err != NM_ESUCCESS) {
    fprintf(stderr, "launcher: nm_core_driver_load_init_some returned err = %d\n", err);
    goto out_err;
  }
  for(j = 0; j < status->nr_rails; j++) {
    printf("# launcher: local url[%d]: '%s'\n", j, status->l_url[j]);
  }

  err = nm_core_gate_init(status->p_core, &status->gate);
  if (err != NM_ESUCCESS) {
    fprintf(stderr, "launcher: nm_core_gate_init returned err = %d\n", err);
    goto out_err;
  }
}
#endif /* 0 - legacy code */

int nm_session_code_eq(const void*key1, const void*key2)
{
  const uint32_t*p_code1 = key1;
  const uint32_t*p_code2 = key2;
  return (*p_code1 == *p_code2);
}

uint32_t nm_session_code_hash(const void*key)
{
  const uint32_t*p_hash_code = key;
  return *p_hash_code;
}

/** Initialize the strategy */
static void nm_session_init_strategy(int*argc, char**argv)
{
  const char*strategy_name = getenv("NMAD_STRATEGY");
  if(!strategy_name)
    {
#if defined(CONFIG_STRAT_SPLIT_BALANCE)
      strategy_name = "split_balance";
#elif defined(CONFIG_STRAT_SPLIT_ALL)
      strategy_name = "split_all";
#elif defined(CONFIG_STRAT_DEFAULT)
      strategy_name = "default";
#elif defined(CONFIG_STRAT_AGGREG)
      strategy_name = "aggreg";
#elif defined(CONFIG_STRAT_AGGREG_AUTOEXTENDED)
      strategy_name = "aggreg_autoextended";
#else /*  defined(CONFIG_STRAT_CUSTOM) */
      strategy_name = "custom";
#endif
    }
  puk_component_t strategy = nm_core_component_load("strategy", strategy_name);
  int err = nm_core_set_strategy(nm_session.p_core, strategy);
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "# session: error %d while loading strategy %s.\n", err, strategy_name);
      abort();
    }
}

/** Initialize the drivers */
static void nm_session_init_drivers(int*argc, char**argv)
{
  const char*assembly_name = getenv("NMAD_ASSEMBLY");
  if(assembly_name)
    {
      /* assembly name given (e.g. NewMadico) */
      fprintf(stderr, "# session: loading assembly %s\n", assembly_name);
      puk_component_t driver_assembly = puk_adapter_resolve(assembly_name);
      assert(driver_assembly != NULL);
      const char*driver_url = NULL;
      struct nm_drv*p_drv = NULL;
      struct nm_driver_query_param param = { .key = NM_DRIVER_QUERY_BY_NOTHING };
      int err = nm_core_driver_load_init(nm_session.p_core, driver_assembly, &param, &p_drv, &driver_url);
      assert(err == NM_ESUCCESS);
      const struct nm_drv_iface_s*drv_iface = puk_adapter_get_driver_NewMad_Driver(driver_assembly, NULL);
      assert(drv_iface != NULL);
      const char*driver_name = drv_iface->name;
      padico_string_t url_string = padico_string_new();
      padico_string_catf(url_string, "%s/%s", driver_name, driver_url);
      nm_session.n_drivers = 1;
      nm_session.drivers = malloc(sizeof(struct nm_drv*));
      nm_session.drivers[0] = p_drv;
      nm_session.local_url = strdup(padico_string_get(url_string));
      padico_string_delete(url_string);
    }
  else
    {
      const char*driver_env = getenv("NMAD_DRIVER");
      if(!driver_env)
	{
	  driver_env = "custom";
	}
      /* parse driver_string */
      nm_session.n_drivers = 0;
      char*driver_string = strdup(driver_env); /* strtok writes into the string */
      padico_string_t url_string = NULL;
      char*token = strtok(driver_string, "+");
      while(token)
	{
	  char*driver_name = strdup(token); /* take a copy so as our writes don't confuse strtok */
	  char*index_string = strchr(driver_name, ':');
	  int index = -1;
	  if(index_string)
	    {
	      *index_string = '\0';
	      index_string++;
	      index = atoi(index_string);
	    }
	  if((strcmp(driver_name, "ib") == 0) || (strcmp(driver_name, "ibv") == 0))
	    {
	      free(driver_name);
	      driver_name = strdup("ibverbs");
	    }
	  else if((strcmp(driver_name, "myri") == 0) || (strcmp(driver_name, "myrinet") == 0))
	    {
	      free(driver_name);
	      driver_name = strdup("mx");
	    }
	  puk_component_t driver_assembly = nm_core_component_load("driver", driver_name);
	  assert(driver_assembly != NULL);
	  const char*driver_url = NULL;
	  struct nm_drv*p_drv = NULL;
	  struct nm_driver_query_param param = { .key = NM_DRIVER_QUERY_BY_NOTHING };
	  if(index >= 0)
	    {
	      param.key = NM_DRIVER_QUERY_BY_INDEX;
	      param.value.index = index;
	    }
	  int err = nm_core_driver_load_init(nm_session.p_core, driver_assembly, &param, &p_drv, &driver_url);
	  assert(err == NM_ESUCCESS);
	  const struct nm_drv_iface_s*drv_iface = puk_adapter_get_driver_NewMad_Driver(driver_assembly, NULL);
	  assert(drv_iface != NULL);
	  const char*driver_realname = drv_iface->name;
	  nm_session.n_drivers++;
	  nm_session.drivers = realloc(nm_session.drivers, nm_session.n_drivers * sizeof(struct nm_drv*));
	  nm_session.drivers[nm_session.n_drivers - 1] = p_drv;
	  if(url_string == NULL)
	    {
	      url_string = padico_string_new();
	      padico_string_catf(url_string, "%s/%s", driver_realname, driver_url);
	    }
	  else
	    {
	      padico_string_catf(url_string, "+%s/%s", driver_realname, driver_url);
	    }
	  free(driver_name);
	  token = strtok(NULL, "+");
	}
      nm_session.local_url = strdup(padico_string_get(url_string));
      padico_string_delete(url_string);
      free(driver_string);
    }
}

/** Initializes the global nmad core */
static void nm_session_do_init(int*argc, char**argv)
{
  /* init core */
  int err = nm_core_init(argc, argv, &nm_session.p_core);
  assert(err == NM_ESUCCESS);
  
  /* load strategy */
  nm_session_init_strategy(argc, argv);

  /* load driver */
  nm_session_init_drivers(argc, argv);
}


/* ********************************************************* */
/* ** Session interface implementation */

int nm_session_create(nm_session_t*pp_session, const char*label)
{
  if(!nm_session.sessions)
    {
      nm_session.gates = puk_hashtable_new_string();
      nm_session.sessions = puk_hashtable_new(&nm_session_code_hash, &nm_session_code_eq);
    }
  assert(label != NULL);
  assert(pp_session != NULL);
  const int len = strlen(label);
  const uint32_t hash_code = puk_hash_oneatatime((void*)label, len);
  struct nm_session_s*p_session = puk_hashtable_lookup(nm_session.sessions, &hash_code);
  if(p_session != NULL)
    {
      fprintf(stderr, "# session: collision detected while creating session %s- a session with hashcode %d already exist (%s)\n",
	      label, hash_code, p_session->label);
      abort();
    }
#ifndef NM_TAG_STRUCT
  if(puk_hashtable_size(nm_session.sessions) > 0)
    {
      fprintf(stderr, "# session: current flavor does not support multiple sessions. Please activate option 'tag_huge'.\n");
      abort();
    }
#endif /* NM_TAG_STRUCT */
  p_session = TBX_MALLOC(sizeof(struct nm_session_s));
  p_session->p_core = NULL;
  p_session->label = strdup(label);
  p_session->hash_code = hash_code;
  puk_hashtable_insert(nm_session.sessions, &p_session->hash_code, p_session);
  nm_session.ref_count++;
  *pp_session = p_session;
  return NM_ESUCCESS;
}

int nm_session_init(nm_session_t p_session, int*argc, char**argv, const char**p_local_url)
{
  assert(p_session != NULL);
  if(nm_session.p_core == NULL)
    {
      nm_session_do_init(argc, argv);
    }
  p_session->p_core = nm_session.p_core;
  nm_sr_init(p_session);
  *p_local_url = nm_session.local_url;

  return NM_ESUCCESS;
}

int nm_session_connect(nm_session_t p_session, nm_gate_t*pp_gate, const char*url)
{
  assert(p_session != NULL);
  assert(nm_session.gates != NULL);
  nm_gate_t p_gate = puk_hashtable_lookup(nm_session.gates, url);
  while(p_gate == NULL)
    {
      assert(nm_session.p_core != NULL);
      /* create gate */
      int err = nm_core_gate_init(nm_session.p_core, &p_gate);
      assert(err == NM_ESUCCESS);
      /* connect gate */
      const int is_server = (strcmp(url, nm_session.local_url) > 0);
      /* connect all drivers */
      char*parse_string = strdup(url);
      char*token = strtok(parse_string, "+");
      int i;
      for(i = 0; i < nm_session.n_drivers; i++)
	{
	  struct nm_drv*p_drv = nm_session.drivers[i];
	  char*driver_string = strdup(token);
	  char*driver_name = strdup(driver_string);
	  char*driver_url = strchr(driver_name, '/');
	  *driver_url = '\0';
	  driver_url++;
	  const struct nm_drv_iface_s*drv_iface = p_drv->driver;
	  if(strcmp(drv_iface->name, driver_name) != 0)
	    {
	      fprintf(stderr, "# session: local and peer driver do not match; remote = %s; local = %s\n", 
		      driver_name, drv_iface->name);
	      abort();
	    }
	  if(is_server)
	    {
	      err = nm_core_gate_accept(nm_session.p_core, p_gate, p_drv, driver_url);
	      assert(err == NM_ESUCCESS);
	    }
	  else
	    {
	      err = nm_core_gate_connect(nm_session.p_core, p_gate, p_drv, driver_url);
	      assert(err == NM_ESUCCESS);
	    }
	  free(driver_string);
	  token = strtok(NULL, "+");
	}
      free(parse_string);
      /* get peer identity */
      nm_sr_request_t sreq1, sreq2, rreq1, rreq2;
      nm_tag_t tag = 42;
      int local_url_size = strlen(nm_session.local_url);
      err = nm_sr_isend(p_session, p_gate, tag, &local_url_size, sizeof(local_url_size), &sreq1);
      if(err != NM_ESUCCESS)
	abort();
      nm_sr_isend(p_session, p_gate, tag, nm_session.local_url, local_url_size, &sreq2);
      int remote_url_size = -1;
      nm_sr_irecv(p_session, p_gate, tag, &remote_url_size, sizeof(remote_url_size), &rreq1);
      nm_sr_rwait(p_session, &rreq1);
      char*remote_url = TBX_MALLOC(remote_url_size + 1);
      nm_sr_irecv(p_session, p_gate, tag, remote_url, remote_url_size, &rreq2);
      nm_sr_rwait(p_session, &rreq2);
      nm_sr_swait(p_session, &sreq1);
      nm_sr_swait(p_session, &sreq2);
      remote_url[remote_url_size] = '\0';
      /* insert new gate into hashtable */
      puk_hashtable_insert(nm_session.gates, remote_url, p_gate);
      /* lookup again, in case the new gate is not the expected one */
      p_gate = puk_hashtable_lookup(nm_session.gates, url);
    }
  *pp_gate = p_gate;
  return NM_ESUCCESS;
}

int nm_session_destroy(nm_session_t p_session)
{
  puk_hashtable_remove(nm_session.sessions, p_session);
  TBX_FREE((void*)p_session->label);
  TBX_FREE(p_session);
  nm_session.ref_count--;
  if(nm_session.ref_count == 0)
    {
      nm_core_exit(nm_session.p_core);
      nm_session.p_core = NULL;
      if(nm_session.drivers)
	{
	  free(nm_session.drivers);
	  nm_session.drivers = NULL;
	}
      nm_session.n_drivers = 0;
      free((void*)nm_session.local_url);
      nm_session.local_url = NULL;
    }
  return NM_ESUCCESS;
}


