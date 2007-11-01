#include <tbx.h>

#include <nm_public.h>
#include <nm_so_public.h>

#include <nm_so_sendrecv_interface.h>
#include <nm_so_pack_interface.h>

#include <nm_drivers.h>

/** Returns process rank */
int get_rank(void);

/** Returns the number of nodes */
int get_size(void);

/** Returns the out gate id of the process dest */
nm_gate_id_t get_gate_out_id(int dest);

/** Returns the in gate id of the process dest */
nm_gate_id_t get_gate_in_id(int dest);

/** Initializes everything. Returns 1 if server, 0 if client. */
void init(int *argc, char **argv);

/** Cleans session. Returns NM_ESUCCESS or EXIT_FAILURE. */
int nmad_exit(void);

#ifdef CONFIG_PROTO_MAD3

#include <madeleine.h>
#include <nm_mad3_private.h>

static p_mad_madeleine_t       madeleine	= NULL;
static int                     is_server	= -1;
static struct nm_so_interface *sr_if;
static nm_so_pack_interface    pack_if;
static nm_gate_id_t	       gate_id    	=    0;
static p_mad_session_t         session          = NULL;

int get_rank(void) {
  return session->process_rank;
}

int get_size(void) {
  return tbx_slist_get_length(madeleine->dir->process_slist);
}

nm_gate_id_t get_gate_out_id(int dest) {
  p_mad_channel_t channel = tbx_htable_get(madeleine->channel_htable, "pm2");
  p_mad_connection_t connection = tbx_darray_get(channel->out_connection_darray, dest);
  p_mad_nmad_connection_specific_t cs = connection->specific;
  return cs->gate_id;
}

nm_gate_id_t get_gate_in_id(int dest) {
  p_mad_channel_t channel = tbx_htable_get(madeleine->channel_htable, "pm2");
  p_mad_connection_t connection = tbx_darray_get(channel->in_connection_darray, dest);
  p_mad_nmad_connection_specific_t cs = connection->specific;
  return cs->gate_id;
}

void init(int *argc, char **argv) {
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
   * Globally unique process rank.
   */
  is_server = session->process_rank;

  if (!is_server) {
    /* client needs gate_id to connect to the server */
    p_mad_channel_t channel = tbx_htable_get(madeleine->channel_htable, "pm2");
    p_mad_connection_t connection = tbx_darray_get(channel->out_connection_darray, 1);
    p_mad_nmad_connection_specific_t cs = connection->specific;
    gate_id = cs->gate_id;
  }

  /*
   * Reference to the NewMadeleine core object
   */
  p_core = mad_nmad_get_core();
  sr_if = mad_nmad_get_sr_interface();
  pack_if = (nm_so_pack_interface)sr_if;
}

int nmad_exit(void) {
  mad_exit(madeleine);
  return NM_ESUCCESS;
}

#else /* ! CONFIG_PROTO_MAD3 */

static void usage(void) {
  fprintf(stderr, "usage: <prog> [-R <rail>] [<remote url>]\n");
  fprintf(stderr, "  The rail may be:\n");
#if defined CONFIG_MX
  fprintf(stderr, "    mx   to use any MX board\n");
  fprintf(stderr, "    mx:N        the Nth MX board\n");
#endif
#if defined CONFIG_QSNET
  fprintf(stderr, "    qsnet       the QsNet board\n");
#endif
#if defined CONFIG_IBVERBS
  fprintf(stderr, "    ib          any InfiniBand device\n");
  fprintf(stderr, "    ib:N        the Nth InfiniBand device\n");
#endif
#if defined CONFIG_GM
  fprintf(stderr, "    gm          any GM board\n");
#endif
  fprintf(stderr, "    tcp         any TCP connection\n");
  exit(EXIT_FAILURE);
}

typedef int (*nm_driver_load)(struct nm_drv_ops*);

static struct nm_core		*p_core		= NULL;
static struct nm_so_interface	*sr_if	= NULL;
static nm_so_pack_interface	pack_if;

static char	*r_url	= NULL;
static char	*l_url	= NULL;
static uint8_t	 drv_id		=    0;
static nm_gate_id_t gate_id	=    0;
static int	 is_server;

int get_rank(void) {
  fprintf(stderr, "get_rank not implemented. Try using the mad3 protocol.\n");
  exit(EXIT_FAILURE);
}

int get_size(void) {
  fprintf(stderr, "get_size not implemented. Try using the mad3 protocol.\n");
  exit(EXIT_FAILURE);
}

nm_gate_id_t get_gate_in_id(int dest) {
  fprintf(stderr, "get_gate_in_id not implemented. Try using the mad3 protocol.\n");
  exit(EXIT_FAILURE);
}

nm_gate_id_t get_gate_out_id(int dest) {
  fprintf(stderr, "get_gate_out_id not implemented. Try using the mad3 protocol.\n");
  exit(EXIT_FAILURE);
}

/* get default drivers if no rails were given on the command line
 */
