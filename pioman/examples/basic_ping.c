/* basic_ping.c */

#define __PROF_APP__
#include <sys/utsname.h>
#include <semaphore.h>
#include "pm2_common.h"


static char * buffer;
static int sock;
static int fini;

#define ROUNDS 1
#define NB_LOOP 1000
#define WARMUP 100
#define SIZE 4

static int file_des;

//#define DEBUG 1
#ifdef MARCEL // threads bourrins uniquement avec Marcel
#define NB_BOURRIN 4
#else
#define NB_BOURRIN 1
#endif
//#define PRINT_STATS 1

#if 1
#define WRITE  piom_write
#define READ   piom_read
#define SELECT piom_select

#else
#define WRITE  write
#define READ   read
#define SELECT select

#endif
void envoi(void* b, unsigned int size, int id)
{
#if DEBUG
	fprintf(stderr, "[%d]\tSending %d to fd %d\n", id, buffer[id], file_des);
#endif
	int done=0;
	int ret=0;
	do{    	
		PROF_EVENT(rec_write);
		do{
			ret = WRITE(file_des, (void*)b, size);
		}while(ret < 0);
		done += ret;
		PROF_EVENT(rec_write_done);
	}while(done<SIZE);
#if DEBUG
	fprintf(stderr, "[%d]\tData sent (fd %d)\n", id, file_des);
#endif
}

