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

/* JUST FOR TESTING!! DO NOT USE!! */
/* #define WITHOUT_ACK */

#ifdef PM2
#include <marcel.h>
#endif

#include <sys/netinterf.h>
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
#include <assert.h>

#include <vipl.h>
#include <safe_malloc.h>
#include <mad_timing.h>

/* #define DEBUG */

#ifdef DEBUG
#define VERIFY(op, val) \
  if((op) != (val)) \
    fprintf(stderr, "ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__), \
    exit(1)
#else
#define VERIFY(op, val) ((void)(op))
#endif

#define DESC_SIZE    (sizeof(VIP_DESCRIPTOR) + MAX_IOVECS * sizeof(VIP_DATA_SEGMENT))

#define ALIGNED(x, a)   (((unsigned)(x) + (a) - 1) & ~(unsigned)((a) - 1))

#define ALIGNED_32(x)   ALIGNED((x), 32)
#define ALIGNED_DESC(x) ALIGNED((x), VIP_DESCRIPTOR_ALIGNMENT)

static long *header[MAX_MODULES];
static VIP_DESCRIPTOR *recv_desc[MAX_MODULES][3];
static int lprd[MAX_MODULES]; /* Last Posted Receive Descriptor */
static VIP_DESCRIPTOR *send_desc[MAX_MODULES][2];

static VIP_MEM_HANDLE headHand[MAX_MODULES], 
                      recvHand[MAX_MODULES][3],
                      sendHand[MAX_MODULES][2];

static VIP_MEM_HANDLE _send_hand;

#ifdef PM2
static marcel_sem_t mutex[MAX_MODULES];
#else
#define lock_task();
#define unlock_task();
#endif

static int send_sock[MAX_MODULES];
static int pm2self, confsize;

static char my_name[128];

#define min(a, b)  ((a) < (b) ? (a) : (b))
#define max(a, b)  ((a) > (b) ? (a) : (b))

typedef enum { NO_CONSOLE, PM2_CONSOLE, XPM2_CONSOLE } console_t;

static char *cons_image[] = { "no", "pm2", "xpm2" };

static console_t cons = NO_CONSOLE;

static VIP_NIC_HANDLE        nicHand;
static VIP_NIC_ATTRIBUTES    nicAttrs;
static VIP_NET_ADDRESS	     *localAddr;
static VIP_NET_ADDRESS	     *remoteAddr;
static VIP_MEM_ATTRIBUTES    memAttrs;
static VIP_PROTECTION_HANDLE ptag;
static VIP_VI_HANDLE	     viRecvHand[MAX_MODULES], viSendHand[MAX_MODULES];
static VIP_VI_ATTRIBUTES     viAttrs;
static VIP_CQ_HANDLE         cqHand;
static VIP_DESCRIPTOR        *already_received[MAX_MODULES];

/* Aligned Malloc */
char *amalloc(int size, int align)
{
    char *ptr, *ini;
    unsigned mask = align - 1;

    ini = ptr = MALLOC(size + 2 * align - 1);
    if(ptr != NULL && ((unsigned)ptr & mask) != 0) {
	ptr = (char *) (((unsigned)ptr + mask) & ~mask);
    }

    *(char **)ptr = ini;
    ptr += align;

    return ptr;
}

void afree(void *ptr, int align)
{
  FREE(*(char **)((char *)ptr - align));
}

static void parse_cmd_line(int *argc, char **argv)
{
  int i, j;

  i = j = 1;
  while(i<*argc) {
    if(!strcmp(argv[i], "-pm2cons")) {
      if(i == *argc-1) {
	fprintf(stderr, "Fatal error: -pm2cons option must be followed by either pm2 or xpm2.\n");
	exit(1);
      }
      if(!strcmp(argv[i+1], "pm2"))
	cons = PM2_CONSOLE;
      else if(!strcmp(argv[i+1], "xpm2"))
	cons = XPM2_CONSOLE;
      else {
	fprintf(stderr, "Fatal error: -pm2cons option must be followed by either pm2 or xpm2 (found %s).\n", argv[i+1]);
	exit(1);
      }
      i++;
    } else {
      argv[j++] = argv[i];
    }
    i++;
  }
  *argc = j;
}

static void spawn_procs(char *argv[], int port)
{ 
  char cmd[1024], arg[128];
  int i;

  sprintf(cmd, "%s/bin/via/madspawn %s %d %d %s",
	  getenv("MADELEINE_ROOT"), my_name, confsize, port, cons_image[cons]);
  i=0;
  while(argv[i]) {
    sprintf(arg, " %s ", argv[i]);
    strcat(cmd, arg);
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

static void init_headers()
{
  int i, j;

  for(i=0; i<confsize; i++) {

    already_received[i] = NULL;

    header[i] = (long *)amalloc(MAX_HEADER, 32);
    VERIFY(VipRegisterMem(nicHand, header[i], MAX_HEADER, &memAttrs, &headHand[i]), VIP_SUCCESS);
  
    for(j=0; j<3; j++) {
      recv_desc[i][j] = (VIP_DESCRIPTOR *)amalloc(DESC_SIZE, VIP_DESCRIPTOR_ALIGNMENT);
      VERIFY(VipRegisterMem(nicHand, recv_desc[i][j], DESC_SIZE, &memAttrs, &recvHand[i][j]),
	     VIP_SUCCESS);
    }
    for(j=0; j<2; j++) {
      send_desc[i][j] = (VIP_DESCRIPTOR *)amalloc(DESC_SIZE, VIP_DESCRIPTOR_ALIGNMENT);
      VERIFY(VipRegisterMem(nicHand, send_desc[i][j], DESC_SIZE, &memAttrs, &sendHand[i][j]),
	     VIP_SUCCESS);
    }
  }
}

void mad_via_register_send_buffers(char *area, unsigned len)
{
  VERIFY(VipRegisterMem(nicHand, area, len, &memAttrs, &_send_hand), VIP_SUCCESS);
#ifdef DEBUG
  fprintf(stderr, "Send buffers at %p, len %d.\n", area, len);
#endif
}

void accepter_connexion_avec(char *machine, int rang)
{
  VIP_VI_ATTRIBUTES remoteViAttrs;
  VIP_CONN_HANDLE connHand;
  VIP_DESCRIPTOR *descp;

  VERIFY(VipCreateVi(nicHand, &viAttrs, NULL, cqHand, &viRecvHand[rang]), VIP_SUCCESS);

  VERIFY(VipCreateVi(nicHand, &viAttrs, NULL, NULL, &viSendHand[rang]), VIP_SUCCESS);

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  VERIFY(VipNSGetHostByName(nicHand, machine, remoteAddr, 0), VIP_SUCCESS);

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  remoteAddr->DiscriminatorLen = sizeof(int);

  *(int *)(localAddr->HostAddress + localAddr->HostAddressLen) = rang << 1;
  *(int *)(remoteAddr->HostAddress + remoteAddr->HostAddressLen) = (pm2self << 1) + 1;

  lprd[rang] = 0;
  descp = recv_desc[rang][lprd[rang]];

  descp->CS.Length = MAX_HEADER;
  descp->CS.Status = 0;
  descp->CS.Control = VIP_CONTROL_OP_SENDRECV;
  descp->CS.SegCount = 1;
  descp->DS[0].Local.Data.Address = header[rang];
  descp->DS[0].Local.Handle = headHand[rang];
  descp->DS[0].Local.Length = MAX_HEADER;

  VERIFY(VipPostRecv(viRecvHand[rang], descp, recvHand[rang][lprd[rang]]), VIP_SUCCESS);

  VERIFY(VipConnectWait(nicHand, localAddr, VIP_INFINITE, remoteAddr, &remoteViAttrs,
			&connHand),
	 VIP_SUCCESS);

  VERIFY(VipConnectAccept(connHand, viRecvHand[rang]), VIP_SUCCESS);

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  VERIFY(VipNSGetHostByName(nicHand, machine, remoteAddr, 0), VIP_SUCCESS);

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  remoteAddr->DiscriminatorLen = sizeof(int);

  *(int *)(localAddr->HostAddress + localAddr->HostAddressLen) = (rang << 1) + 1;
  *(int *)(remoteAddr->HostAddress + remoteAddr->HostAddressLen) = pm2self << 1;

  VERIFY(VipConnectWait(nicHand, localAddr, VIP_INFINITE, remoteAddr, &remoteViAttrs,
			&connHand),
	 VIP_SUCCESS);

  VERIFY(VipConnectAccept(connHand, viSendHand[rang]), VIP_SUCCESS);
}

void demander_connexion_avec(char *machine, int rang)
{
  VIP_VI_ATTRIBUTES remoteViAttrs;
  VIP_DESCRIPTOR *descp;

  sleep(1);

  VERIFY(VipCreateVi(nicHand, &viAttrs, NULL, cqHand, &viRecvHand[rang]), VIP_SUCCESS);

  VERIFY(VipCreateVi(nicHand, &viAttrs, NULL, NULL, &viSendHand[rang]), VIP_SUCCESS);

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  VERIFY(VipNSGetHostByName(nicHand, machine, remoteAddr, 0), VIP_SUCCESS);

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  remoteAddr->DiscriminatorLen = sizeof(int);
	
  *(int *)(localAddr->HostAddress + localAddr->HostAddressLen) = (rang << 1) + 1;
  *(int *)(remoteAddr->HostAddress + remoteAddr->HostAddressLen) = pm2self << 1;

  lprd[rang] = 0;
  descp = recv_desc[rang][lprd[rang]];

  descp->CS.Length = MAX_HEADER;
  descp->CS.Status = 0;
  descp->CS.Control = VIP_CONTROL_OP_SENDRECV;
  descp->CS.SegCount = 1;
  descp->DS[0].Local.Data.Address = header[rang];
  descp->DS[0].Local.Handle = headHand[rang];
  descp->DS[0].Local.Length = MAX_HEADER;

  VERIFY(VipPostRecv(viRecvHand[rang], descp, recvHand[rang][lprd[rang]]), VIP_SUCCESS);

  VERIFY(VipConnectRequest(viSendHand[rang], localAddr, remoteAddr, VIP_INFINITE,
			   &remoteViAttrs),
	 VIP_SUCCESS);

  sleep(1);

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  VERIFY(VipNSGetHostByName(nicHand, machine, remoteAddr, 0), VIP_SUCCESS);

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  remoteAddr->DiscriminatorLen = sizeof(int);
	
  *(int *)(localAddr->HostAddress + localAddr->HostAddressLen) = rang << 1;
  *(int *)(remoteAddr->HostAddress + remoteAddr->HostAddressLen) = (pm2self << 1) + 1;

  VERIFY(VipConnectRequest(viRecvHand[rang], localAddr, remoteAddr, VIP_INFINITE,
		       &remoteViAttrs), VIP_SUCCESS);
}

void initialiser_via(void)
{
  int i, j;
  char machine[128];

  fprintf(stderr, "Initialisation de VIA (machine <%s>, rang <%d>)...\n", my_name, pm2self);

  init_headers();

  VERIFY(VipCreateCQ(nicHand, MAX_MODULES*2, &cqHand), VIP_SUCCESS);

  for(i=0; i<confsize; i++)
    for(j=i+1; j<confsize; j++)
      if(pm2self == i) {
	write(send_sock[j], my_name, 128);
	read(send_sock[j], machine, 128);
	accepter_connexion_avec(machine, j);
      } else if(pm2self == j) {
	read(send_sock[i], machine, 128);
	write(send_sock[i], my_name, 128);
	demander_connexion_avec(machine, i);
      }

  fprintf(stderr, "Initialisation de VIA terminée.\n");
}

static void master(char *argv[])
{
  int sock;
  struct sockaddr_in adresse;
  int port;
  char hostname[64], execname[1024];

  port = 0;

  if((sock = creer_socket(SOCK_STREAM, port, &adresse)) == -1)
    exit(1);

  if(listen(sock, MAX_MODULES) == -1) {
    perror("ERROR: listen failed\n");
    exit(1);
  }

  if(argv[0][0] != '/') {
    getcwd(execname, 1024);
    strcat(execname, "/");
    strcat(execname, argv[0]);
    argv[0] = execname;
    if(strlen(execname) >= 1024)
      fprintf(stderr, "master: Ouille !!!\n");
  }

  spawn_procs(argv, ntohs(adresse.sin_port));

  {
    int i, n, fd;

    for(i=1; i<confsize; i++) {
      fd = accept(sock, NULL, NULL);
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
}

static void slave(int serverport, char *serverhost)
{
  struct sockaddr_in adresse, serveur;
  struct hostent *host;
  char hostname[64];
  int i, j;

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
	perror("ERROR: listen failed\n");
	exit(1);
      }

      len = strlen(my_name)+1;
      port = ntohs(adresse.sin_port);
      write(send_sock[0], &len, sizeof(int));
      write(send_sock[0], my_name, len);
      write(send_sock[0], &port, sizeof(int));

      for(j=pm2self+1; j<confsize; j++) {
	fd = accept(sock, NULL, NULL);
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
}

void mad_via_network_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int i, f;
  char output[64];
  char *_argv[128]; /* sauvegarde */

  VERIFY(VipOpenNic(VIA_DEV_NAME, &nicHand), VIP_SUCCESS);

  VERIFY(VipNSInit(nicHand, NULL), VIP_SUCCESS);

  VERIFY(VipQueryNic(nicHand, &nicAttrs), VIP_SUCCESS);

  localAddr = MALLOC(sizeof(VIP_NET_ADDRESS)+nicAttrs.NicAddressLen+sizeof(int));
  localAddr->HostAddressLen = nicAttrs.NicAddressLen;
    
  localAddr->DiscriminatorLen = sizeof(int);
  memcpy(localAddr->HostAddress,
	 nicAttrs.LocalNicAddress, nicAttrs.NicAddressLen);

  remoteAddr = MALLOC(sizeof(VIP_NET_ADDRESS)+nicAttrs.NicAddressLen+sizeof(int));
  memset(remoteAddr, 0, sizeof(VIP_NET_ADDRESS)+nicAttrs.NicAddressLen+sizeof(int));

  VERIFY(VipCreatePtag(nicHand, &ptag), VIP_SUCCESS);

  memAttrs.Ptag = ptag;
  memAttrs.EnableRdmaWrite = VIP_TRUE;
  memAttrs.EnableRdmaRead = VIP_FALSE;

  viAttrs.ReliabilityLevel = VIP_SERVICE_UNRELIABLE;
  viAttrs.Ptag = ptag;
  viAttrs.EnableRdmaWrite = VIP_TRUE;
  viAttrs.EnableRdmaRead = VIP_FALSE;
  viAttrs.QoS = 0;
  viAttrs.MaxTransferSize = 32000;

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
	printf("Config. size (1-%d or %d for one proc. per node) : ",
	       MAX_MODULES, NETWORK_ONE_PER_NODE);
	scanf("%d", &nb_proc);
      } while((nb_proc < 0 || nb_proc > MAX_MODULES) && nb_proc != NETWORK_ONE_PER_NODE);
    }

    if(nb_proc != NETWORK_ONE_PER_NODE)
      confsize = nb_proc;

      memcpy(_argv, argv, (*argc+1)*sizeof(char *));
      parse_cmd_line(argc, argv);

      if(cons != NO_CONSOLE)
	confsize++;

#ifdef DEBUG
    fprintf(stderr, "Process %d started on %s\n", pm2self, my_name);
#endif

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
      dup2((f = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0600)), STDOUT_FILENO);
      dup2(STDOUT_FILENO, STDERR_FILENO);
      close(f);
    }

#ifdef DEBUG
    fprintf(stderr, "Process %d started on %s\n", pm2self, my_name);
#endif

    slave(atoi(getenv("MAD_SERVER_PORT")), getenv("MAD_SERVER_HOST"));
  }

  initialiser_via();

  for(i=0; i<confsize; i++)
    if(i != pm2self)
      close(send_sock[i]);