static void
get_default_driver_load(nm_driver_load *load)
{
#if defined CONFIG_MX
  printf("Using MX\n");
  *load = &nm_mx_load;
#elif defined CONFIG_QSNET
  printf("Using QsNet\n");
  *load = &nm_qsnet_load;
#elif defined CONFIG_IBVERBS
  printf("Using IBVerbs\n");
  *load = &nm_ibverbs_load;
#elif defined CONFIG_GM
  printf("Using GM\n");
  *load = &nm_gm_load;
#else
  printf("Using TCPDG\n");
  *load = &nm_tcpdg_load;
#endif
}

/* look for a driver string in a rail and check whether the board id is forced
 */
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

/* handle one rail from the railstring
 */
static int
handle_one_rail(char *token, int index,
		nm_driver_load *load,
		struct nm_driver_query_param *param)
{
  int board_id;

#if defined CONFIG_MX
  if (lookup_rail_driver_and_board_id(token, "mx", param)) {
    if (param->key == NM_DRIVER_QUERY_BY_INDEX)
      printf("Using MX board #%d for rail #%d\n",
	     param->value.index, index);
    else
      printf("Using any MX board for rail #%d\n",
	     index);
    *load = &nm_mx_load;
  } else
#endif
#ifdef CONFIG_QSNET
  if (!strcmp("qsnet", token)) {
    printf("Using QsNet for rail #%d\n", index);
    *load = &nm_qsnet_load;
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
    *load = &nm_ibverbs_load;
  } else
#endif
#if defined CONFIG_GM
  if (!strcmp("gm", token)) {
    printf("Using GM for rail #%d\n", index);
    *load = &nm_gm_load;
  } else
#endif
  if (!strcmp("tcp", token) || !strcmp("tcpdg", token)) {
    printf("Using TCPDG for rail #%d\n", index);
    *load = &nm_tcpdg_load;
  } else {
    fprintf(stderr, "Unrecognized rail \"%s\"\n", token);
    return -1;
  }

  return 0;
}

/* get drivers from the railstring from the command line
 */
static void
get_railstring_driver_load(nm_driver_load *load,
			   struct nm_driver_query_param *param,
			   char * railstring)
{
  int err;

  err = handle_one_rail(railstring, 0,
			load, param);
  if (err < 0)
    usage();
}

void
init(int	 *argc,
     char	**argv) {
  int i, j, err;
  char *railstring = NULL;
  nm_driver_load driver_load = NULL;
  struct nm_driver_query_param params[1];

  params[1].key = NM_DRIVER_QUERY_BY_NOTHING;

  err = nm_core_init(argc, argv, &p_core, nm_so_load);
  if (err != NM_ESUCCESS) {
    printf("nm_core_init returned err = %d\n", err);
    goto out_err;
  }

  err = nm_so_sr_init(p_core, &sr_if);
  if(err != NM_ESUCCESS) {
    printf("nm_so_sr_init return err = %d\n", err);
    goto out_err;
  }

  err = nm_so_pack_interface_init(p_core, &pack_if);
  if(err != NM_ESUCCESS) {
    printf("nm_so_pack_interface_init return err = %d\n", err);
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
    if (!strncmp(argv[i], "-", 1)) {
      break;
    }
    /* handle urls */
    else {
      r_url	= argv[i];
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
    get_railstring_driver_load(&driver_load, params, railstring);
  } else {
    /* use default drivers */
    get_default_driver_load(&driver_load);
  }

  if (r_url) {
    printf("running as client using remote url: %s\n", r_url);
  } else {
    printf("running as server\n");
  }

  err = nm_core_driver_load_init_with_params(p_core, driver_load, params, 1, &drv_id, &l_url);
  if (err != NM_ESUCCESS) {
    printf("nm_core_driver_load_init returned err = %d\n", err);
    goto out_err;
  }

  err = nm_core_gate_init(p_core, &gate_id);
  if (err != NM_ESUCCESS) {
    printf("nm_core_gate_init returned err = %d\n", err);
    goto out_err;
  }

  if (r_url) {
    err = nm_core_gate_connect(p_core, gate_id, drv_id, r_url);
    if (err != NM_ESUCCESS) {
      printf("nm_core_gate_connect returned err = %d\n", err);
      goto out_err;
    }
  } else {
    printf("local url: \"%s\"\n", l_url);

    err = nm_core_gate_accept(p_core, gate_id, drv_id, NULL);
    if (err != NM_ESUCCESS) {
      printf("nm_core_gate_accept returned err = %d\n", err);
      goto out_err;
    }
  }

  is_server	= !r_url;
  return;

 out_err:
  exit(EXIT_FAILURE);
}

int nmad_exit(void) {
  int err, ret = NM_ESUCCESS;

  err = nm_core_driver_exit(p_core);
  if(err != NM_ESUCCESS) {
    printf("nm_core_driver_exit return err = %d\n", err);
    ret = EXIT_FAILURE;
  }
  err = nm_core_exit(p_core);
  if(err != NM_ESUCCESS) {
    printf("nm_core__exit return err = %d\n", err);
    ret = EXIT_FAILURE;
  }
  err = nm_so_sr_exit(sr_if);
  if(err != NM_ESUCCESS) {
    printf("nm_so_sr_exit return err = %d\n", err);
    ret = EXIT_FAILURE;
  }
  return ret;
}
#endif /* CONFIG_PROTO_MAD3 */
