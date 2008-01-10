/* multi_connect.c */

#define __PROF_APP__
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <semaphore.h>
#include "pm2_common.h"


static char * buffer;
static int sock;
static int fini;

#define ROUNDS 1
#define NB_LOOP 1000
#define WARMUP 100
#define SIZE 4
#define NB_SERVER 1
#define NB_CLIENT 1

static int file_des[NB_CLIENT];

//#define DEBUG 1
#ifdef MARCEL // threads bourrins uniquement avec Marcel
#define NB_BOURRIN 4
#else
#define NB_BOURRIN 1
#endif
//#define PRINT_STATS 1


void envoi(void* b, unsigned int size, int id)
{
#if DEBUG
	fprintf(stderr, "Thread %d\n",id);
	fprintf(stderr, "\tEnvoyer %d to fd %d\n", buffer[id], file_des[id]);
#endif
	int done=0;
	do{    	
		PROF_EVENT(rec_write);
		done+=piom_write(file_des[id], (void*)b, size);
		PROF_EVENT(rec_write_done);
	}while(done<SIZE);
#if DEBUG
	fprintf(stderr, "Thread %d\n",id);
	fprintf(stderr, "\tEnvoyer done (fd %d)\n", file_des[id]);
#endif
}

void rec(void * b, unsigned int size, int id)
{
	int done=0;
	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(file_des[id], &fd);
#if DEBUG
	fprintf(stderr, "Thread %d\n",id);
	fprintf(stderr, "\trecevoir (fd=%d)\n", file_des[id]);
#endif
	do{
		PROF_EVENT(rec_read);
		done+=piom_read(file_des[id], (void*)b,size);    
		PROF_EVENT(rec_read_done);
	}while(done<size);
#if DEBUG
	fprintf(stderr, "Thread %d\n",id);
	fprintf(stderr, "\tRecu : %d from fd %d\n", buffer[id], file_des[id]);
#endif
}

void envoyer(int id)
{
	PROF_EVENT(ENVOYER);
	envoi(&buffer[id],sizeof(char), id);
	PROF_EVENT(ENVOYER_DONE);
}

void recevoir(int id)
{
	PROF_EVENT(RECEVOIR);
	rec(&buffer[id],sizeof(char), id);
	PROF_EVENT(RECEVOIR_DONE);
}

#ifdef MARCEL
static pmarcel_sem_t ready_sem;
static pmarcel_sem_t bourrin_ready;
#endif 

static int ok;
static volatile int fin[NB_BOURRIN];
static int pos;

void bourrin()
{
	int my_pos=pos++;
	ok++;
#ifndef PRINT_STATS
	fprintf(stderr, "##Demarrage du bourrin\n");
#endif
#ifdef MARCEL
	marcel_sem_V(&ready_sem);
#endif
	while(!fin[my_pos]);
#if DEBUG
	fprintf(stderr, "Fin du bourrin\n");
#endif
}

/* Burne pendant 100 ms */
void burn()
{
	tbx_tick_t t1, t2;
	long long int duree;
	return ;
	PROF_EVENT(debur_du_burner);
	TBX_GET_TICK(t1);
	for(;;)
	{
		TBX_GET_TICK(t2);
		duree=TBX_TICK_RAW_DIFF(t1, t2) ;
		if(tbx_tick2usec(duree) > 1000000)
		{
			PROF_EVENT(fin_du_burner);
			return;
		}
	}
}

#define IP_JOE "10.0.0.3"
#define IP_JACK "10.0.0.4"