#ifdef PM2
  for(i=0; i<confsize; i++)
    marcel_sem_init(&mutex[i], 1);
#endif

  if(cons != NO_CONSOLE)
    *nb = confsize - 1;
  else
    *nb = confsize;

  *whoami = pm2self;
  for(i=0; i<*nb; i++)
    tids[i] = i;
}


void mad_via_network_send(int dest_node, struct iovec *vector, size_t count)
{
  VIP_MEM_HANDLE vecHand[MAX_IOVECS];
  VIP_DESCRIPTOR *descp;
  int i;
#ifdef PM2
  int r;
#endif

  TIMING_EVENT("network_send'begin");

#ifdef PM2
  marcel_sem_P(&mutex[dest_node]);
#endif

  lock_task();

#ifdef WITHOUT_ACK
  if(count > 1) {
#endif

  /* On va forcément recevoir un Ack, donc on poste un nouveau descripteur... */
  descp = recv_desc[dest_node][2];

  descp->CS.Length = 0;
  descp->CS.Status = 0;
  descp->CS.Control = VIP_CONTROL_OP_SENDRECV;
  descp->CS.SegCount = 0;

  VERIFY(VipPostRecv(viSendHand[dest_node], descp, recvHand[dest_node][2]), VIP_SUCCESS);

#ifdef WITHOUT_ACK
  }
#endif

  /* Preparation et émission du Header */

  send_desc[dest_node][1]->CS.Length = vector[0].iov_len;
  send_desc[dest_node][1]->CS.Status = 0;
  send_desc[dest_node][1]->CS.Control = VIP_CONTROL_OP_SENDRECV | VIP_CONTROL_IMMEDIATE;
  send_desc[dest_node][1]->CS.SegCount = 1;
  send_desc[dest_node][1]->CS.ImmediateData = 0xdeadbeef;
  send_desc[dest_node][1]->DS[0].Local.Data.Address = vector[0].iov_base;
  send_desc[dest_node][1]->DS[0].Local.Handle = _send_hand;
  send_desc[dest_node][1]->DS[0].Local.Length = vector[0].iov_len;

  VERIFY(VipPostSend(viSendHand[dest_node], send_desc[dest_node][1],
		     sendHand[dest_node][1]),
	 VIP_SUCCESS);

#ifdef PM2
  do {
    r = VipSendDone(viSendHand[dest_node], &descp);
    if(r != VIP_SUCCESS) {
      unlock_task();
      marcel_givehandback();
      lock_task();
    }
  } while(r != VIP_SUCCESS);
#else
  VERIFY(VipSendWait(viSendHand[dest_node], VIP_INFINITE, &descp), VIP_SUCCESS);
#endif

#ifdef DEBUG
  fprintf(stderr, "Le header est envoyé (taille = %d).\n", vector[0].iov_len);
#endif

#ifdef WITHOUT_ACK
  if(count > 1) {
#endif

  /* Attente de l'Ack */
#ifdef PM2
  do {
    r = VipRecvDone(viSendHand[dest_node], &descp);
    if(r != VIP_SUCCESS) {
      unlock_task();
      marcel_givehandback();
      lock_task();
    }
  } while(r != VIP_SUCCESS);
#else
  VERIFY(VipRecvWait(viSendHand[dest_node], VIP_INFINITE, &descp), VIP_SUCCESS);
#endif

#ifdef DEBUG
  fprintf(stderr, "J'ai reçu l'accusé de réception.\n");
#endif

#ifdef WITHOUT_ACK
  }
#endif

  if(count > 1) {
    send_desc[dest_node][1]->CS.Length = 0;
    send_desc[dest_node][1]->CS.Status = 0;
    send_desc[dest_node][1]->CS.Control = VIP_CONTROL_OP_SENDRECV | VIP_CONTROL_IMMEDIATE;
    send_desc[dest_node][1]->CS.SegCount = count-1;
    send_desc[dest_node][1]->CS.ImmediateData = 0xdeadbeef;
    
    for(i=1; i<count; i++) {
      if(VipRegisterMem(nicHand, vector[i].iov_base, vector[i].iov_len,
			&memAttrs, &vecHand[i]) != VIP_SUCCESS)
	fprintf(stderr, "Failed to register message\n");
      send_desc[dest_node][1]->DS[i-1].Local.Data.Address = vector[i].iov_base;
      send_desc[dest_node][1]->DS[i-1].Local.Handle = vecHand[i];
      send_desc[dest_node][1]->DS[i-1].Local.Length = vector[i].iov_len;
      send_desc[dest_node][1]->CS.Length += vector[i].iov_len;
    }

    /* Emission du Body */
    VERIFY(VipPostSend(viSendHand[dest_node], send_desc[dest_node][1],
		       sendHand[dest_node][1]),
	   VIP_SUCCESS);

#ifdef PM2
    do {
      r = VipSendDone(viSendHand[dest_node], &descp);
      if(r != VIP_SUCCESS) {
	unlock_task();
	marcel_givehandback();
	lock_task();
      }
    } while(r != VIP_SUCCESS);
#else
    VERIFY(VipSendWait(viSendHand[dest_node], VIP_INFINITE, &descp), VIP_SUCCESS);
#endif

    for(i=1; i<count; i++)
      VERIFY(VipDeregisterMem(nicHand, vector[i].iov_base, vecHand[i]), VIP_SUCCESS);

#ifdef DEBUG
    fprintf(stderr, "Le body est envoyé.\n");
#endif
  }

  unlock_task();

#ifdef PM2
  marcel_sem_V(&mutex[dest_node]);
#endif

  TIMING_EVENT("network_send'end");
}

