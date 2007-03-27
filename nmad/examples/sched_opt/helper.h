#include <tbx.h>

#include <nm_public.h>
#include <nm_so_public.h>

#include <nm_so_sendrecv_interface.h>
#include <nm_so_pack_interface.h>

#include <nm_drivers.h>

#ifdef CONFIG_PROTO_MAD3

#include <madeleine.h>
#include <nm_mad3_private.h>

static p_mad_madeleine_t       madeleine	= NULL;
static int                     is_server	= -1;
static struct nm_so_interface *sr_if;
static nm_so_pack_interface    pack_if;
static uint8_t	               gate_id    	=    0;
static p_mad_session_t         session          = NULL;

/*
 * Returns process rank
 */
int get_rank() {
  return session->process_rank;
}

/*
 * Returns the number of nodes
 */
int get_size() {
  return tbx_slist_get_length(madeleine->dir->process_slist);
}

/*
 * Returns the gate id of the process dest
 */
int get_gate_out_id(int dest) {
  p_mad_channel_t channel = tbx_htable_get(madeleine->channel_htable, "pm2");
  p_mad_connection_t connection = tbx_darray_get(channel->out_connection_darray, dest);
  p_mad_nmad_connection_specific_t cs = connection->specific;
  return cs->gate_id;
}

int get_gate_in_id(int dest) {
  p_mad_channel_t channel = tbx_htable_get(madeleine->channel_htable, "pm2");
  p_mad_connection_t connection = tbx_darray_get(channel->in_connection_darray, dest);
  p_mad_nmad_connection_specific_t cs = connection->specific;
  return cs->gate_id;
}

/* initialize everything
 *
 * returns 1 if server, 0 if client
 */
void
init(int	  argc,
     char	**argv) {
  struct nm_core         *p_core     = NULL;

  /*
   * Initialization of various libraries.
   * Reference to the Madeleine object.
   */
  madeleine    = mad_init(&argc, argv);

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

/* clean session
 *
 * returns NM_ESUCCESS or EXIT_FAILURE
 */
int nmad_exit() {
  mad_exit(madeleine);
  return NM_ESUCCESS;
}

#else /* ! CONFIG_PROTO_MAD3 */

static
void
usage(void) {
        fprintf(stderr, "usage: <prog> [[-h <remote_hostname>] <remote url>]\n");
        exit(EXIT_FAILURE);
}

typedef int (*nm_driver_load)(struct nm_drv_ops*);

#if defined CONFIG_IBVERBS
const static nm_driver_load p_driver_load = &nm_ibverbs_load;
#elif defined CONFIG_MX
const static nm_driver_load p_driver_load = &nm_mx_load;
#elif defined CONFIG_GM
const static nm_driver_load p_driver_load = &nm_gm_load;
#elif defined CONFIG_QSNET
const static nm_driver_load p_driver_load = &nm_qsnet_load;
#elif defined CONFIG_TCP
const static nm_driver_load p_driver_load = &nm_tcp_load;
#else
const static nm_driver_load p_driver_load = &nm_tcpdg_load;
#endif

static struct nm_core		*p_core		= NULL;
static struct nm_so_interface	*sr_if	= NULL;
static nm_so_pack_interface	pack_if;

static char	*hostname	= "localhost";
static char	*r_url	= NULL;
static char	*l_url	= NULL;
static uint8_t	 drv_id		=    0;
static uint8_t	 gate_id	=    0;
static int	 is_server;

/*
 * Returns process rank
 */
int get_rank() {
  fprintf(stderr, "not implemented\n");
  exit(EXIT_FAILURE);
}

/*
 * Returns the number of nodes
 */
int get_size() {
  fprintf(stderr, "not implemented\n");
  exit(EXIT_FAILURE);
}

/*
 * Returns the gate id of the process dest
 */
int get_gate_in_id(int dest) {
  fprintf(stderr, "not implemented\n");
  exit(EXIT_FAILURE);
}

/*
 * Returns the gate id of the process dest
 */
int get_gate_out_id(int dest) {
  fprintf(stderr, "not implemented\n");
  exit(EXIT_FAILURE);
}

/* initialize everything
 *
 * returns 1 if server, 0 if client
 */
void
init(int	  argc,
     char	**argv) {
        int err;

        err = nm_core_init(&argc, argv, &p_core, nm_so_load);
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

        argc--;
        argv++;

        if (argc) {
                /* client */
                while (argc) {
                        if (!strcmp(*argv, "-h")) {
                                argc--;
                                argv++;

                                if (!argc)
                                        usage();

                                hostname = *argv;
                        } else {
                                r_url	= *argv;
                        }

                        argc--;
                        argv++;
                }

                printf("running as client using remote url: %s[%s]\n", hostname, r_url);
        } else {
                /* no args: server */
                printf("running as server\n");
        }

        err = nm_core_driver_load_init(p_core, p_driver_load, &drv_id, &l_url);
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
                err = nm_core_gate_connect(p_core, gate_id, drv_id,
                                           hostname, r_url);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect returned err = %d\n", err);
                        goto out_err;
                }
        } else {
                printf("local url: \"%s\"\n", l_url);

                err = nm_core_gate_accept(p_core, gate_id, drv_id, NULL, NULL);
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