static int max_serv, max_client;
void serveur(){
	int id_server=max_serv++;
	int i,j;
	int iter=0;
	marcel_attr_t attr_main;
	marcel_attr_init(&attr_main);
	marcel_attr_setname(&attr_main, "bourrin");

	marcel_sem_init(&ready_sem,0);
 
	struct marcel_sched_param par;
	par.__sched_priority= MA_DEF_PRIO;
	marcel_attr_setschedpolicy(&attr_main, MARCEL_SCHED_SHARED);
	marcel_sched_setscheduler(marcel_self(),MARCEL_SCHED_SHARED, &par);

	struct marcel_sched_param main_param;
	main_param.__sched_priority=MA_MAX_RT_PRIO;
	marcel_sched_setparam(marcel_self(), &main_param);

	/* Debut du test */
#if DEBUG
	fprintf(stderr, "Debut du test (server %d)\n",id_server);
#endif /* DEBUG */
	pos=0;

	for(i=0; i < NB_BOURRIN; i++)
	{      
		/* Tours de chauffe non mesurés */
		for(j=0;j<WARMUP;j++) {
			PROF_EVENT1(BOUCLE,i);
#if DEBUG
			fprintf(stderr, "Boucle (%d) : iter= %d\n",id_server,iter);
#endif /* DEBUG */

			PROF_EVENT1(ENVOYER,j);
			envoi(&iter,sizeof(int), id_server);
			PROF_EVENT(ENVOYER_DONE);
			PROF_EVENT(RECEVOIR);
			rec(&iter,sizeof(int), id_server);
			PROF_EVENT(RECEVOIR_DONE);

			iter++;
		}

		PROF_EVENT(pret_a_travailler);
		/* Tours chronometrés */
		for(j=0;j<NB_LOOP;j++) {
#if DEBUG
			fprintf(stderr, "Boucle (%d) : iter= %d\n",id_server, iter);
#endif /* DEBUG */

			PROF_EVENT1(ENVOYER,j);
			envoi(&iter,sizeof(int), id_server);
			PROF_EVENT(ENVOYER_DONE);
			PROF_EVENT(RECEVOIR);
			rec(&iter,sizeof(int), id_server);
			PROF_EVENT(RECEVOIR_DONE);

			burn();
			iter++;
		}
		ok=0;
		fprintf(stderr, "#%d Bourrins done\n",i);
		fin[i]=0;
#ifdef MARCEL
		marcel_sem_V(&bourrin_ready);
//		marcel_create(&pid[i], &attr_main, (void*)bourrin, NULL);
#endif // MARCEL

	}
	PROF_EVENT(DESTRUCTION_BOURRINS);
	for(i=NB_BOURRIN-1; i >=0; i--)
	{
		fin[i]=1;

		for(j=0;j<WARMUP;j++) {
#if DEBUG
			fprintf(stderr, "Boucle (%d) : iter= %d\n",id_server, iter);
#endif

			PROF_EVENT1(ENVOYER,j);
			envoi(&iter,sizeof(int), id_server);
			PROF_EVENT(ENVOYER_DONE);
			PROF_EVENT(RECEVOIR);
			rec(&iter,sizeof(int), id_server);
			PROF_EVENT(RECEVOIR_DONE);
			iter++;
		}

		for(j=0;j<NB_LOOP;j++) {
#if DEBUG
			fprintf(stderr, "Boucle (%d) : iter= %d\n",id_server, iter);
#endif /* DEBUG */
			PROF_EVENT1(ENVOYER,j);
			envoi(&iter,sizeof(int), id_server);
			PROF_EVENT(ENVOYER_DONE);
			PROF_EVENT(RECEVOIR);
			rec(&iter,sizeof(int), id_server);
			PROF_EVENT(RECEVOIR_DONE);
			iter++;
		}
		marcel_sem_V(&bourrin_ready);
		fprintf(stderr, "#%d Bourrins done\n",i);
		ok=0;
	}
#ifndef PRINT_STATS
	fprintf(stderr, "FIN DU TEST (server %d)\n", id_server);
#endif
	PROF_EVENT(FIN_PING_PONG);
	sleep(1);
          
#if DEBUG
	fprintf(stderr, "Fin du ping pong\n");
#endif
	shutdown(sock,2);
}