static int current_expeditor;

void mad_via_network_receive(network_handler func)
{ 
  VIP_DESCRIPTOR *descp;
  VIP_BOOLEAN isRecv;
  VIP_VI_HANDLE vih;
  int i;
#ifdef PM2
  int r;
#endif

#ifdef DEBUG
  fprintf(stderr, "J'attends un header...\n");
#endif

  lock_task();

  current_expeditor = -1;

  for(i=0; i<confsize; i++)
    if(already_received[i] != NULL) {
      current_expeditor = i;
      descp = already_received[i];
      already_received[i] = NULL;
      break;
    }

  if(current_expeditor == -1) {

#ifdef PM2
    do {
      r = VipCQDone(cqHand, &vih, &isRecv);
      if(r != VIP_SUCCESS) {
	unlock_task();
	marcel_givehandback();
	lock_task();
      }
    } while(r != VIP_SUCCESS);
#else
    VERIFY(VipCQWait(cqHand, VIP_INFINITE, &vih, &isRecv), VIP_SUCCESS);
#endif

    for(current_expeditor=0; vih != viRecvHand[current_expeditor]; current_expeditor++) ;

#ifdef DEBUG
    fprintf(stderr, "Expeditor is : %d\n", current_expeditor);
#endif

    VERIFY(VipRecvDone(vih, &descp), VIP_SUCCESS);
  }

#ifdef DEBUG
  fprintf(stderr, "Header recu (taille = %d)...\n", descp->CS.Length);
#endif

  unlock_task();

  (*func)(header[current_expeditor], descp->CS.Length);

  if(current_expeditor != -1) { /* pas de body */

    lock_task();

#ifdef DEBUG
    fprintf(stderr, "Pas de body, donc je reposte un descripteur pour le header...\n");
#endif
    /* On reposte un Header */
    descp = recv_desc[current_expeditor][lprd[current_expeditor]];
    descp->CS.Length = MAX_HEADER;
    descp->CS.Status = 0;
    descp->CS.Control = VIP_CONTROL_OP_SENDRECV;
    descp->CS.SegCount = 1;
    descp->DS[0].Local.Data.Address = header[current_expeditor];
    descp->DS[0].Local.Handle = headHand[current_expeditor];
    descp->DS[0].Local.Length = MAX_HEADER;
 
    VERIFY(VipPostRecv(viRecvHand[current_expeditor], descp,
		       recvHand[current_expeditor][current_expeditor]),
	   VIP_SUCCESS);

#ifndef WITHOUT_ACK

    /* Et on n'oublie pas d'envoyer un Ack... */
    send_desc[current_expeditor][0]->CS.Length = 0;
    send_desc[current_expeditor][0]->CS.Status = 0;
    send_desc[current_expeditor][0]->CS.Control = VIP_CONTROL_OP_SENDRECV | VIP_CONTROL_IMMEDIATE;
    send_desc[current_expeditor][0]->CS.SegCount = 0;
    send_desc[current_expeditor][0]->CS.ImmediateData = 0xdeadbeef;
    
    VERIFY(VipPostSend(viRecvHand[current_expeditor], send_desc[current_expeditor][0], 
		       sendHand[current_expeditor][0]),
	   VIP_SUCCESS);

#ifdef PM2
    do {
      r = VipSendDone(viRecvHand[current_expeditor], &descp);
      if(r != VIP_SUCCESS) {
	unlock_task();
	marcel_givehandback();
	lock_task();
      }
    } while(r != VIP_SUCCESS);
#else
    VERIFY(VipSendWait(viRecvHand[current_expeditor], VIP_INFINITE, &descp), VIP_SUCCESS);
#endif

#endif

    unlock_task();
  }
}

