
#define USE_BLOCKING_IO
#define USE_MARCEL_POLL
//#define DEBUG

#ifdef PM2
#include "marcel.h"
#endif

#include "sys/netinterf.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/uio.h>
#ifdef SIG64
#include <sys/resource.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "mad_types.h"
#include "mad_timing.h"
#include "sys/debug.h"

static long header[MAX_HEADER/sizeof(long)] __MAD_ALIGNED__;

#ifdef PM2
static marcel_mutex_t mutex[MAX_MODULES];
#endif
static int send_sock[MAX_MODULES];
static int private_pipe[2];
static int pm2self, confsize;
static fd_set read_fds;
static int nb_fds;

static char my_name[128];

#ifndef min
#define min(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b)  ((a) > (b) ? (a) : (b))
#endif

#define MAX_IOV    16

static char cons_image[1024];

static void parse_cmd_line(int *argc, char **argv)
{
  int i, j;

  i = j = 1;
  strcpy(cons_image, "no");
  while(i<*argc) {
    if(!strcmp(argv[i], "-pm2cons")) {
      if(i == *argc-1) {
	fprintf(stderr, "Fatal error: -pm2cons option must be followed by <console_name>.\n");
	exit(1);
      }
      strcpy(cons_image, argv[i+1]);
      i++;
    } else {
      argv[j++] = argv[i];
    }
    i++;
  }
  *argc = j;
  argv[j] = NULL;
}

static char *get_mad_root(void)
{
  static char buf[1024];
  char *ptr;

  if((ptr = getenv("MAD1_ROOT")) != NULL)
    return ptr;
  else {
    sprintf(buf, "%s/mad1", getenv("PM2_ROOT"));
    return buf;
  }
}

static void spawn_procs(char *argv[], int port)
{ 
  char cmd[1024], arg[128];
  int i;

  LOG_IN();

  sprintf(cmd, "%s/bin/tcp/madspawn %s %d %d %s", get_mad_root(),
	  my_name, confsize, port, cons_image);
  i=0;
  while(argv[i]) {
    if(i == 0)
      sprintf(arg, " %s ", argv[i]);
    else
      sprintf(arg, " '%s' ", argv[i]);
    strcat(cmd, arg);
    i++;
  }
  system(cmd);

  LOG_OUT();
}

static int creer_socket(int type, int port, struct sockaddr_in *adresse)
{
  struct sockaddr_in tempo;
  int desc;
  int length = sizeof(struct sockaddr_in);

  LOG_IN();

  if((desc = socket(AF_INET, type, 0)) == -1) {
    perror("ERROR: Cannot create socket\n");
    LOG_OUT();
    return -1;
  }
  
  tempo.sin_family = AF_INET;
  tempo.sin_addr.s_addr = htonl(INADDR_ANY);
  tempo.sin_port = htons(port);

  if(bind(desc, (struct sockaddr *)&tempo, sizeof(struct sockaddr_in)) == -1) {
    perror("ERROR: Cannot bind socket\n");
    close(desc);
    LOG_OUT();
    return -1;
  }

  if(adresse != NULL)
    if(getsockname(desc, (struct sockaddr *)adresse, &length) == -1) {
      perror("ERROR: Cannot get socket name\n");
      LOG_OUT();
      return -1;
    }

  LOG_OUT();

  return desc;
}

static void master(char *argv[])
{
  int sock;
  struct sockaddr_in adresse;
  int port;
  char hostname[64], execname[1024];

  LOG_IN();

  port = 0;

  if((sock = creer_socket(SOCK_STREAM, port, &adresse)) == -1)
    exit(1);

  if(listen(sock, MAX_MODULES) == -1) {
    perror("master'listen\n");
    exit(1);
  }

  if(argv[0][0] != '/') {
    getcwd(execname, 1024);
    strcat(execname, "/");
    strcat(execname, argv[0]);
    argv[0] = execname;
  }

  spawn_procs(argv, ntohs(adresse.sin_port));

  {
    int i, n, fd;

    for(i=1; i<confsize; i++) {
      do {
	fd = accept(sock, NULL, NULL);
	if(fd == -1 && errno != EINTR) {
	  perror("master'accept");
	  exit(1);
	}
      } while(fd == -1);
      read(fd, &n, sizeof(int));
      send_sock[n] = fd;
    }

    close(sock);

    for(i=1; i<confsize-1; i++) {
      /* on recoit les coordonnées du processus i et on les diffuse aux autres */
      int j, len;

      read(send_sock[i], &len, sizeof(int));
      read(send_sock[i], hostname, len);
      read(send_sock[i], &port, sizeof(int));
      for(j=i+1; j<confsize; j++) {
	write(send_sock[j], &len, sizeof(int));
	write(send_sock[j], hostname, len);
	write(send_sock[j], &port, sizeof(int));
	write(send_sock[j], &i, sizeof(int));
      }
    }

    /* Attente que les processus aient terminés leurs connections respectives... */
    for(i=1; i<confsize; i++) {
      read(send_sock[i], &n, sizeof(int));
    }

    /* C'est bon, tout le monde peut démarrer ! */
    for(i=1; i<confsize; i++) {
      write(send_sock[i], &n, sizeof(int));
    }

  }

  LOG_OUT();
}

