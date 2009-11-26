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
#include <nm_session_interface.h>
#include <nm_session_private.h>
#include <Padico/Puk.h>
#include <pm2_common.h>

#include <string.h>
#include <assert.h>

static struct
{
  struct nm_core*p_core;
  struct nm_drv*p_drv;
  const char*local_url;
  puk_hashtable_t gates;
  int ref_count;
} nm_session =
  {
    .p_core    = NULL,
    .p_drv     = NULL,
    .local_url = NULL,
    .gates     = NULL,
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

/** get default drivers if no rails were given on the command line */
static int get_default_driver_assemblies(puk_component_t*assemblies)
{
  int cur_nr_drivers = 0;

  /* load all built-in drivers  */
#if defined CONFIG_MX
  assemblies[cur_nr_drivers++] = load_driver("mx");
#endif
#ifdef CONFIG_QSNET
  assemblies[cur_nr_drivers++] = load_driver("qsnet");
#endif
#if defined CONFIG_IBVERBS
  assemblies[cur_nr_drivers++] = load_driver("ibverbs");
#endif
#if defined CONFIG_GM
  assemblies[cur_nr_drivers++] = load_driver("gm");
#endif
#if defined CONFIG_TCP
  assemblies[cur_nr_drivers++] = load_driver("tcp");
#endif
#if defined CONFIG_LOCAL
  assemblies[cur_nr_drivers++] = load_driver("local");
#endif

  return cur_nr_drivers;
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


/** Initializes the global nmad core */
static void nm_session_do_init(int*argc, char**argv)
{
  /* init core */
  int err = nm_core_init(argc, argv, &nm_session.p_core);
  assert(err == NM_ESUCCESS);
  
  /* load driver */
  const char*driver_name = getenv("NMAD_DRIVER");
  if(!driver_name)
    driver_name = "custom";
  puk_component_t driver_assembly = nm_core_component_load("driver", driver_name);
  const char*driver_url = NULL;
  err = nm_core_driver_load_init(nm_session.p_core, driver_assembly, &nm_session.p_drv, &driver_url);
  assert(err == NM_ESUCCESS);
  nm_session.local_url = strdup(driver_url);
  /* TODO: add custom railstring support with:
   *   . default driver
       *   . multi-rail
       *   . configuration through argv / environment variable
       */
  nm_session.gates = puk_hashtable_new_string();
}


/* ********************************************************* */
/* ** Session interface implementation */

int nm_session_create(nm_session_t*pp_session, const char*label)
{
  struct nm_session_s*p_session = TBX_MALLOC(sizeof(struct nm_session_s));
  assert(label != NULL);
  assert(pp_session != NULL);
  p_session->p_core = NULL;
  p_session->label = strdup(label);
  const int len = strlen(label);
  p_session->hash_code = puk_hash_oneatatime((void*)label, len);
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
  *p_local_url = nm_session.local_url;

  return NM_ESUCCESS;
}

int nm_session_connect(nm_session_t p_session, nm_gate_t*pp_gate, const char*url)
{
  assert(p_session != NULL);
  assert(nm_session.gates != NULL);
  nm_gate_t p_gate = puk_hashtable_lookup(nm_session.gates, url);
  if(!p_gate)
    {
      assert(nm_session.p_core != NULL);
      int err = nm_core_gate_init(nm_session.p_core, &p_gate);
      assert(err == NM_ESUCCESS);
      const int is_server = (strcmp(url, nm_session.local_url) > 0);
      if(is_server)
	{
	  err = nm_core_gate_accept(nm_session.p_core, p_gate, nm_session.p_drv, url);
	  assert(err == NM_ESUCCESS);
	}
      else
	{
	  err = nm_core_gate_connect(nm_session.p_core, p_gate, nm_session.p_drv, url);
	  assert(err == NM_ESUCCESS);
	}
      char*key_url = strdup(url);
      puk_hashtable_insert(nm_session.gates, key_url, p_gate);
    }
  *pp_gate = p_gate;
  return NM_ESUCCESS;
}

int nm_session_destroy(nm_session_t p_session)
{
  TBX_FREE(p_session);
  nm_session.ref_count--;
  if(nm_session.ref_count == 0)
    {
      nm_core_exit(nm_session.p_core);
      nm_session.p_core = NULL;
      nm_session.p_drv = NULL;
      free((void*)nm_session.local_url);
      nm_session.local_url = NULL;
    }
  return NM_ESUCCESS;
}

struct nm_core*nm_session_get_core(nm_session_t p_session)
{
  return p_session->p_core;
}