void mad_via_network_receive_data(struct iovec *vector, size_t count)
{
  VIP_MEM_HANDLE vecHand[MAX_IOVECS];
  VIP_DESCRIPTOR *descp;
  VIP_BOOLEAN isRecv;
  VIP_VI_HANDLE vih;
  int i, found;
#ifdef PM2
  int r;
#endif

  lock_task();

  /* On prepare la reception du Body */
  descp = recv_desc[current_expeditor][lprd[current_expeditor]];

  descp->CS.Length = 0;
  descp->CS.Status = 0;
  descp->CS.Control = VIP_CONTROL_OP_SENDRECV;
  descp->CS.SegCount = count;

  for(i=0; i<count; i++) {

    VERIFY(VipRegisterMem(nicHand, vector[i].iov_base, vector[i].iov_len,
			  &memAttrs, &vecHand[i]),
	   VIP_SUCCESS);

    descp->DS[i].Local.Data.Address = vector[i].iov_base;
    descp->DS[i].Local.Handle = vecHand[i];
    descp->DS[i].Local.Length = vector[i].iov_len;
    descp->CS.Length += vector[i].iov_len;
  }

  VERIFY(VipPostRecv(viRecvHand[current_expeditor], descp,
		     recvHand[current_expeditor][lprd[current_expeditor]]),
	 VIP_SUCCESS);

  /* ... et la reception du prochain Header */
  lprd[current_expeditor] ^= 1; /* xor */
  descp = recv_desc[current_expeditor][lprd[current_expeditor]];

  descp->CS.Length = MAX_HEADER;
  descp->CS.Status = 0;
  descp->CS.Control = VIP_CONTROL_OP_SENDRECV;
  descp->CS.SegCount = 1;
  descp->DS[0].Local.Data.Address = header[current_expeditor];
  descp->DS[0].Local.Handle = headHand[current_expeditor];
  descp->DS[0].Local.Length = MAX_HEADER;

  VERIFY(VipPostRecv(viRecvHand[current_expeditor], descp,
		     recvHand[current_expeditor][lprd[current_expeditor]]),
	 VIP_SUCCESS);

  /* On envoie le Ack à l'émetteur */
  send_desc[current_expeditor][0]->CS.Length = 0;
  send_desc[current_expeditor][0]->CS.Status = 0;
  send_desc[current_expeditor][0]->CS.Control = VIP_CONTROL_OP_SENDRECV | VIP_CONTROL_IMMEDIATE;
  send_desc[current_expeditor][0]->CS.SegCount = 0;
  send_desc[current_expeditor][0]->CS.ImmediateData = 0xdeadbeef;

  VERIFY(VipPostSend(viRecvHand[current_expeditor], send_desc[current_expeditor][0], 
		     sendHand[current_expeditor][0]), VIP_SUCCESS);

#ifdef PM2
  do {
    r = VipSendDone(viRecvHand[current_expeditor], &descp);
    if(r != VIP_SUCCESS) {
      unlock_task();
      marcel_givehandback();
      lock_task();
    }
  } while(r != VIP_SUCCESS);
#else
  VERIFY(VipSendWait(viRecvHand[current_expeditor], VIP_INFINITE, &descp), VIP_SUCCESS);
#endif

#ifdef DEBUG
  fprintf(stderr, "Accusé envoyé...\n");
#endif

  /* Finalement, on recoit le body */
  found = 0;
  do {
    int i;

#ifdef PM2
    do {
      r = VipCQDone(cqHand, &vih, &isRecv);
      if(r != VIP_SUCCESS) {
	unlock_task();
	marcel_givehandback();
	lock_task();
      }
    } while(r != VIP_SUCCESS);
#else
    VERIFY(VipCQWait(cqHand, VIP_INFINITE, &vih, &isRecv), VIP_SUCCESS);
#endif

    VERIFY(VipRecvDone(vih, &descp), VIP_SUCCESS);

    for(i = 0; vih != viRecvHand[i]; i++) ;

    if(i != current_expeditor)
      already_received[i] = descp;
    else
      found = 1;

  } while(!found);

  for(i=0; i<count; i++)
    VERIFY(VipDeregisterMem(nicHand, vector[i].iov_base, vecHand[i]), VIP_SUCCESS);

#ifdef DEBUG
  fprintf(stderr, "Body recu\n");
#endif
  current_expeditor = -1;

  unlock_task();
}