static void slave(int serverport, char *serverhost)
{
  struct sockaddr_in adresse, serveur;
  struct hostent *host;
  char hostname[64];
  int i, j;

  LOG_IN();

  if((host = gethostbyname(serverhost)) == NULL) {
    fprintf(stderr, "ERROR: Cannot find internet address of %s\n", serverhost);
    exit(1);
  }

  if((send_sock[0] = creer_socket(SOCK_STREAM, 0, &adresse)) == -1)
    exit(1);

  serveur.sin_family = AF_INET;
  serveur.sin_port = htons(serverport);
  memcpy(&serveur.sin_addr.s_addr, host->h_addr, host->h_length);

  if(connect(send_sock[0], (struct sockaddr *)&serveur, sizeof(struct sockaddr_in)) == -1){
    perror("connect failed\n");
    exit(1);
  }

  /* Il faut indiquer son rang au processus 0 */
  write(send_sock[0], &pm2self, sizeof(int));

  /* Etablissement des connections avec les autres */
  for(i=1; i<confsize-1; i++) {
    if(pm2self == i) {
      int sock, port, len, fd, n;

      port = 0;

      if((sock = creer_socket(SOCK_STREAM, port, &adresse)) == -1) {
	fprintf(stderr, "Aie...\n");
	exit(1);
      }

      if(listen(sock, MAX_MODULES) == -1) {
	perror("slave'listen\n");
	exit(1);
      }

      len = strlen(my_name)+1;
      port = ntohs(adresse.sin_port);
      write(send_sock[0], &len, sizeof(int));
      write(send_sock[0], my_name, len);
      write(send_sock[0], &port, sizeof(int));

      for(j=pm2self+1; j<confsize; j++) {
	do {
	  fd = accept(sock, NULL, NULL);
	  if(fd == -1 && errno != EINTR) {
	    perror("slave'accept");
	    exit(1);
	  }
	} while(fd == -1);
	read(fd, &n, sizeof(int));
	send_sock[n] = fd;
      }

      close(sock);

    } else if(pm2self > i) {
      int len, num, port;

      read(send_sock[0], &len, sizeof(int));
      read(send_sock[0], hostname, len);
      read(send_sock[0], &port, sizeof(int));
      read(send_sock[0], &num, sizeof(int));

      if((host = gethostbyname(hostname)) == NULL) {
	fprintf(stderr, "ERROR: Cannot find internet address of %s\n", hostname);
	exit(1);
      }

      if((send_sock[num] = creer_socket(SOCK_STREAM, 0, &adresse)) == -1) {
	fprintf(stderr, "Aie...\n");
	exit(1);
      }

      serveur.sin_family = AF_INET;
      serveur.sin_port = htons(port);
      memcpy(&serveur.sin_addr.s_addr, host->h_addr, host->h_length);

      if(connect(send_sock[num], (struct sockaddr *)&serveur, sizeof(struct sockaddr_in)) == -1){
	perror("connect failed\n");
	exit(1);
      }

      /* On indique son rang au processus */
      write(send_sock[num], &pm2self, sizeof(int));
    }
  }

  /* envoi de l'accusé de réception, et attente de confirmation */
  write(send_sock[0], &j, sizeof(int));
  read(send_sock[0], &j, sizeof(int));

  LOG_OUT();
}

