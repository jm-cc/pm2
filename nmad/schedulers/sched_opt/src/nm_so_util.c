#include <string.h>

#ifdef CONFIG_PROTO_MAD3
#include <pm2_common.h>
#include <madeleine.h>
#include <nm_mad3_private.h>
#endif

#include <tbx.h>
#include <nm_public.h>
#include <nm_so_public.h>
#include <nm_so_debug.h>
#include <nm_so_sendrecv_interface.h>
#include <nm_so_pack_interface.h>
#include <nm_drivers.h>
#include <Padico/Puk.h>

#ifdef CONFIG_MULTI_RAIL
#define RAIL_MAX 8
#else
#define RAIL_MAX 1
#endif

static inline puk_component_t load_driver(const char *driver_name)
{
  return nm_core_component_load("driver", driver_name); 
}

#ifdef CONFIG_PROTO_MAD3

static p_mad_madeleine_t       madeleine	= NULL;
static p_mad_session_t         session          = NULL;

int nm_so_get_rank(int *rank) {
  *rank = session->process_rank;
  return NM_ESUCCESS;
}

int nm_so_get_size(int *size) {
  *size = tbx_slist_get_length(madeleine->dir->process_slist);
  return NM_ESUCCESS;
}

int nm_so_get_sr_if(struct nm_so_interface **sr_if) {
  *sr_if = mad_nmad_get_sr_interface();
  return NM_ESUCCESS;
}

int nm_so_get_pack_if(nm_so_pack_interface *pack_if) {
  struct nm_so_interface *sr_if = mad_nmad_get_sr_interface();
  *pack_if = (nm_so_pack_interface)sr_if;
  return NM_ESUCCESS;
}

int nm_so_get_gate_out_id(int dest, nm_gate_id_t *gate_id) {
  p_mad_channel_t channel = tbx_htable_get(madeleine->channel_htable, "pm2");
  p_mad_connection_t connection = tbx_darray_get(channel->out_connection_darray, dest);
  p_mad_nmad_connection_specific_t cs = connection->specific;
  *gate_id = cs->gate_id;
  return NM_ESUCCESS;
}

int nm_so_get_gate_in_id(int dest, nm_gate_id_t *gate_id) {
  p_mad_channel_t channel = tbx_htable_get(madeleine->channel_htable, "pm2");
  p_mad_connection_t connection = tbx_darray_get(channel->in_connection_darray, dest);
  p_mad_nmad_connection_specific_t cs = connection->specific;
  *gate_id = cs->gate_id;
  return NM_ESUCCESS;
}

void nm_so_init(int *argc, char	**argv) {
  struct nm_core         *p_core     = NULL;

  /*
   * Initialization of various libraries.
   * Reference to the Madeleine object.
   */
  madeleine    = mad_init(argc, argv);

  /*
   * Reference to the session information object
   */
  session      = madeleine->session;

  /*
   * Reference to the NewMadeleine core object
   */
  p_core = mad_nmad_get_core();

}

int nm_so_exit(void) {
  mad_exit(madeleine);
  return NM_ESUCCESS;
}

#else /* ! CONFIG_PROTO_MAD3 */

