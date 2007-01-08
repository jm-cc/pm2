/* basic_ping.c */
#define MARCEL_INTERNAL_INCLUDE

#define __PROF_APP__
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <semaphore.h>
#include "pm2_common.h"


//#define DEBUG 1
#ifdef MARCEL // threads bourrins uniquement avec Marcel
#define NB_BOURRIN 5
#else
#define NB_BOURRIN 1
#endif
#define PRINT_STATS 0

#define BURNER 1
#define nb_burners 5
#define BURNING_TIME 1000000

#define POLLING_LOOPS 1000000
#define SYSCALL_LOOPS 100

#ifdef MARCEL
static marcel_sem_t ready_sem;
static marcel_sem_t *burner_sem, *burner_ready;
static marcel_sem_t bourrin_ready;
#endif 

static int ok;
static volatile int fin[NB_BOURRIN];
static int pos, fini;

void bourrin()
{
	int my_pos=pos++;
	ok++;
#ifdef DEBUG
	fprintf(stderr, "##Demarrage du bourrin\n");
#endif
	while(!fini);
}

/* Burne pendant 100 ms */
void *burn(void * arg)
{
	
	/* TODO : i=(int*)arg + sems */
	tbx_tick_t t1, t2;
	long long int duree;
	int num_burner=(intptr_t)arg;
#ifdef DEBUG
	fprintf(stderr, "Demarrage du burner %d\n",num_burner);
#endif 

	do{
		marcel_sem_P(&burner_sem[num_burner]);
		TBX_GET_TICK(t1);
		PROF_EVENT1(debut_du_burner, num_burner);
		do {
			TBX_GET_TICK(t2);
			duree=TBX_TICK_RAW_DIFF(t1, t2) ;
		}while(tbx_tick2usec(duree) < BURNING_TIME);
		PROF_EVENT1(fin_du_burner, num_burner);		
		marcel_sem_V(&burner_ready[num_burner]);
	}while(!fini);
}

void wait_for_burners() {
	int i;

	int nvp=marcel_nbvps();
	/* Reveille tous les burners */
#ifdef DEBUG
	fprintf(stderr, "Waking up %d burners\n",nvp);
#endif
	for(i=0;i<nvp;i++)
		marcel_sem_V(&burner_sem[i]);
	/* Attend la fin de tous les burners */
	for(i=0;i<nvp;i++)
		marcel_sem_P(&burner_ready[i]);
#ifdef DEBUG
	fprintf(stderr, "%d burners done\n",nvp);
#endif
}


/*********************************************/
/* création d'un serveur XPaul pour le test  */
/*********************************************/
typedef struct xpaul_io_server {
	struct xpaul_server server;
} *xpaul_io_serverid_t;

typedef struct xpaul_tcp_ev {
	struct xpaul_req inst;
	int ret_val;
} *xpaul_ev_t;

static struct xpaul_io_server xpaul_io_server = {
	.server =
	XPAUL_SERVER_INIT(xpaul_io_server.server, "Testing Server"),

};

static tbx_tick_t t1, t2, t3;
/* XPAUL_FUNCTYPE_BLOCK_WAITONE */
static int xpaul_test_block_waitone(xpaul_server_t server,
			  xpaul_op_t _op,
			  xpaul_req_t req, int nb_ev, int option)
{
	TBX_GET_TICK(t2);
	PROF_EVENT(test_waitone);
	xpaul_req_success(req);
}

/* XPAUL_FUNCTYPE_POLL_POLLONE */
static int xpaul_test_pollone(xpaul_server_t server,
			  xpaul_op_t _op,
			  xpaul_req_t req, int nb_ev, int option)
{
	xpaul_req_success(req);
}


void xpaul_test_init(void)
{
	LOG_IN();
#ifdef MA__LWPS
	xpaul_server_start_lwp(&xpaul_io_server.server, 1);
#endif /* MA__LWPS */
	xpaul_io_server.server.stopable=1;
	xpaul_server_set_poll_settings(&xpaul_io_server.server,
				       XPAUL_POLL_AT_TIMER_SIG
				       | XPAUL_POLL_AT_IDLE, 1, -1);

#ifdef MA__LWPS
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_BLOCK_WAITONE,
				  (xpaul_pcallback_t) {
				  .func = &xpaul_test_block_waitone,.speed =
				  XPAUL_CALLBACK_SLOWEST});
