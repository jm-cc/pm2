/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

/* 
 * WARNING : This current UDP implementation *requires* to be compiled
 * with the -DPM2 flag.
 */

#include <sys/netinterf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <marcel.h>

#define CREDIT_ASAP

enum { NET_MESSAGE, NET_REQUEST, NET_CREDITS };

#define SOCKET_BUFFER_SIZE   0xC000 /* 48 Ko */
#define UDP_HEADER_SIZE      220

/*
 * HEAD_OF_HEADER *must* be less or equal to PIGGYBACK_AREA_LEN!
 */

#define HEAD_OF_HEADER       2

enum { H_TAG, H_CREDITS };

#define MAX_MSG_SIZE         8192
#define MAX_HEADER           (MAX_MSG_SIZE - UDP_HEADER_SIZE)

static int send_sock[MAX_MODULES];

static unsigned mes_credits[MAX_MODULES],
           credits_a_rendre[MAX_MODULES],
           credits_attendus[MAX_MODULES];
static boolean request_sent;

static marcel_sem_t credit_sem[MAX_MODULES], mutex[MAX_MODULES];

static int num_port[MAX_MODULES];
static int private_pipe[2];
static int pm2self, confsize;
static fd_set read_fds;
static int nb_fds;

static char my_name[128];

#define max(a, b)  ((a) > (b) ? (a) : (b))

static void spawn_procs(char *argv[])
{
  char cmd[1024], port[128];
  int i;

  sprintf(cmd, "%s/bin/udp/madspawn %s %d", getenv("MADELEINE_ROOT"), my_name, confsize);
  for(i=1; i<confsize; i++) {
    sprintf(port, " %d ", num_port[i]);
    strcat(cmd, port);
  }
  i=0;
  while(argv[i]) {
    sprintf(port, " %s ", argv[i]);
    strcat(cmd, port);
    i++;
  }
  system(cmd);
}

static int creer_socket(int type, int port, struct sockaddr_in *adresse)
{
  struct sockaddr_in tempo;
  int desc;
  int length = sizeof(struct sockaddr_in);

  if((desc = socket(AF_INET, type, 0)) == 1) {
    perror("ERROR: Cannot create socket\n");
    return -1;
  }

  tempo.sin_family = AF_INET;
  tempo.sin_addr.s_addr = htonl(INADDR_ANY);
  tempo.sin_port = htons(port);

  if(bind(desc, (struct sockaddr *)&tempo, sizeof(struct sockaddr_in)) == -1) {
    perror("ERROR: Cannot bind socket\n");
    close(desc);
    return -1;
  }

  if(adresse != NULL)
    if(getsockname(desc, (struct sockaddr *)adresse, &length) == -1) {
      perror("ERROR: Cannot get socket name\n");
      return -1;
    }

  return desc;
}