void client() {
	int i,j;
	int id_client=max_client++;
	int iter;
	tbx_tick_t wake1,wake2;
	long long int duree;
	fprintf(stderr, "Lancement du client %d\n",id_client);
		
	for(i=0; i < NB_BOURRIN; i++)
	{
		PROF_EVENT1(BOUCLE, i);
		for(j=0;j<WARMUP;j++) {
			TBX_GET_TICK(wake1);

			PROF_EVENT(RECEVOIR);
			rec(&iter,sizeof(int), id_client);
			PROF_EVENT(RECEVOIR_DONE);

			PROF_EVENT1(ENVOYER,j);
			envoi(&iter,sizeof(int), id_client);
			PROF_EVENT(ENVOYER_DONE);

			TBX_GET_TICK(wake2);
			duree=TBX_TICK_RAW_DIFF(wake1, wake2);
#ifdef PRINT_STATS 
			fprintf(stderr, "#%d\t%.1f (server %d) (%d)\n", i, tbx_tick2usec(TBX_TICK_RAW_DIFF(wake1, wake2))/2, id_client, iter);
#endif
			burn();
		}
		duree=0;
		PROF_EVENT(warmup_done);
		for(j=0;j<NB_LOOP;j++) {
			TBX_GET_TICK(wake1);
			PROF_EVENT(RECEVOIR);
			rec(&iter,sizeof(int), id_client);
			PROF_EVENT(RECEVOIR_DONE);

			PROF_EVENT1(ENVOYER,j);
			envoi(&iter,sizeof(int), id_client);
			PROF_EVENT(ENVOYER_DONE);

			TBX_GET_TICK(wake2);
			duree+= TBX_TICK_RAW_DIFF(wake1, wake2);
#ifdef PRINT_STATS 
			fprintf(stderr, "#%d\t%.1f (server %d) (%d)\n", i, tbx_tick2usec(TBX_TICK_RAW_DIFF(wake1, wake2))/2, id_client, iter);
#endif
			burn();
		}
#ifndef PRINT_STATS
	  fprintf(stderr, "###%d Bourrins : latence = %lf µs\n",i, tbx_tick2usec(duree)/(2*NB_LOOP));
#endif
		ok=0;
	}

	for(i=NB_BOURRIN-1; i>=0; i--)
	{
		PROF_EVENT1(BOUCLE2, i);
		for(j=0;j<WARMUP;j++) {
			TBX_GET_TICK(wake1);
			PROF_EVENT(RECEVOIR);
			rec(&iter,sizeof(int), id_client);
			PROF_EVENT(RECEVOIR_DONE);

			PROF_EVENT1(ENVOYER,j);
			envoi(&iter,sizeof(int), id_client);
			PROF_EVENT(ENVOYER_DONE);

			TBX_GET_TICK(wake2);
			burn();
#ifdef PRINT_STATS 
			fprintf(stderr, "#%d\t%.1f (server %d) (%d)\n", i, tbx_tick2usec(TBX_TICK_RAW_DIFF(wake1, wake2))/2, id_client, iter);
#endif

		}
		duree=0;
		PROF_EVENT(warmup2_done);
		for(j=0;j<NB_LOOP;j++) {
			TBX_GET_TICK(wake1);

			PROF_EVENT(RECEVOIR);
			rec(&iter,sizeof(int), id_client);
			PROF_EVENT(RECEVOIR_DONE);

			PROF_EVENT1(ENVOYER,j);
			envoi(&iter,sizeof(int), id_client);
			PROF_EVENT(ENVOYER_DONE);

			TBX_GET_TICK(wake2);
			duree+= TBX_TICK_RAW_DIFF(wake1, wake2);
#ifdef PRINT_STATS 
			fprintf(stderr, "#%d\t%.1f (server %d) (%d)\n", i, tbx_tick2usec(TBX_TICK_RAW_DIFF(wake1, wake2))/2, id_client, iter);
#endif
		}
#ifndef PRINT_STATS
	  fprintf(stderr, "###%d Bourrins : latence = %lf µs\n",i, tbx_tick2usec(duree)/(2*NB_LOOP));
#endif
		ok=0;
	}
	envoyer(id_client);

	fprintf(stderr, "## Fin du ping pong\n");	
}