void mad_tcp_network_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int i, f;
  char output[64];
  int un = 1, packet = 0x8000;
  struct linger ling = { 1, 50 };
  char *_argv[128]; /* sauvegarde */

  LOG_IN();

  if(getenv("MAD_MY_NUMBER") == NULL) { /* master process */

    pm2self = 0;

    setbuf(stdout, NULL);
    dup2(STDERR_FILENO, STDOUT_FILENO);

    {
      int ret = system("exit `cat ${PM2_CONF_FILE} | wc -w`");

      confsize = WEXITSTATUS(ret);
    }

    {
      FILE *f;

      sprintf(output, "%s", getenv("PM2_CONF_FILE"));

      f = fopen(output, "r");
      if(f == NULL) {
	perror(output);
	exit(1);
      }


      fscanf(f, "%s", my_name); /* first line of .madconf */
      fclose(f);

    }

    if(nb_proc == NETWORK_ASK_USER) {
      do {
	printf("Config. size (1-%d or %d for one proc. per node) : ", MAX_MODULES, NETWORK_ONE_PER_NODE);
	scanf("%d", &nb_proc);
      } while((nb_proc < 0 || nb_proc > MAX_MODULES) && nb_proc != NETWORK_ONE_PER_NODE);
    }

    if(nb_proc != NETWORK_ONE_PER_NODE)
      confsize = nb_proc;

      memcpy(_argv, argv, (*argc+1)*sizeof(char *));
      parse_cmd_line(argc, argv);

      if(strcmp(cons_image, "no"))
	confsize++;

    mdebug("Process %d started on %s\n", pm2self, my_name);

    if(confsize > 1)
       master(_argv);

  } else { /* slave process */

    parse_cmd_line(argc, argv);

    pm2self = atoi(getenv("MAD_MY_NUMBER"));
    confsize = atoi(getenv("MAD_NUM_NODES"));
    strcpy(my_name, getenv("MAD_MY_NAME"));

    if(getenv("MAD_CONSOLE") == NULL) {
#ifdef PM2
      sprintf(output, "/tmp/%s-pm2log-%d", getenv("USER"), pm2self);
#else
      sprintf(output, "/tmp/%s-madlog-%d", getenv("USER"), pm2self);
#endif
      fflush(stdout);
      setbuf(stdout, NULL);
      dup2((f = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0600)),
	   STDOUT_FILENO);
      dup2(STDOUT_FILENO, STDERR_FILENO);
      close(f);
    }

#ifdef DEBUG
    fprintf(stderr, "Process %d started on %s\n", pm2self, my_name);
#endif

    slave(atoi(getenv("MAD_SERVER_PORT")), getenv("MAD_SERVER_HOST"));
  }

  pipe(private_pipe);
  send_sock[pm2self] = private_pipe[0]; /* uniformisation de la réception */

  for(i=0; i<confsize; i++) {
#if defined(PM2) && !defined(USE_BLOCKING_IO)
    if(fcntl(send_sock[i], F_SETFL, O_NONBLOCK) < 0) {
      perror("fcntl");
      exit(1);
    }
#endif
    if(i != pm2self) {
       if(setsockopt(send_sock[i], IPPROTO_TCP, TCP_NODELAY,
		     (char *)&un, sizeof(int)) == -1) {
	 perror("setsockopt failed");
	 exit(1);
       }
       if(setsockopt(send_sock[i], SOL_SOCKET, SO_LINGER,
		     (char *)&ling, sizeof(struct linger)) == -1) {
	 perror("setsockopt failed");
	 exit(1);
       }
       if(setsockopt(send_sock[i], SOL_SOCKET, SO_SNDBUF,
		     (char *)&packet, sizeof(int)) == -1) {
	 perror("setsockopt failed");
	 exit(1);
       }
       if(setsockopt(send_sock[i], SOL_SOCKET, SO_RCVBUF,
		     (char *)&packet, sizeof(int)) == -1) {
	 perror("setsockopt failed");
	 exit(1);
       }
    }
  }

#ifdef PM2
  for(i=0; i<confsize; i++)
    marcel_mutex_init(&mutex[i], NULL);
#endif

  FD_ZERO(&read_fds);
  nb_fds = 0;
  for(i=0; i<confsize; i++) {
    FD_SET(send_sock[i], &read_fds);
    nb_fds = max(nb_fds, send_sock[i]);
  }

  if(strcmp(cons_image, "no"))
    *nb = confsize - 1;
  else
    *nb = confsize;

  *whoami = pm2self;

  if(tids != NULL)
    for(i=0; i<*nb; i++)
      tids[i] = i;

  LOG_OUT();
}