static void master(char *argv[])
{
  struct sockaddr_in adresse;
  int port;
  char hostname[64], execname[1024];
  int i;

  for(i=1; i<confsize; i++) {
    if((send_sock[i] = creer_socket(SOCK_DGRAM, 0, &adresse)) == -1)
      exit(1);
    num_port[i] = ntohs(adresse.sin_port);
  }

  if(argv[0][0] != '/') {
    getcwd(execname, 1024);
    strcat(execname, "/");
    strcat(execname, argv[0]);
    argv[0] = execname;
  }
  spawn_procs(argv);

  {
    int len;
    struct sockaddr_in other;
    struct hostent *host;

    for(i=1; i<confsize; i++) {
      read(send_sock[i], &len, sizeof(int));
      read(send_sock[i], hostname, len);
      read(send_sock[i], &port, sizeof(int));

      if((host = gethostbyname(hostname)) == NULL) {
	fprintf(stderr, "ERROR: Cannot find internet address of %s\n", hostname);
	exit(1);
      }

      other.sin_family = AF_INET;
      other.sin_port = htons(port);
      memcpy(&other.sin_addr.s_addr, host->h_addr, host->h_length);

      if(connect(send_sock[i], (struct sockaddr *)&other, sizeof(struct sockaddr_in)) == -1){
	perror("connect failed\n");
	exit(1);
      }
    }
  }

  {
    int i, n;

    for(i=1; i<confsize-1; i++) {
      /* on recoit les coordonnées du processus i et on les diffuse */
      int j, len;

      for(j=i+1; j<confsize; j++) {
	read(send_sock[i], &len, sizeof(int));
	read(send_sock[i], hostname, len);
	read(send_sock[i], &port, sizeof(int));

	write(send_sock[j], &len, sizeof(int));
	write(send_sock[j], hostname, len);
	write(send_sock[j], &port, sizeof(int));
      }
    }

    /* Attente que les processus aient terminés leurs connections */
    for(i=1; i<confsize; i++) {
      read(send_sock[i], &n, sizeof(int));
    }

    /* C'est bon, tout le monde peut démarrer ! */
    for(i=1; i<confsize; i++) {
      write(send_sock[i], &n, sizeof(int));
    }
  }
}

static void slave(int serverport, char *serverhost)
{
  struct sockaddr_in adresse, serveur, other;
  struct hostent *host;
  char hostname[64];
  int i, j, len, port;

  if((host = gethostbyname(serverhost)) == NULL) {
    fprintf(stderr, "ERROR: Cannot find internet address of %s\n", serverhost);
    exit(1);
  }

  if((send_sock[0] = creer_socket(SOCK_DGRAM, 0, &adresse)) == -1)
    exit(1);

  serveur.sin_family = AF_INET;
  serveur.sin_port = htons(serverport);
  memcpy(&serveur.sin_addr.s_addr, host->h_addr, host->h_length);

  if(connect(send_sock[0], (struct sockaddr *)&serveur, sizeof(struct sockaddr_in)) == -1){
    perror("connect failed\n");
    exit(1);
  }

  len = strlen(my_name)+1;
  port = ntohs(adresse.sin_port);
  write(send_sock[0], &len, sizeof(int));
  write(send_sock[0], my_name, len);
  write(send_sock[0], &port, sizeof(int));

  /* Etablissement des connections avec les autres */
  for(i=1; i<confsize-1; i++) {
    if(pm2self == i) {
      int port, len;

      for(j=pm2self+1; j<confsize; j++) {

	if((send_sock[j] = creer_socket(SOCK_DGRAM, 0, &adresse)) == -1) {
	  fprintf(stderr, "Aie...\n");
	  exit(1);
	}

	len = strlen(my_name)+1;
	port = ntohs(adresse.sin_port);
	write(send_sock[0], &len, sizeof(int));
	write(send_sock[0], my_name, len);
	write(send_sock[0], &port, sizeof(int));

	read(send_sock[j], &len, sizeof(int));
	read(send_sock[j], hostname, len);
	read(send_sock[j], &port, sizeof(int));

	if((host = gethostbyname(hostname)) == NULL) {
	  fprintf(stderr, "ERROR: Cannot find internet address of %s\n", hostname);
	  exit(1);
	}

	other.sin_family = AF_INET;
	other.sin_port = htons(port);
	memcpy(&other.sin_addr.s_addr, host->h_addr, host->h_length);

	if(connect(send_sock[j], (struct sockaddr *)&other, sizeof(struct sockaddr_in)) == -1){
	  perror("connect failed\n");
	  exit(1);
	}
      }

    } else if(pm2self > i) {
      int len, port;

      read(send_sock[0], &len, sizeof(int));
      read(send_sock[0], hostname, len);
      read(send_sock[0], &port, sizeof(int));

      if((host = gethostbyname(hostname)) == NULL) {
	fprintf(stderr, "ERROR: Cannot find internet address of %s\n", hostname);
	exit(1);
      }

      if((send_sock[i] = creer_socket(SOCK_DGRAM, 0, &adresse)) == -1) {
	fprintf(stderr, "Aie...\n");
	exit(1);
      }

      serveur.sin_family = AF_INET;
      serveur.sin_port = htons(port);
      memcpy(&serveur.sin_addr.s_addr, host->h_addr, host->h_length);

      if(connect(send_sock[i], (struct sockaddr *)&serveur, sizeof(struct sockaddr_in)) == -1){
	perror("connect failed\n");
	exit(1);
      }

      len = strlen(my_name)+1;
      port = ntohs(adresse.sin_port);
      write(send_sock[i], &len, sizeof(int));
      write(send_sock[i], my_name, len);
      write(send_sock[i], &port, sizeof(int));
    }
  }

  /* envoi de l'accusé de réception, et attente de confirmation */
  write(send_sock[0], &j, sizeof(int));
  read(send_sock[0], &j, sizeof(int));
}