int main(int argc, char ** argv)
{
	common_init(&argc, argv, NULL);
	struct sockaddr_in addr;
	socklen_t length;
	int i,j;
	int yes = 1;
	fini=0;

	fprintf(stderr, "## multi_connect\n");
#ifdef MARCEL
	marcel_t pid[NB_BOURRIN];
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
#endif

	buffer=(char*)malloc(SIZE);
	for(i=0;i<SIZE;i++)
		buffer[i]=0;

/*  Initialisation */
	addr.sin_addr.s_addr=inet_addr(IP_JOE);   

	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror ("socket");
		return 1;
	}
	int port=8000;
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port++); // Port écouté du serveur
	length=sizeof(addr);
	setsockopt(sock, SOL_IP, TCP_NODELAY, &yes, sizeof(yes));

#ifdef DO_PROFILE
	profile_activate(FUT_ENABLE,USER_APP_MASK|MARCEL_PROF_MASK|MAD_PROF_MASK|XPAUL_PROF_MASK, 0);	  
#endif

	if(argc>1)
	{
		/*  CLIENT  */
		for(j=0;j<NB_CLIENT;j++){
			fprintf(stderr, "## Connexion %d\n",j);
			i=0;
			while(connect(sock,(struct sockaddr*)&addr,sizeof(addr)) ==-1)
			{
				fprintf(stderr, "Erreur de connection\n");
				perror("connect");
				if(i++ > 5)
					exit(1);
			}
			file_des[j]=sock;
			fprintf(stderr, "## Connexion %d OK\n",j);
			/* Reinit socket */
			sock = socket (AF_INET, SOCK_STREAM, 0);
			if (sock < 0)
			{
				perror ("socket");
				return 1;
			}

			addr.sin_family=AF_INET;
			addr.sin_port=htons(port++); // Port écouté du serveur
			length=sizeof(addr);
			setsockopt(sock, SOL_IP, TCP_NODELAY, &yes, sizeof(yes));

		}
		marcel_t pid_client[NB_CLIENT];
		for(i= 0; i<NB_CLIENT;i++)
		{
			marcel_create(&pid_client[i], &attr_main, (void*)client, NULL);
		}

	

		for(i= 0; i<NB_CLIENT;i++)
		{
			marcel_join(pid_client[i],  NULL);
		}
	}  
	else
	{
		/* SERVEUR */
		/* Connection */
		for(j=0;j<NB_CLIENT;j++){
			fprintf(stderr, "Creation du server %d (port %d)\n",j,port);
			if(bind(sock,(struct sockaddr*)&addr,length) == -1)
			{
				perror("bind");
				exit(1);
			}      
			setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
			addr.sin_addr.s_addr=INADDR_ANY;
			listen(sock, 1);
			i=0;
			fprintf(stderr, "Bind %d OK\n",j);
			while((file_des[j]=accept(sock,(struct sockaddr*)&addr,&length)) == -1)
			{
				perror("accept");
				if(i++>5)
					exit(0);
			}
			fprintf(stderr, "Accept %d done\n",j);
			/*  Reinit */
			addr.sin_addr.s_addr=inet_addr(IP_JOE);   
	
			sock = socket (AF_INET, SOCK_STREAM, 0);
			if (sock < 0)
			{
				perror ("socket");
				return 1;
			}

			addr.sin_family=AF_INET;
			addr.sin_port=htons(port++); // Port écouté du serveur
			length=sizeof(addr);
			setsockopt(sock, SOL_IP, TCP_NODELAY, &yes, sizeof(yes));


		}
		marcel_t pid_server[NB_SERVER];
		for(i= 0; i<NB_SERVER; i++)
		{
			marcel_create(&pid_server[i], &attr_main, (void*)serveur, NULL);
		}

		for(i=0;i<NB_BOURRIN;i++){
//			for(j=0;j<NB_CLIENT;j++)
//				marcel_sem_P(&bourrin_ready);

			marcel_create(&pid[i], &attr_main, (void*)bourrin, NULL);
		}
				
		for(i= 0; i<NB_SERVER;i++)
		{
			marcel_join(pid_server[i],  NULL);
		}
	}
#ifdef DO_PROFILE
	profile_stop();
#endif

	fprintf(stderr, "\n\n\n\n######################\n");
	fprintf(stderr, "### BENCHMARK DONE ###\n");
	fprintf(stderr, "######################\n\n\n\n");
	fini=1;        
	shutdown(sock,2);              
	common_exit(NULL);
	return 0;
}