#endif
	xpaul_server_add_callback(&xpaul_io_server.server,
				  XPAUL_FUNCTYPE_POLL_POLLONE,
				  (xpaul_pcallback_t) {
				  .func = &xpaul_test_pollone,.speed =
				  XPAUL_CALLBACK_SLOWEST});

	xpaul_server_start(&xpaul_io_server.server);
	LOG_OUT();
}

int xpaul_test_read()
{
	struct xpaul_tcp_ev ev;
	struct xpaul_wait wait;
	ev.inst.func_to_use=XPAUL_FUNC_POLLING; /* peut être supprimé */
	xpaul_wait(&xpaul_io_server.server, &ev.inst, &wait, 0);
	return 0;
}

int xpaul_test_read_sys()
{
	struct xpaul_tcp_ev ev;
	struct xpaul_wait wait;
	ev.inst.func_to_use=XPAUL_FUNC_SYSCALL;
	xpaul_wait(&xpaul_io_server.server, &ev.inst, &wait, 0);
	return 0;
}

int main(int argc, char ** argv)
{
	common_init(&argc, argv, NULL);

	int i,j;
	int yes = 1;
	fini=0;

#ifdef MARCEL
	marcel_attr_t attr_main;
	marcel_attr_init(&attr_main);
	marcel_attr_setname(&attr_main, "client-server");

	marcel_sem_init(&ready_sem,0);
	marcel_sem_init(&bourrin_ready,0);
	struct marcel_sched_param par;
	par.__sched_priority= MA_DEF_PRIO;
	marcel_attr_setschedpolicy(&attr_main, MARCEL_SCHED_SHARED);
	marcel_sched_setscheduler(marcel_self(),MARCEL_SCHED_SHARED, &par);

	struct marcel_sched_param main_param;
	main_param.__sched_priority=MA_MAX_RT_PRIO;
	marcel_sched_setparam(marcel_self(), &main_param);

	burner_sem=(marcel_sem_t*) malloc(sizeof(marcel_sem_t)*marcel_nbvps());
	burner_ready=(marcel_sem_t*) malloc(sizeof(marcel_sem_t)*marcel_nbvps());
	marcel_t *burners=(marcel_t*) malloc(sizeof(marcel_t)*marcel_nbvps());
	marcel_attr_t attr;
	marcel_attr_setname(&attr, "burner");
	for(i =0;i<marcel_nbvps();i++)
	{
		marcel_sem_init(&burner_sem[i], 0);
		marcel_sem_init(&burner_ready[i], 0);
		marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(i));		
		marcel_create(&burners[i], &attr, burn, (void*)(intptr_t)i);
	}

#endif /* MARCEL */

	long long int duree, duree2;

	/* Initialisation du server de test */
#ifdef DEBUG
	fprintf(stderr, "Starting Test XPaulette Server\n");
#endif
	xpaul_test_init();
#ifdef DEBUG
	fprintf(stderr, "Test XPaulette Server started\n");
#endif
	wait_for_burners();
#ifdef DO_PROFILE
	profile_activate(FUT_ENABLE,USER_APP_MASK|MARCEL_PROF_MASK|MAD_PROF_MASK|XPAUL_PROF_MASK, 0);	  
#endif
	/* Test du polling */
#ifdef DEBUG
	fprintf(stderr, "Testing polling functions\n");
#endif
	TBX_GET_TICK(t1);
	for(i=0;i<POLLING_LOOPS;i++)
			xpaul_test_read();
	TBX_GET_TICK(t2);
	fprintf(stderr, "Polling : %f us\n", tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/(POLLING_LOOPS));

#ifdef MA__LWPS
/* Test de l'export de syscall */
#ifdef DEBUG
	fprintf(stderr, "Testing syscall functions\n");
#endif
	duree=0;
	duree2=0;
	for(i=0;i<SYSCALL_LOOPS;i++){
		PROF_EVENT1(BOUCLE, i);
		TBX_GET_TICK(t1);
		xpaul_test_read_sys();
		TBX_GET_TICK(t3);
		duree+=TBX_TICK_RAW_DIFF(t1, t2);
		duree2+=TBX_TICK_RAW_DIFF(t2, t3);		
		if(!(i%1000)){
			wait_for_burners();
		}
	}

	fprintf(stderr, "Syscall : %f us / %f us\n", tbx_tick2usec(duree)/(SYSCALL_LOOPS),tbx_tick2usec(duree2)/(SYSCALL_LOOPS));
#endif /* MA__LWPS */

	
#ifdef DO_PROFILE
	profile_stop();
#endif
	fini=1;        
	wait_for_burners();
	common_exit(NULL);
	return 0;
}
