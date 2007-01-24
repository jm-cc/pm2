#include <tbx.h>

#include <nm_public.h>
#include <nm_so_public.h>

#include <nm_so_sendrecv_interface.h>
#include <nm_so_pack_interface.h>

#include <nm_drivers.h>

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

        err = nm_core_driver_init(p_core, p_driver_load, &drv_id, &l_url);
        if (err != NM_ESUCCESS) {
                printf("nm_core_driver_init returned err = %d\n", err);
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
