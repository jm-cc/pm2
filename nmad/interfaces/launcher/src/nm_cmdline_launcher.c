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

#include <Padico/Puk.h>
#include <nm_public.h>
#include <nm_private.h>
#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>

#include <string.h>


/* ** Cmd line launcher ************************************ */

static void*     nm_cmdline_launcher_instanciate(puk_instance_t i, puk_context_t c);
static void      nm_cmdline_launcher_destroy(void*_status);

const static struct puk_adapter_driver_s nm_cmdline_launcher_adapter_driver =
  {
    .instanciate = &nm_cmdline_launcher_instanciate,
    .destroy     = &nm_cmdline_launcher_destroy
  };

static void      nm_cmdline_launcher_init(void*_status, int*argc, char**argv, const char*group_name);
static int       nm_cmdline_launcher_get_size(void*_status);
static int       nm_cmdline_launcher_get_rank(void*_status);
static nm_core_t nm_cmdline_launcher_get_core(void*_status);
static void      nm_cmdline_launcher_get_gates(void*_status, nm_gate_t*gates);

const static struct newmad_launcher_driver_s nm_cmdline_launcher_driver =
  {
    .init      = &nm_cmdline_launcher_init,
    .get_size  = &nm_cmdline_launcher_get_size,
    .get_rank  = &nm_cmdline_launcher_get_rank,
    .get_core  = &nm_cmdline_launcher_get_core,
    .get_gates = &nm_cmdline_launcher_get_gates
  };

extern void nm_cmdline_launcher_declare(void)
{
  puk_adapter_declare("NewMad_Launcher",
                      puk_adapter_provides("PadicoAdapter", &nm_cmdline_launcher_adapter_driver),
                      puk_adapter_provides("NewMad_Launcher", &nm_cmdline_launcher_driver ));

}

#define RAIL_MAX NM_DRV_MAX

struct nm_cmdline_launcher_status_s
{
  nm_core_t p_core;
  nm_gate_t gate;
  int is_server;
  char *l_url[RAIL_MAX];
  int nr_rails;
};

static void*nm_cmdline_launcher_instanciate(puk_instance_t i, puk_context_t c)
{
  struct nm_cmdline_launcher_status_s*status = TBX_MALLOC(sizeof(struct nm_cmdline_launcher_status_s));
  status->p_core = NULL;
  status->gate = NM_GATE_NONE;
  status->is_server = 0;
  status->nr_rails = 0;
  return status;
}

static void nm_cmdline_launcher_destroy(void*_status)
{
  struct nm_cmdline_launcher_status_s*status = _status;
  int j, err, ret = NM_ESUCCESS;

  for(j = 0; j < status->nr_rails; j++) {
    free(status->l_url[j]);
  }

  err = nm_core_exit(status->p_core);
  if(err != NM_ESUCCESS) {
    fprintf(stderr, "launcher: nm_core__exit return err = %d\n", err);
    ret = EXIT_FAILURE;
  }

  TBX_FREE(status);

  common_exit(NULL);

}

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

static int nm_cmdline_launcher_get_rank(void*_status)
{
  struct nm_cmdline_launcher_status_s*status = _status;
  return status->is_server ? 0 : 1;
}

static int nm_cmdline_launcher_get_size(void*_status)
{
  return 2;
}

static nm_core_t nm_cmdline_launcher_get_core(void*_status)
{
  struct nm_cmdline_launcher_status_s*status = _status;
  return status->p_core;
}

static void nm_cmdline_launcher_get_gates(void*_status, nm_gate_t *_gates)
{
  struct nm_cmdline_launcher_status_s*status = _status;
  int self = nm_cmdline_launcher_get_rank(status);
  int peer = 1 - self;
  _gates[self] = NULL;
  _gates[peer] = status->gate;
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

void nm_cmdline_launcher_init(void*_status, int *argc, char **argv, const char*_label)
{
  struct nm_cmdline_launcher_status_s*status = _status;
  int i,j, err;
  /* rails */
  char *railstring = NULL;
  int nr_r_urls = 0;
  /* per rail arrays */
  puk_component_t driver_assemblies[RAIL_MAX];
  struct nm_driver_query_param params[RAIL_MAX];
  struct nm_driver_query_param *params_array[RAIL_MAX];
  int nparam_array[RAIL_MAX];
  char *r_url[RAIL_MAX];
  nm_drv_id_t drv_id[RAIL_MAX];

  for(i=0; i<RAIL_MAX; i++) {
    params[i].key = NM_DRIVER_QUERY_BY_NOTHING;
    params_array[i] = &params[i];
    nparam_array[i] = 1;
  }

  nm_so_debug_init(argc, argv, PM2DEBUG_DO_OPT|PM2DEBUG_CLEAROPT);
  nm_sr_debug_init(argc, argv, PM2DEBUG_DO_OPT|PM2DEBUG_CLEAROPT);

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

  status->is_server = (!nr_r_urls);

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

  err = nm_core_driver_load_init_some_with_params(status->p_core, status->nr_rails, driver_assemblies, params_array, nparam_array, drv_id, status->l_url);
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

  if (status->is_server) {
    /* server  */
    for(j = 0; j < status->nr_rails; j++) {
      err = nm_core_gate_accept(status->p_core, status->gate, drv_id[j], NULL);
      if (err != NM_ESUCCESS) {
	fprintf(stderr, "launcher: nm_core_gate_accept(drv#%d) returned err = %d\n", j, err);
	goto out_err;
      }
    }
  } else {
    /* client */
    for(j = 0; j < status->nr_rails; j++) {
      err = nm_core_gate_connect(status->p_core, status->gate, drv_id[j], r_url[j]);
      if (err != NM_ESUCCESS) {
	fprintf(stderr, "launcher: nm_core_gate_connect(drv#%d) returned err = %d\n", j, err);
	goto out_err;
      }
    }

  }

  return;

 out_err:
  exit(EXIT_FAILURE);
}