void mad_via_network_exit()
{
  int i, j;

  FREE(localAddr);
  FREE(remoteAddr);

  for(i=0; i<confsize; i++) {
    if(pm2self != i) {
      VERIFY(VipDisconnect(viRecvHand[i]), VIP_SUCCESS);
      VERIFY(VipDisconnect(viSendHand[i]), VIP_SUCCESS);

      VERIFY(VipDestroyVi(viRecvHand[i]), VIP_SUCCESS);
      VERIFY(VipDestroyVi(viSendHand[i]), VIP_SUCCESS);
    }
  }

  VERIFY(VipDestroyCQ(cqHand), VIP_SUCCESS);

  for(i=0; i<confsize; i++) {

    VERIFY(VipDeregisterMem(nicHand, header[i], headHand[i]), VIP_SUCCESS);
    afree(header[i], 32);

    for(j=0; j<2; j++) {
      VERIFY(VipDeregisterMem(nicHand, send_desc[i][j], sendHand[i][j]), VIP_SUCCESS);
      afree(send_desc[i][j], VIP_DESCRIPTOR_ALIGNMENT);
    }

    for(j=0; j<3; j++) {
      VERIFY(VipDeregisterMem(nicHand, recv_desc[i][j], recvHand[i][j]), VIP_SUCCESS);
      afree(recv_desc[i][j], VIP_DESCRIPTOR_ALIGNMENT);
    }

  }
}

static netinterf_t mad_via_netinterf = {
  mad_via_network_init,
  mad_via_network_send,
  mad_via_network_receive,
  mad_via_network_receive_data,
  mad_via_network_exit
};

netinterf_t *mad_via_netinterf_init()
{
  return &mad_via_netinterf;
}