void mad_udp_network_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int i, f;
  char output[64];
  int packet = SOCKET_BUFFER_SIZE;
  struct linger ling = { 1, 50 };

  if(getenv("MAD_MY_NUMBER") == NULL) { /* master process */

    pm2self = 0;

    setbuf(stdout, NULL);
    dup2(STDERR_FILENO, STDOUT_FILENO);

    {
      int ret = system("exit `cat ${MADELEINE_ROOT}/.madconf | wc -w`");

      confsize = WEXITSTATUS(ret);
    }

    {
      FILE *f;

      sprintf(output, "%s/.madconf", getenv("MADELEINE_ROOT"));
      f = fopen(output, "r");
      if(f == NULL) {
	perror("fichier ~/.madconf");
	exit(1);
      }
      fscanf(f, "%s", my_name); /* first line of .madconf */
      fclose(f);
    }

    if(nb_proc == NETWORK_ASK_USER) {
      do {
	tprintf("Config. size (1-%d or %d for one proc. per node) : ", MAX_MODULES, NETWORK_ONE_PER_NODE);
	scanf("%d", &nb_proc);
      } while((nb_proc < 0 || nb_proc > MAX_MODULES) && nb_proc != NETWORK_ONE_PER_NODE);
    }

    if(nb_proc != NETWORK_ONE_PER_NODE)
      confsize = nb_proc;

#ifdef DEBUG
    fprintf(stderr, "Process %d started on %s\n", pm2self, my_name);
#endif

    if(confsize > 1)
      master(argv);

  } else { /* slave process */

    setsid();

    pm2self = atoi(getenv("MAD_MY_NUMBER"));
    confsize = atoi(getenv("MAD_NUM_NODES"));
    strcpy(my_name, getenv("MAD_MY_NAME"));

#ifdef PM2
    sprintf(output, "/tmp/%s-pm2log-%d", getenv("USER"), pm2self);
#else
    sprintf(output, "/tmp/%s-madlog-%d", getenv("USER"), pm2self);
#endif
    dup2((f = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0600)), STDOUT_FILENO);
    dup2(STDOUT_FILENO, STDERR_FILENO);
    close(f);

#ifdef DEBUG
    fprintf(stderr, "Process %d started on %s\n", pm2self, my_name);
