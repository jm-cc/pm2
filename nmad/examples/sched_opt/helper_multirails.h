#include <string.h>

#include <tbx.h>

#include <nm_public.h>
#include <nm_so_public.h>
#include "nm_so_debug.h"
#include <nm_so_sendrecv_interface.h>
#include <nm_so_pack_interface.h>

#include <nm_drivers.h>

#define RAIL_MAX 8
#define RAIL_NR_DEFAULT 2

static int                       is_server = -1;
static struct nm_core		*p_core	= NULL;
static struct nm_so_interface	*sr_if = NULL;
static nm_so_pack_interface	pack_if;
static nm_gate_id_t	        gate_id	=    0;

#ifdef CONFIG_PROTO_MAD3

void
init(int	 *argc,
     char	**argv) {
  fprintf(stderr, "Multi-rail is not compatible with the protocol mad3\n");
}

int nmad_exit() {
  exit(1);
}

#else

static
void
usage(void) {
  fprintf(stderr, "usage: <prog> [-R <rail1+rail2...>] [<remote url> ...]\n");
  fprintf(stderr, "  There must be at least 2 rails, and as many rails followed by the server's URLs on the sender's side\n");
  fprintf(stderr, "  Rails may be:\n");
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

/* get default drivers if no rails were given on the command line
 */
static int
get_default_driver_loads(nm_driver_load *loads)
{
  int cur_nr_drivers = 0;

#if defined CONFIG_MX
  printf("Using MX for rail #%d\n", cur_nr_drivers);
  loads[cur_nr_drivers++] = &nm_mx_load;
#endif
#ifdef CONFIG_QSNET
  printf("Using QsNet for rail #%d\n", cur_nr_drivers);
  loads[cur_nr_drivers++] = &nm_qsnet_load;
#endif
#if defined CONFIG_IBVERBS
  printf("Using IBVerbs for rail #%d\n", cur_nr_drivers);
  loads[cur_nr_drivers++] = &nm_ibverbs_load;
#endif
#if defined CONFIG_GM
  printf("Using GM for rail #%d\n", cur_nr_drivers);
  loads[cur_nr_drivers++] = &nm_gm_load;
#endif
  printf("Using TCPDG for rail #%d\n", cur_nr_drivers);
  loads[cur_nr_drivers++] = &nm_tcpdg_load;

  /* FIXME: be sure we don't add more than RAIL_MAX if we add more drivers one day */
  if (cur_nr_drivers < RAIL_NR_DEFAULT) {
    fprintf(stderr, "Found %d rail(s), need at least %d by default\n", cur_nr_drivers, RAIL_NR_DEFAULT);
    exit(EXIT_FAILURE);
  }

  return RAIL_NR_DEFAULT;
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
static int
get_railstring_driver_loads(nm_driver_load *loads,
			    struct nm_driver_query_param *params,
			    char * railstring)
{
  int cur_nr_drivers = 0;
  char * token;
  int err;

  token = strtok(railstring, "+");
  while (token) {
    err = handle_one_rail(token, cur_nr_drivers,
			  &loads[cur_nr_drivers],
			  &params[cur_nr_drivers]);
    if (err < 0)
      usage();
    cur_nr_drivers++;
    token = strtok(NULL, "+");
  }

  return cur_nr_drivers;
}

/* initialize everything
 *
 * returns 1 if server, 0 if client
 */
void
init(int	 *argc,
     char	**argv)
{
  static int i,j, err;
  /* rails */
  char *railstring = NULL;
  int nr_rails = 0;
  int nr_r_urls = 0;
  /* per rail arrays */
  nm_driver_load driver_loads[RAIL_MAX];
  struct nm_driver_query_param params[RAIL_MAX];
  struct nm_driver_query_param *params_array[RAIL_MAX];
  int nparam_array[RAIL_MAX];
  char *r_url[RAIL_MAX];
  char *l_url[RAIL_MAX];
  uint8_t drv_id[RAIL_MAX];

  for(i=0; i<RAIL_MAX; i++) {
    params[i].key = NM_DRIVER_QUERY_BY_NOTHING;
    params_array[i] = &params[i];
    nparam_array[i] = 1;
  }

  nm_so_debug_init(argc, argv, PM2DEBUG_DO_OPT|PM2DEBUG_CLEAROPT);
  nm_so_sr_debug_init(argc, argv, PM2DEBUG_DO_OPT|PM2DEBUG_CLEAROPT);

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
    nr_rails = get_railstring_driver_loads(driver_loads, params, railstring);
  } else {
    /* use default drivers */
    nr_rails = get_default_driver_loads(driver_loads);
  }

  if (nr_rails < 2) { /* FIXME: won't be necessary once the strategy handles this */
    fprintf(stderr, "Need at least 2 rails\n");
    usage();
  }

  is_server = (!nr_r_urls);

  /* if client, we need as many url as drivers */
  if (!is_server && nr_r_urls < nr_rails) {
    fprintf(stderr, "Need %d url for these %d rails\n", nr_r_urls, nr_rails);
    usage();
  }

  if (is_server) {
    printf("running as server\n");
  } else {
    printf("running as client using remote url:");
    for(j=0; j<nr_rails; j++)
      printf(" %s", r_url[j]);
    printf("\n");
  }

  err = nm_core_driver_load_init_some_with_params(p_core, nr_rails, driver_loads, params_array, nparam_array, drv_id, l_url);
  if (err != NM_ESUCCESS) {
    printf("nm_core_driver_load_init_some returned err = %d\n", err);
    goto out_err;
  }
  for(j=0; j<nr_rails; j++) {
    printf("local url[%d]: [%s]\n", j, l_url[j]);
  }

  nm_ns_init(p_core);

  err = nm_core_gate_init(p_core, &gate_id);
  if (err != NM_ESUCCESS) {
    printf("nm_core_gate_init returned err = %d\n", err);
    goto out_err;
  }

  if (is_server) {
    /* server  */
    for(j=0; j<nr_rails; j++) {
      err = nm_core_gate_accept(p_core, gate_id, drv_id[j], NULL);
      if (err != NM_ESUCCESS) {
	printf("nm_core_gate_accept(drv#%d) returned err = %d\n", j, err);
	goto out_err;
      }
    }

  } else {
    /* client */
    for(j=0; j<nr_rails; j++) {
      err = nm_core_gate_connect(p_core, gate_id, drv_id[j], r_url[j]);
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

/* clean session
 *
 * returns NM_ESUCCESS or EXIT_FAILURE
 */
int nmad_exit() {
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

