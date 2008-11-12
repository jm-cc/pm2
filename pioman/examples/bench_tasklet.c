#define MARCEL_INTERNAL_INCLUDE

#define __PROF_APP__
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <semaphore.h>
#include "pm2_common.h"

/* This benchmark evaluate the cost of invoking a polling tasklet */

#define SYSCALL_LOOPS 100
#define INNER_LOOPS 1000
#define POLLING_LOOPS (10000000/INNER_LOOPS)

piom_cond_t cond;

typedef struct piom_io_server {
	struct piom_server server;
} *piom_io_serverid_t;

typedef struct piom_tcp_ev {
	struct piom_req inst;
	int ret_val;
} *piom_ev_t;

static struct piom_io_server piom_io_server = {
	.server =
	PIOM_SERVER_INIT(piom_io_server.server, "Testing Server"),

};

static tbx_tick_t t1, t2, t3;

/* PIOM_FUNCTYPE_POLL_POLLONE */
static int piom_test_pollone(piom_server_t server,
			  piom_op_t _op,
			  piom_req_t req, int nb_ev, int option)
{
	static int inner_loops=0;
	inner_loops++;
	if(inner_loops>=INNER_LOOPS) {
		piom_cond_signal(&cond, 1);
		inner_loops=0;
	}
}


void piom_test_init(void)
{
	LOG_IN();
#ifdef MA__LWPS
	piom_server_start_lwp(&piom_io_server.server, 1);
#endif /* MA__LWPS */
	piom_io_server.server.stopable=1;
	piom_server_set_poll_settings(&piom_io_server.server,
				      PIOM_POLL_AT_TIMER_SIG
				      | PIOM_POLL_AT_IDLE, 1, -1);

	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_POLL_POLLONE,
				  (piom_pcallback_t) {
				  .func = &piom_test_pollone,.speed =
				  PIOM_CALLBACK_SLOWEST});
	piom_cond_init(&cond, 0);
	piom_server_start(&piom_io_server.server);
	LOG_OUT();
}

int piom_test_read()
{
	struct piom_wait wait;
	piom_cond_init(&cond, 2);
	piom_cond_wait(&cond, 1);
	return 0;
}

int piom_test_read_sys()
{
	struct piom_wait wait;
	piom_cond_mask(&cond, 2);
	piom_cond_wait(&cond, 1);
	return 0;
}

int main(int argc, char ** argv)
{

	common_init(&argc, argv, NULL);

	int i,j;
	int yes = 1;

	long long int duree, duree2;

	piom_test_init();
	
	struct piom_tcp_ev ev;	
	piom_req_init(&ev.inst);
	ev.inst.func_to_use=PIOM_FUNC_POLLING;
	piom_req_submit(&piom_io_server.server, &ev.inst);


	for(i=0;i<1000;i++)
	{
		piom_test_read();
	}
	TBX_GET_TICK(t1);
	for(i=0;i<POLLING_LOOPS;i++)
	{
		piom_test_read();
	}
	TBX_GET_TICK(t2);
	fprintf(stderr, "Polling : %f us\n", tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/(POLLING_LOOPS*INNER_LOOPS));
	common_exit(NULL);
	return 0;
}