#endif

    slave(atoi(getenv("MAD_SERVER_PORT")), getenv("MAD_SERVER_HOST"));
  }

  pipe(private_pipe);
  send_sock[pm2self] = private_pipe[0]; /* uniformisation de la réception (en UDP ???) */

  for(i=0; i<confsize; i++) {
    /*     fcntl(send_sock[i], F_SETFL, O_NONBLOCK); */
    if(i != pm2self) {
       if(setsockopt(send_sock[i], SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(struct linger)) == -1) {
	 perror("setsockopt failed");
	 exit(1);
       }
       if(setsockopt(send_sock[i], SOL_SOCKET, SO_SNDBUF, (char *)&packet, sizeof(int)) == -1) {
	 perror("setsockopt failed");
	 exit(1);
       }
       if(setsockopt(send_sock[i], SOL_SOCKET, SO_RCVBUF, (char *)&packet, sizeof(int)) == -1) {
	 perror("setsockopt failed");
	 exit(1);
       }
    }
  }

  FD_ZERO(&read_fds);
  nb_fds = 0;
  for(i=0; i<confsize; i++) {
    FD_SET(send_sock[i], &read_fds);
    nb_fds = max(nb_fds, send_sock[i]);
  }

  request_sent = FALSE;
  for(i=0; i<confsize; i++) {
    marcel_sem_init(&credit_sem[i], 0);
    marcel_sem_init(&mutex[i], 1);
    credits_a_rendre[i] = credits_attendus[i] = 0;
    mes_credits[i] = SOCKET_BUFFER_SIZE - 2*(UDP_HEADER_SIZE+HEAD_OF_HEADER*sizeof(long));
  }

  *nb = confsize;
  *whoami = pm2self;
  for(i=0; i<confsize; i++)
    tids[i] = i;
}

void mad_udp_network_send(int dest_node, struct iovec *vector, size_t count)
{
  int i, fd, n;
  long entete[HEAD_OF_HEADER];

  marcel_sem_P(&mutex[dest_node]);

  if(dest_node == pm2self)
    fd = private_pipe[1];
  else {
    fd = send_sock[dest_node];

    n = UDP_HEADER_SIZE + vector[0].iov_len;

    if(count > 1) {
      n += UDP_HEADER_SIZE;
      for(i=1; i<count; i++)
	n += vector[i].iov_len;
    }

    lock_task();
    if(mes_credits[dest_node] < n) {
      credits_attendus[dest_node] = n - mes_credits[dest_node];
      mes_credits[dest_node] = 0;
      if(!request_sent) { /*  Seulement utile lorsque CREDIT_ASAP est défini  */
	entete[H_TAG] = NET_REQUEST;
	entete[H_CREDITS] = credits_a_rendre[dest_node];
	credits_a_rendre[dest_node] = 0;
	write(fd, entete, HEAD_OF_HEADER*sizeof(long));
	request_sent = TRUE;
      }
      unlock_task();
      marcel_sem_P(&credit_sem[dest_node]);
    } else {
      mes_credits[dest_node] -= n;
      unlock_task();
    }
  }

  lock_task();

  {
    register long *ptr = (long *)vector[0].iov_base;

    ptr[H_TAG] = NET_MESSAGE;
    ptr[H_CREDITS] = credits_a_rendre[dest_node];
    credits_a_rendre[dest_node] = 0;
  }

  unlock_task();

  write(fd, vector[0].iov_base, vector[0].iov_len);

  if(count > 1) {
    writev(fd, vector+1, count-1);
  }

  marcel_sem_V(&mutex[dest_node]);
}

static int current_expeditor;