void mad_tcp_network_send(int dest_node, struct iovec *vector, size_t count)
{
  int fd;
  ssize_t len;
  struct iovec backup;

  LOG_IN();

  TIMING_EVENT("network_send'begin");

#ifdef PM2
  marcel_mutex_lock(&mutex[dest_node]);
#endif

  if(dest_node == pm2self)
    fd = private_pipe[1];
  else
    fd = send_sock[dest_node];

  /* On utilise la zone de piggyback : */
  ((long*)(vector[0].iov_base))[0] = vector[0].iov_len;

  backup = vector[0];

  TIMING_EVENT("writev_section'begin");

  do {

    len = writev(fd, vector, min(MAX_IOV, count));

    if(len == -1) {
#ifdef DEBUG
      if(errno != EAGAIN)
	perror("network_send'writev\n");
#endif
    } else {
      do {

	if(len >= vector[0].iov_len) {
	  len -= vector[0].iov_len;
	  vector[0] = backup;
	  vector++;
	  if(--count)
	    backup = vector[0];

	  mdebug("One more iovec sent.\n");

	} else {
	  vector[0].iov_len -= len;
	  ((char *)vector[0].iov_base) += len;
	  len = 0;
	}

      } while(len > 0);
    }

#ifdef PM2
    if(count) {
      marcel_givehandback();
    }
#endif

  } while(count != 0);

  TIMING_EVENT("writev_section'end");

#ifdef PM2
  marcel_mutex_unlock(&mutex[dest_node]);
#endif

  TIMING_EVENT("network_send'end");

  LOG_OUT();
}

static int current_expeditor;

void mad_tcp_network_receive(char **head)
{ 
  int n, i;
  fd_set rfds;

  LOG_IN();

  do {
    do {
      rfds = read_fds;
#ifdef PM2

#if defined(USE_BLOCKING_IO) && defined(MARCEL_ACT)
      n = select(nb_fds+1, &rfds, NULL, NULL, NULL);
#elif defined(USE_MARCEL_POLL)
      n = marcel_select(nb_fds+1, &rfds, NULL);
#else
      n = tselect(nb_fds+1, &rfds, NULL, NULL);
      if(n <= 0)
	marcel_givehandback();
#endif

#else
      n = select(nb_fds+1, &rfds, NULL, NULL, NULL);
#endif
    } while(n <= 0);

    for(i=0; i<confsize; i++) {
      if(FD_ISSET(send_sock[i], &rfds)) {

	n = read(send_sock[i], header, sizeof(long));

	if(n == -1) {
#ifdef DEBUG
	  if(errno != EAGAIN)
	    perror("network_receive'read");
#endif
#ifdef PM2
	  marcel_givehandback();
#endif
	  n = 0;
	} else {
	  if(n == 0) { /* fermeture de socket détectée */

	    mdebug("connexion coupée avec le processus %d\n", i);

	    FD_CLR(send_sock[i], &read_fds);
	    close(send_sock[i]);
	  }
	  break;
	}
      }
    }
  } while(n == 0);

  current_expeditor = i;

  {
    char *ptr = (char *)(header + 1);
    register size_t to_send = header[0] - sizeof(long);
    ssize_t len;

    do {

      len = read(send_sock[i], ptr, to_send);

#ifdef DEBUG
      if(len == -1 && errno != EAGAIN)
	perror("network_receive_data'read");
#endif
      if(len > 0) {
	ptr += len;
	to_send -= len;
      }
#ifdef PM2
      if(to_send) {
	marcel_givehandback();
      }
#endif
    } while(to_send);
  }

  mdebug("One more iovec received\n");

  *head = (char *)header;

  LOG_OUT();
}

void mad_tcp_network_receive_data(struct iovec *vector, size_t count)
{
  register ssize_t len;

  LOG_IN();

  if(!count) {
    LOG_OUT();
    return;
  }

  do {

    len = readv(send_sock[current_expeditor], vector, min(MAX_IOV, count));

    if(len < 0) {
#ifdef DEBUG
      if(errno != EAGAIN)
	perror("readv");
#endif
    } else {
      do {
	if(len >= vector[0].iov_len) {
	  len -= vector[0].iov_len;
	  vector++;
	  count--;

	  mdebug("One more iovec received\n");

	} else {
	  vector[0].iov_len -= len;
	  ((char *)vector[0].iov_base) += len;
	  len = 0;
	}
      } while(len > 0);
    }
#ifdef PM2
    if(count) {
      marcel_givehandback();
    }
#endif
  } while(count != 0);

  LOG_OUT();
}

void mad_tcp_network_exit()
{
  int i;

  LOG_IN();

  for(i=0; i<confsize; i++)
    close(send_sock[i]);
  close(private_pipe[1]);

  LOG_OUT();

}

static netinterf_t mad_tcp_netinterf = {
  mad_tcp_network_init,
  mad_tcp_network_send,
  mad_tcp_network_receive,
  mad_tcp_network_receive_data,
  mad_tcp_network_exit
};

netinterf_t *mad_tcp_netinterf_init()
{
  return &mad_tcp_netinterf;
}

