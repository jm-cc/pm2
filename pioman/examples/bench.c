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


#define DEBUG 1
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
#else
piom_cond_t cond;
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
#ifdef MARCEL
		marcel_sem_P(&burner_sem[num_burner]);
#endif
		TBX_GET_TICK(t1);
		PROF_EVENT1(debut_du_burner, num_burner);
		do {
			TBX_GET_TICK(t2);
			duree=TBX_TICK_RAW_DIFF(t1, t2) ;
		}while(tbx_tick2usec(duree) < BURNING_TIME);
		PROF_EVENT1(fin_du_burner, num_burner);		
#ifdef MARCEL
		marcel_sem_V(&burner_ready[num_burner]);
#endif
	}while(!fini);
}

void wait_for_burners() {
//	return;
	int i;
#ifdef MARCEL
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
#endif

}


/*********************************************/
/* création d'un serveur Piom pour le test  */
/*********************************************/
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
/* PIOM_FUNCTYPE_BLOCK_WAITONE */
static int piom_test_block_waitone(piom_server_t server,
			  piom_op_t _op,
			  piom_req_t req, int nb_ev, int option)
{
	TBX_GET_TICK(t2);
	PROF_EVENT(test_waitone);
	piom_req_success(req);
#ifndef MARCEL
	piom_cond_signal(&cond, 1);
#endif
}

/* PIOM_FUNCTYPE_POLL_POLLONE */
static int piom_test_pollone(piom_server_t server,
			  piom_op_t _op,
			  piom_req_t req, int nb_ev, int option)
{
	piom_req_success(req);
#ifndef MARCEL
	piom_cond_signal(&cond, 1);
#endif
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

#ifdef MA__LWPS
	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_BLOCK_WAITONE,
				  (piom_pcallback_t) {
				  .func = &piom_test_block_waitone,.speed =
				  PIOM_CALLBACK_SLOWEST});
#endif
	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_POLL_POLLONE,
				  (piom_pcallback_t) {
				  .func = &piom_test_pollone,.speed =
				  PIOM_CALLBACK_SLOWEST});
#ifndef MARCEL
	piom_cond_init(&cond, 0);
#endif
	piom_server_start(&piom_io_server.server);
	LOG_OUT();
}

int piom_test_read()
{
	struct piom_tcp_ev ev;
	struct piom_wait wait;
#ifndef MARCEL
	piom_cond_mask(&cond, 0);
	piom_req_init(&ev.inst);
#endif
	ev.inst.func_to_use=PIOM_FUNC_POLLING; /* peut être supprimé */
#ifndef MARCEL
	piom_req_submit(&piom_io_server.server, &ev.inst);
	piom_cond_wait(&cond, 1);
#else
	piom_wait(&piom_io_server.server, &ev.inst, &wait, 0);
#endif
	return 0;
}

int piom_test_read_sys()
{
	struct piom_tcp_ev ev;
	struct piom_wait wait;
#ifndef MARCEL
	piom_cond_mask(&cond, 0);
	piom_req_init(&ev.inst);	
#endif
	ev.inst.func_to_use=PIOM_FUNC_SYSCALL;
#ifdef MARCEL
	piom_wait(&piom_io_server.server, &ev.inst, &wait, 0);
#else
	piom_req_submit(&piom_io_server.server, &ev.inst);
	piom_cond_wait(&cond, 1);
#endif
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
	fprintf(stderr, "Starting Test Piomette Server\n");
#endif
	piom_test_init();
#ifdef DEBUG
	fprintf(stderr, "Test Piomette Server started\n");
#endif
	wait_for_burners();
#ifdef DO_PROFILE
	profile_activate(FUT_ENABLE,USER_APP_MASK|MARCEL_PROF_MASK|MAD_PROF_MASK|PIOM_PROF_MASK, 0);	  
#endif
	/* Test du polling */
#ifdef DEBUG
	fprintf(stderr, "Testing polling functions\n");
#endif
	TBX_GET_TICK(t1);
	for(i=0;i<POLLING_LOOPS;i++)
			piom_test_read();
	TBX_GET_TICK(t2);
	fprintf(stderr, "Polling : %f us\n", tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/(POLLING_LOOPS));

#ifdef MA__LWPS
/* Test de l'export de syscall */
#ifdef DEBUG
	fprintf(stderr, "Testing syscall functions\n");
#endif
	duree=0;
	duree2=0;
/*	for(i=0;i<SYSCALL_LOOPS/2;i++)
		piom_test_read_sys();
	fprintf(stderr, "warmup done\n");
*/
	for(i=0;i<SYSCALL_LOOPS;i++){
		//fprintf(stderr, "syscall loop %d\n", i);
		PROF_EVENT1(BOUCLE, i);
		TBX_GET_TICK(t1);
		piom_test_read_sys();
		TBX_GET_TICK(t3);
		duree+=TBX_TICK_RAW_DIFF(t1, t2);
		duree2+=TBX_TICK_RAW_DIFF(t2, t3);		
		if(!(i%10)){
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