void rec(void * b, unsigned int size, int id)
{
	int done=0;
	int ret=0;
	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(file_des, &fd);
#if DEBUG
	fprintf(stderr, "[%d]\tReceiving on fd %d\n", id, file_des);
#endif
	do{
		PROF_EVENT(rec_read);
		do {
			ret = READ(file_des, (void*)b,size);    
		}while(ret < 0);
		done+=ret;
		PROF_EVENT(rec_read_done);
	}while(done<size);
#if DEBUG
	fprintf(stderr, "[%d]\tReceived : %d from fd %d\n", id, buffer[id], file_des);
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
	fprintf(stderr, "##Starting greedy thread\n");
#endif
#ifdef MARCEL
	marcel_sem_V(&ready_sem);
#endif
	while(!fin[my_pos]);
#if DEBUG
	fprintf(stderr, "End of greedy thread\n");
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

static int max_serv, max_client;
void serveur(){
	int id_server=max_serv++;
	int i,j;
	int iter=0;
	marcel_attr_t attr_main;
	marcel_attr_init(&attr_main);
	marcel_attr_setname(&attr_main, "greedy");

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
	fprintf(stderr, "Starting benchmark (server %d)\n", id_server);
#endif /* DEBUG */
	pos=0;

	for(i=0; i < NB_BOURRIN; i++)
	{      
		PROF_EVENT1(BOUCLE,i);
		/* Tours de chauffe non mesurés */
		for(j=0;j<WARMUP;j++) {
#if DEBUG
			fprintf(stderr, "Loop (%d) : iter= %d\n",id_server,iter);
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
			fprintf(stderr, "Loop (%d) : iter= %d\n",id_server, iter);
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
		fprintf(stderr, "#%d Greedy threads done\n",i);
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
			fprintf(stderr, "Loop (%d) : iter= %d\n",id_server, iter);
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
			fprintf(stderr, "Loop (%d) : iter= %d\n",id_server, iter);
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
		fprintf(stderr, "#%d Greedy threads done\n",i);
		ok=0;
	}
#ifndef PRINT_STATS
	fprintf(stderr, "Benchmark done (server %d)\n", id_server);
#endif
	PROF_EVENT(FIN_PING_PONG);
	sleep(1);
          
	shutdown(sock,2);
}

void client() {
	int i,j;
	int id_client=max_client++;
	int iter;
	tbx_tick_t wake1,wake2;
	long long int duree;
	fprintf(stderr, "Launching client %d\n",id_client);
		
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
		fprintf(stderr, "###%d greedy threads : latency = %lf µs\n",i, tbx_tick2usec(duree)/(2*NB_LOOP));
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
		fprintf(stderr, "###%d greedy : latency = %lf µs\n",i, tbx_tick2usec(duree)/(2*NB_LOOP));
#endif
		ok=0;
	}
	envoyer(id_client);

}

void usage(char** argv)
{
	fprintf(stderr, "Usage: %s -s|-c\n", argv[0]);
	exit(1);
}
int main(int argc, char ** argv)
{
	common_init(&argc, argv, NULL);
	struct sockaddr_in addr;
	socklen_t length;
	int i,j;
	int yes = 1;
	fini=0;
	int is_server=0;
	char *remote_drv_url=NULL;

	if(argc < 2 )
		usage(argv);
	if(!strcmp(argv[1],"-s")){
		is_server=1;
		printf("Running as server\n");
	} else if (!strcmp(argv[1],"-c")){
		is_server=0;
		printf("Running as client\n");
	} else {
		usage(argv);
	}

#ifdef MARCEL
	/* Initialize thread-related structures */
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

	/* crappy connecting management */
	struct sockaddr_in address;
        socklen_t          len  = sizeof(struct sockaddr_in);
        struct sockaddr_in temp;
        int                server_fd;
	uint16_t	   port;
	struct utsname     utsname;
        SYSCALL(server_fd = socket(AF_INET, SOCK_STREAM, 0));
	
        temp.sin_family      = AF_INET;
        temp.sin_addr.s_addr = htonl(INADDR_ANY);
        temp.sin_port        = htons(0);

        SYSCALL(bind(server_fd, (struct sockaddr *)&temp, len));

	SYSCALL(getsockname(server_fd, (struct sockaddr *)&address, &len));
	SYSCALL(listen(server_fd, 5));
	uname(&utsname);
	port		= ntohs(address.sin_port);
	
	if(is_server) {
		int fd;
		printf("%s -- %d\n", utsname.nodename, port);
		SYSCALL(fd = accept(server_fd, NULL, NULL));

		file_des=fd;
		int	    val = 1;
		socklen_t sock_len = sizeof(int);
		SYSCALL(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sock_len));
		SYSCALL(fcntl(fd, F_SETFL, O_NONBLOCK));
	} else {
		char *remote_hostname, *remote_port;
		remote_hostname=argv[2];
		remote_port=argv[3];

		int fd;
		printf("%s:%d\n", utsname.nodename, port);
		char 			*saveptr = NULL;
		port	= strtol(remote_port, (char **)NULL, 10);
		
		printf("connecting to server %s on port %d\n", remote_hostname, port);
		SYSCALL(fd = socket(AF_INET, SOCK_STREAM, 0));
		struct sockaddr_in temp;

		temp.sin_family      = AF_INET;
		temp.sin_addr.s_addr = htonl(INADDR_ANY);
		temp.sin_port        = htons(0);
		
		SYSCALL(bind(fd, (struct sockaddr *)&temp, len));

		struct hostent *host_entry;
		host_entry = gethostbyname(remote_hostname);
		address.sin_family	= AF_INET;
		address.sin_port	= htons(port);
		memcpy(&address.sin_addr.s_addr,
		       host_entry->h_addr,
		       (size_t)host_entry->h_length);		
		memset(address.sin_zero, 0, 8);

		SYSCALL(connect(fd, (struct sockaddr *)&address,
				sizeof(struct sockaddr_in)));
		file_des=fd;
		int val;
		socklen_t len = sizeof(int);
		SYSCALL(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&val, len));
		SYSCALL(fcntl(fd, F_SETFL, O_NONBLOCK));
 	}

#ifdef DO_PROFILE
	profile_activate(FUT_ENABLE,USER_APP_MASK|MARCEL_PROF_MASK|MAD_PROF_MASK|PIOM_PROF_MASK, 0);	  
#endif

	if(is_server)
	{
		serveur();
	}  
	else
	{
		client();
	}
#ifdef DO_PROFILE
	profile_stop();
#endif

	fprintf(stderr, "### BENCHMARK DONE ###\n");
	fini=1;        
	shutdown(sock,2);              
	common_exit(NULL);
	return 0;
}