static void
usage(void) {
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

static struct nm_core	      *_p_core	        = NULL;
static struct nm_so_interface *_sr_if           = NULL;
static nm_so_pack_interface    _pack_if;
static int                     _is_server       = -1;
static nm_gate_id_t	       _gate_id	        = 0;

int nm_so_get_rank(int *rank) {
  if (!_is_server) *rank = 0; else *rank = 1;
  return NM_ESUCCESS;
}

int nm_so_get_size(int *size) {
  *size = 2;
  return NM_ESUCCESS;
}

int nm_so_get_sr_if(struct nm_so_interface **sr_if) {
  *sr_if = _sr_if;
  return NM_ESUCCESS;
}

int nm_so_get_pack_if(nm_so_pack_interface *pack_if) {
  *pack_if = _pack_if;
  return NM_ESUCCESS;
}

int nm_so_get_gate_in_id(int dest, nm_gate_id_t *gate_id) {
  *gate_id = _gate_id;
  return NM_ESUCCESS;
}

int nm_so_get_gate_out_id(int dest, nm_gate_id_t *gate_id) {
  *gate_id = _gate_id;
  return NM_ESUCCESS;
}

/** get default drivers if no rails were given on the command line */
static int
get_default_driver_assemblies(puk_component_t*assemblies)
{
  int cur_nr_drivers = 0;

#ifdef CONFIG_MULTI_RAIL
  /* load one rail for each driver */

#if defined CONFIG_MX
  printf("Using MX for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("mx");
#endif
#ifdef CONFIG_QSNET
  printf("Using QsNet for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("qsnet");
#endif
#if defined CONFIG_IBVERBS
  printf("Using IBVerbs for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("ibverbs");
#endif
#if defined CONFIG_GM
  printf("Using GM for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("gm");
#endif
#if defined CONFIG_TCP
  printf("Using TCP for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("tcp");
#endif

#else /* !CONFIG_MULTI_RAIL */
  /* load one rail with the first driver only */

  if (NR_ENABLED_DRIVERS > 1) {
    fprintf(stderr, "Too many drivers enabled (%d), please specify a single one with -R\n", NR_ENABLED_DRIVERS);
    return -1;
  } else if (!NR_ENABLED_DRIVERS) {
    fprintf(stderr, "No drivers are enabled\n");
    return -1;
  }

#if defined CONFIG_CUSTOM
  printf("Using CUSTOM for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("custom");
#elif defined CONFIG_MX
  printf("Using MX for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("mx");
#elif defined CONFIG_QSNET
  printf("Using QsNet for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("qsnet");
#elif defined CONFIG_IBVERBS
  printf("Using IBVerbs for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("ibverbs");
#elif defined CONFIG_GM
  printf("Using GM for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("gm");
#elif defined CONFIG_TCP
  printf("Using TCP for rail #%d\n", cur_nr_drivers);
  assemblies[cur_nr_drivers++] = load_driver("tcp");
#endif

#endif /* !CONFIG_MULTI_RAIL */

  return cur_nr_drivers;
}

#if defined(CONFIG_MX) || defined(CONFIG_IBVERBS)
/** look for a driver string in a rail and check whether the board id is forced  */
static int
lookup_rail_driver_and_board_id(const char *rail, const char *driver,
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
static int
handle_one_rail(char *token, int index,
		puk_component_t*assembly,
		struct nm_driver_query_param *param)
{
#if defined CONFIG_MX
  if (lookup_rail_driver_and_board_id(token, "mx", param)) {
    if (param->key == NM_DRIVER_QUERY_BY_INDEX)
      printf("Using MX board #%d for rail #%d\n",
	     param->value.index, index);
    else
      printf("Using any MX board for rail #%d\n",
	     index);
    *assembly = load_driver("mx");
  } else
#endif
#ifdef CONFIG_QSNET
  if (!strcmp("qsnet", token)) {
    printf("Using QsNet for rail #%d\n", index);
    *assembly = load_driver("qsnet");
  } else
#endif
#if defined CONFIG_IBVERBS
  if (lookup_rail_driver_and_board_id(token, "ib", param)
      || lookup_rail_driver_and_board_id(token, "ibv", param)
      || lookup_rail_driver_and_board_id(token, "ibverbs", param)) {
    if (param->key == NM_DRIVER_QUERY_BY_INDEX)
      printf("Using IB device #%d for rail #%d\n",
	     param->value.index, index);
    else
      printf("Using any IB device for rail #%d\n",
	     index);
    *assembly = load_driver("ibverbs");
  } else
#endif
#if defined CONFIG_GM
  if (!strcmp("gm", token)) {
    printf("Using GM for rail #%d\n", index);
    *assembly = load_driver("gm");
  } else
#endif
#if defined CONFIG_TCP
  if (!strcmp("tcp", token)) {
    printf("Using TCP for rail #%d\n", index);
    *assembly = load_driver("tcp");
  } else 
#endif
    {
      fprintf(stderr, "Unrecognized rail \"%s\"\n", token);
      return -1;
    }

  return 0;
}

/** get drivers from the railstring from the command line */
static int
get_railstring_driver_assemblies(puk_component_t*assemblies,
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
	fprintf(stderr, "Found too many drivers, only %d are supported\n", RAIL_MAX);
	usage();
    }

    token = strtok(NULL, "+");
  }

  return cur_nr_drivers;
}

static char *l_url[RAIL_MAX];
static int nr_rails = 0;

void nm_so_init(int *argc, char	**argv) {
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
  uint8_t drv_id[RAIL_MAX];

  for(i=0; i<RAIL_MAX; i++) {
    params[i].key = NM_DRIVER_QUERY_BY_NOTHING;
    params_array[i] = &params[i];
    nparam_array[i] = 1;
  }

  nm_so_debug_init(argc, argv, PM2DEBUG_DO_OPT|PM2DEBUG_CLEAROPT);
  nm_so_sr_debug_init(argc, argv, PM2DEBUG_DO_OPT|PM2DEBUG_CLEAROPT);

  err = nm_core_init(argc, argv, &_p_core, nm_so_load);
  if (err != NM_ESUCCESS) {
    printf("nm_core_init returned err = %d\n", err);
    goto out_err;
  }

  err = nm_so_sr_init(_p_core, &_sr_if);
  if(err != NM_ESUCCESS) {
    printf("nm_so_sr_init return err = %d\n", err);
    goto out_err;
  }

  _pack_if = (nm_so_pack_interface)_sr_if;

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
	fprintf(stderr, "Found too many url, only %d are supported\n", RAIL_MAX);
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

  if (railstring) {
    /* parse railstring to get drivers */
    nr_rails = get_railstring_driver_assemblies(driver_assemblies, params, railstring);
  } else {
    /* use default drivers */
    nr_rails = get_default_driver_assemblies(driver_assemblies);
    if (nr_rails < 0) {
      fprintf(stderr, "Failed to select default drivers automatically\n");
      usage();
    }
  }

#ifndef CONFIG_MULTI_RAIL
  // Check the number of protocols against the strategy
  if (nr_rails > 1) {
    TBX_FAILURE("Only 1 driver is supported with the current strategy.");
  }
#endif

  _is_server = (!nr_r_urls);

  /* if client, we need as many url as drivers */
  if (!_is_server && nr_r_urls < nr_rails) {
    fprintf(stderr, "Need %d url for these %d rails\n", nr_r_urls, nr_rails);
    usage();
  }

  if (_is_server) {
    printf("running as server\n");
  } else {
    printf("running as client using remote url:");
    for(j=0; j<nr_rails; j++)
      printf(" %s", r_url[j]);
    printf("\n");
  }

  err = nm_core_driver_load_init_some_with_params(_p_core, nr_rails, driver_assemblies, params_array, nparam_array, drv_id, l_url);
  if (err != NM_ESUCCESS) {
    printf("nm_core_driver_load_init_some returned err = %d\n", err);
    goto out_err;
  }
  for(j=0; j<nr_rails; j++) {
    printf("local url[%d]: [%s]\n", j, l_url[j]);
  }

  nm_ns_init(_p_core);

  err = nm_core_gate_init(_p_core, &_gate_id);
  if (err != NM_ESUCCESS) {
    printf("nm_core_gate_init returned err = %d\n", err);
    goto out_err;
  }

  if (_is_server) {
    /* server  */
    for(j=0; j<nr_rails; j++) {
      err = nm_core_gate_accept(_p_core, _gate_id, drv_id[j], NULL);
      if (err != NM_ESUCCESS) {
	printf("nm_core_gate_accept(drv#%d) returned err = %d\n", j, err);
	goto out_err;
      }
    }
  } else {
    /* client */
    for(j=0; j<nr_rails; j++) {
      err = nm_core_gate_connect(_p_core, _gate_id, drv_id[j], r_url[j]);
      if (err != NM_ESUCCESS) {
	printf("nm_core_gate_connect(drv#%d) returned err = %d\n", j, err);
	goto out_err;
      }
    }

  }

  return;

 out_err:
  exit(EXIT_FAILURE);
}

int nm_so_exit(void) {
  int j, err, ret = NM_ESUCCESS;

  for(j=0; j<nr_rails; j++) {
    free(l_url[j]);
  }

  err = nm_ns_exit(_p_core);
  if(err != NM_ESUCCESS) {
    printf("nm_ns_exit return err = %d\n", err);
    ret = EXIT_FAILURE;
  }
  err = nm_core_driver_exit(_p_core);
  if(err != NM_ESUCCESS) {
    printf("nm_core_driver_exit return err = %d\n", err);
    ret = EXIT_FAILURE;
  }
  err = nm_core_exit(_p_core);
  if(err != NM_ESUCCESS) {
    printf("nm_core__exit return err = %d\n", err);
    ret = EXIT_FAILURE;
  }
  err = nm_so_sr_exit(_sr_if);
  if(err != NM_ESUCCESS) {
    printf("nm_so_sr_exit return err = %d\n", err);
    ret = EXIT_FAILURE;
  }

  common_exit(NULL);

  return ret;
}

#endif /* CONFIG_PROTO_MAD3 */