void mad_udp_network_receive(network_handler func)
{ 
  int n, i;
  fd_set rfds;
  struct timeval tv;
  static long header[MAX_HEADER/sizeof(long)];
  long entete[HEAD_OF_HEADER];

  do {
    do {
      rfds = read_fds;
      lock_task();
      if(marcel_self()->next == marcel_self()) {
	n = select(nb_fds+1, &rfds, NULL, NULL, NULL);
      } else {
	timerclear(&tv);
	n = select(nb_fds+1, &rfds, NULL, NULL, &tv);
      }
      unlock_task();
      if(n <= 0)
	marcel_givehandback_np();
    } while(n <= 0);

    for(i=0; i<confsize; i++) {
      if(FD_ISSET(send_sock[i], &rfds)) {
	n = read(send_sock[i], header, MAX_HEADER);
	if(n == 0) { /* fermeture de socket détectée */
#ifdef DEBUG
	  printf("connexion coupée avec le processus %d\n", i);
#endif
	  FD_CLR(send_sock[i], &read_fds);
	  close(send_sock[i]);
	} else if(header[H_TAG] == NET_MESSAGE) {
	  lock_task();
	  mes_credits[i] += header[H_CREDITS];
	  credits_a_rendre[i] += UDP_HEADER_SIZE + n;
#ifdef CREDIT_ASAP
	  if(credits_attendus[i] && mes_credits[i] >= credits_attendus[i]) {
	    mes_credits[i] -= credits_attendus[i];
	    credits_attendus[i] = 0;
	    marcel_sem_V(&credit_sem[i]);
	  }
#endif
	  unlock_task();
	} else if(header[H_TAG] == NET_CREDITS) {
	  lock_task();
	  request_sent = FALSE;
	  mes_credits[i] += header[H_CREDITS];
	  if(credits_attendus[i]) {
	    if(mes_credits[i] >= credits_attendus[i]) {
	      mes_credits[i] -= credits_attendus[i];
	      credits_attendus[i] = 0;
	      marcel_sem_V(&credit_sem[i]);
	    } else { /* il faut renvoyer une requête */
#ifdef CREDIT_ASAP
	      entete[H_TAG] = NET_REQUEST;
	      entete[H_CREDITS] = credits_a_rendre[i];
	      credits_a_rendre[i] = 0;
	      write(send_sock[i], entete, HEAD_OF_HEADER*sizeof(long));
	      request_sent = TRUE;
#else
	      RAISE(PROGRAM_ERROR); /* this should never happen ! */
#endif
	    }
	  }
	  unlock_task();
	  /* set n back to 0 to avoid exiting from the loop */
	  n = 0;
	} else { /* header[H_TAG] == NET_REQUEST */
	  lock_task();
	  mes_credits[i] += header[H_CREDITS];
	  entete[H_TAG] = NET_CREDITS;
	  entete[H_CREDITS] = credits_a_rendre[i]; /* peut etre zero... */
	  credits_a_rendre[i] = 0;
	  write(send_sock[i], entete, HEAD_OF_HEADER*sizeof(long));
#ifdef CREDIT_ASAP
	  if(credits_attendus[i] && mes_credits[i] >= credits_attendus[i]) {
	    mes_credits[i] -= credits_attendus[i];
	    credits_attendus[i] = 0;
	    marcel_sem_V(&credit_sem[i]);
	  }
#endif
	  unlock_task();
	  /* set n back to 0 to avoid exiting from the loop */
	  n = 0;
	}
	break;
      }
    }
  } while(n == 0);

  current_expeditor = i;
  (*func)(header, n);
}

void mad_udp_network_receive_data(struct iovec *vector, size_t count)
{
  int n;

  n = readv(send_sock[current_expeditor], vector, count);
  lock_task();
  credits_a_rendre[current_expeditor] += UDP_HEADER_SIZE + n;
  unlock_task();
}

void mad_udp_network_exit()
{
  int i;

  for(i=0; i<confsize; i++)
    close(send_sock[i]);
  close(private_pipe[1]);

#ifdef DEBUG
  for(i=0; i<confsize; i++)
    if(i != pm2self) {
      fprintf(stderr, "mes_crédits[%d] = %d (sur %d au départ)\n", i, mes_credits[i], SOCKET_BUFFER_SIZE - 2*(UDP_HEADER_SIZE+HEAD_OF_HEADER*sizeof(long)));
      fprintf(stderr, "crédits_a_rendre[%d] = %d\n", i, credits_a_rendre[i]);
    }
#endif
}

static netinterf_t mad_udp_netinterf = {
  mad_udp_network_init,
  mad_udp_network_send,
  mad_udp_network_receive,
  mad_udp_network_receive_data,
  mad_udp_network_exit
};

netinterf_t *mad_udp_netinterf_init()
{
  return &mad_udp_netinterf;
}

