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
#include <assert.h>

#include <vipl.h>

#include "safe_malloc.h"
#include "mad_timing.h"

/* #define DEBUG */

#define ALWAYS_VERIFY(op, val) \
  if((op) != (val)) \
    fprintf(stderr, "ASSERTION FAILED: %s\nFILE: %s\nLINE: %d\n", \
            #op " != " #val, __FILE__, __LINE__), \
    exit(1)

#ifdef DEBUG
#define VERIFY(op, val) ALWAYS_VERIFY(op, val)
#else
#define VERIFY(op, val) ((void)(op))
#endif

#define MAX_DATA_SEGMENTS       1
#define DESC_SIZE       (sizeof(VIP_DESCRIPTOR) + MAX_DATA_SEGMENTS * sizeof(VIP_DATA_SEGMENT))

#define ALIGNED(x, a)   (((unsigned)(x) + (a) - 1) & ~(unsigned)((a) - 1))
#define ALIGNED_DESC(x) ALIGNED((x), VIP_DESCRIPTOR_ALIGNMENT)

static long header[MAX_HEADER/sizeof(long)];

#define CREDIT_MASK     0x0FF
#define TAG_DATA        0x100
#define TAG_EAT_CREDIT  0x200

/* QSIZE should be kept less than 255 */
#define QSIZE           16
#define QSIZE_MASK      15

#define BUF_SIZE        (512)

typedef char foo_desc[ALIGNED_DESC(DESC_SIZE)];

static foo_desc _rdesc[MAX_MODULES][QSIZE] __attribute__ ((aligned (VIP_DESCRIPTOR_ALIGNMENT)));
static foo_desc _sdesc[MAX_MODULES][QSIZE] __attribute__ ((aligned (VIP_DESCRIPTOR_ALIGNMENT)));

#define recv_desc(m, i) ((VIP_DESCRIPTOR *)_rdesc[m][i])
#define send_desc(m, i) ((VIP_DESCRIPTOR *)_sdesc[m][i])

static foo_desc _prefilled_rdesc __attribute__ ((aligned (VIP_DESCRIPTOR_ALIGNMENT)));

static char send_buf[MAX_MODULES][QSIZE][BUF_SIZE] __MAD_ALIGNED__;
static char recv_buf[MAX_MODULES][QSIZE][BUF_SIZE] __MAD_ALIGNED__;

typedef enum { DESC_AVAIL, DESC_POSTED } desc_state;

static unsigned csd[MAX_MODULES]; /* Current Send Descriptor */
static desc_state sds[MAX_MODULES][QSIZE]; /* Send Descriptor State */

static unsigned crd[MAX_MODULES]; /* Current Receive Descriptor */
static unsigned lrd[MAX_MODULES]; /* Last Received Descriptor */
static desc_state rds[MAX_MODULES][QSIZE]; /* Receive Descriptor State */

#define MAX_CREDITS       (QSIZE - 2)
#define CREDITS_CRITICAL  (QSIZE - 4)

static unsigned credits[MAX_MODULES]; /* Needed to send packets */
static unsigned used_credits[MAX_MODULES]; /* Credits eaten by the sender */

static unsigned send_seq_number[MAX_MODULES];
static unsigned recv_seq_number[MAX_MODULES];

#ifdef PM2
static marcel_mutex_t mutex[MAX_MODULES];
#else
#define lock_task()
#define unlock_task()
#endif

static int pm2self, confsize;

static char *my_name;

static char machines[MAX_MODULES][128];
static int nb_machines = 0;

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
static VIP_VI_HANDLE	     viHand[MAX_MODULES];
static VIP_VI_ATTRIBUTES     viAttrs;
static VIP_CQ_HANDLE         cqHand;
static VIP_MEM_HANDLE        rbufHand, rHand, sbufHand, sHand;

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

static void get_machines(void)
{
  char fname[1024];
  FILE *f;

  sprintf(fname, "%s/.madconf", get_mad_root());

  f = fopen(fname, "r");
  if(f == NULL) {
    perror("$MAD1_ROOT/.madconf");
    exit(1);
  }

  while(fscanf(f, "%s", machines[nb_machines]) == 1) {
#ifdef DEBUG
    fprintf(stderr, "machine[%d] = %s\n", nb_machines, machines[nb_machines]);
#endif
    nb_machines++;
  }

  fclose(f);
}

#define init_rdesc(descp) memcpy(descp, _prefilled_rdesc, sizeof(_prefilled_rdesc))

static void init_prefilled_rdesc()
{
  ((VIP_DESCRIPTOR *)_prefilled_rdesc)->CS.Length = BUF_SIZE;
  ((VIP_DESCRIPTOR *)_prefilled_rdesc)->CS.Status = 0;
  ((VIP_DESCRIPTOR *)_prefilled_rdesc)->CS.Control = VIP_CONTROL_OP_SENDRECV;
  ((VIP_DESCRIPTOR *)_prefilled_rdesc)->CS.SegCount = 1;
  ((VIP_DESCRIPTOR *)_prefilled_rdesc)->DS[0].Local.Handle = rbufHand;
  ((VIP_DESCRIPTOR *)_prefilled_rdesc)->DS[0].Local.Length = BUF_SIZE;
}

static char *machine_name_of_process(int p)
{
  return machines[p % nb_machines];
}

static void init_headers()
{
  int i, j;

  VERIFY(VipRegisterMem(nicHand, _sdesc, sizeof(_sdesc), &memAttrs, &sHand),  VIP_SUCCESS);

  VERIFY(VipRegisterMem(nicHand, send_buf, sizeof(send_buf), &memAttrs, &sbufHand), VIP_SUCCESS);

  VERIFY(VipRegisterMem(nicHand, _rdesc, sizeof(_rdesc), &memAttrs, &rHand), VIP_SUCCESS);

  VERIFY(VipRegisterMem(nicHand, recv_buf, sizeof(recv_buf), &memAttrs, &rbufHand), VIP_SUCCESS);

  init_prefilled_rdesc();

  for(i=0; i<confsize; i++) {

    for(j=0; j<QSIZE; j++) {
      sds[i][j] = DESC_AVAIL;
      rds[i][j] = DESC_POSTED;
    }

    csd[i] = crd[i] = lrd[i] = 0;
    credits[i] = MAX_CREDITS;
    used_credits[i] = 0;
    send_seq_number[i] = recv_seq_number[i] = 0;
  }
}

void mad_via_register_send_buffers(char *area, unsigned len)
{}

void accepter_connexion_avec(char *machine, int rang)
{
  VIP_VI_ATTRIBUTES remoteViAttrs;
  VIP_CONN_HANDLE connHand;
  VIP_DESCRIPTOR *descp;
  int i;

  VERIFY(VipCreateVi(nicHand, &viAttrs, NULL, cqHand, &viHand[rang]), VIP_SUCCESS);

#ifdef DEBUG
  fprintf(stderr, "viHand[%d] = %p\n", rang, viHand[rang]);
#endif

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  VERIFY(VipNSGetHostByName(nicHand, machine, remoteAddr, 0), VIP_SUCCESS);

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  remoteAddr->DiscriminatorLen = sizeof(int);

  *(int *)(localAddr->HostAddress + localAddr->HostAddressLen) = rang;
  *(int *)(remoteAddr->HostAddress + remoteAddr->HostAddressLen) = pm2self;

  for(i=0; i<QSIZE; i++) {

    descp = recv_desc(rang, i);

    init_rdesc(descp);
    descp->DS[0].Local.Data.Address = recv_buf[rang][i];

    VERIFY(VipPostRecv(viHand[rang], descp, rHand), VIP_SUCCESS);
  }

  VERIFY(VipConnectWait(nicHand, localAddr, VIP_INFINITE, remoteAddr, &remoteViAttrs, &connHand),
	 VIP_SUCCESS);

  VERIFY(VipConnectAccept(connHand, viHand[rang]), VIP_SUCCESS);
}

void demander_connexion_avec(char *machine, int rang)
{
  VIP_VI_ATTRIBUTES remoteViAttrs;
  VIP_DESCRIPTOR *descp;
  int i;

  VERIFY(VipCreateVi(nicHand, &viAttrs, NULL, cqHand, &viHand[rang]), VIP_SUCCESS);

#ifdef DEBUG
  fprintf(stderr, "viHand[%d] = %p\n", rang, viHand[rang]);
#endif

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  VERIFY(VipNSGetHostByName(nicHand, machine, remoteAddr, 0), VIP_SUCCESS);

  remoteAddr->HostAddressLen = nicAttrs.NicAddressLen;
  remoteAddr->DiscriminatorLen = sizeof(int);
	
  *(int *)(localAddr->HostAddress + localAddr->HostAddressLen) = rang;
  *(int *)(remoteAddr->HostAddress + remoteAddr->HostAddressLen) = pm2self;

  for(i=0; i<QSIZE; i++) {

    descp = recv_desc(rang, i);

    init_rdesc(descp);
    descp->DS[0].Local.Data.Address = recv_buf[rang][i];

    VERIFY(VipPostRecv(viHand[rang], descp, rHand), VIP_SUCCESS);
  }

  while(VipConnectRequest(viHand[rang], localAddr, remoteAddr, VIP_INFINITE, &remoteViAttrs) != 
	VIP_SUCCESS) {
    fprintf(stderr, "Waiting for process %d to come up...\n", rang);
    sleep(1);
  }
}

void initialiser_via(void)
{
  int i, j;

  fprintf(stderr, "Initialisation de VIA (machine <%s>, rang <%d>)...\n", my_name, pm2self);

  init_headers();

  VERIFY(VipCreateCQ(nicHand, MAX_MODULES*QSIZE, &cqHand), VIP_SUCCESS);

  for(i=0; i<confsize; i++)
    for(j=i+1; j<confsize; j++)
      if(pm2self == i) {
	accepter_connexion_avec(machine_name_of_process(j), j);
      } else if(pm2self == j) {
	demander_connexion_avec(machine_name_of_process(i), i);
      }

  fprintf(stderr, "Initialisation de VIA terminée.\n");
}

void mad_via_network_send(int dest_node, struct iovec *vector, size_t count)
{
  VIP_DESCRIPTOR *descp;
  char *data;
  int v, nb;
#ifdef PM2
  int r;
#else
  VIP_DESCRIPTOR *rdesc;
  VIP_BOOLEAN isRecv;
  VIP_VI_HANDLE vih;
#endif

  TIMING_EVENT("network_send: begin");

#ifdef PM2
  marcel_mutex_lock(&mutex[dest_node]);
#endif

  lock_task();

  v = 0;
  data = vector[0].iov_base;
  nb = vector[0].iov_len;

  /* On utilise la zone de piggyback : */
  ((long*)(data))[0] = nb;

  while(v < count) { /* Il reste des données à émettre */

    /* On attend éventuellement l'autorisation d'émettre */
#ifdef PM2
    while(!credits[dest_node]) {
      unlock_task();
      marcel_givehandback();
      lock_task();
    }
    credits[dest_node]--;
#else
    if(!credits[dest_node]) {
      if(rds[dest_node][crd[dest_node]] == DESC_AVAIL) {
	rdesc = recv_desc(dest_node, crd[dest_node]);
#ifdef DEBUG
	fprintf(stderr, "SEND: Le prochain paquet a déjà été reçu en provenance de %d, rdesc[%d] = %p\n", dest_node, crd[dest_node], rdesc);
#endif
      } else {
	int ex;

	do {
	  VERIFY(VipCQWait(cqHand, VIP_INFINITE, &vih, &isRecv), VIP_SUCCESS);

	  for(ex=0; vih != viHand[ex]; ex++) ;
#ifdef DEBUG
	  fprintf(stderr, "\t\tSEND: receiving packet <%d> from %d\n", recv_seq_number[ex]++, ex);
#endif
	  VERIFY(VipRecvDone(vih, &rdesc), VIP_SUCCESS);

#ifdef DEBUG
	  fprintf(stderr, "rdesc[%d] = %p\n", lrd[ex], rdesc);
#endif
	  rds[ex][lrd[ex]] = DESC_AVAIL;
	  lrd[ex] = (lrd[ex] + 1) & QSIZE_MASK;

	} while(ex != dest_node);
      }
      /* Quel type de paquet est-ce ? */
      if(!(rdesc->CS.ImmediateData & TAG_DATA)) { /* Nous sommes en présence d'un ACK */

#ifdef DEBUG
	fprintf(stderr, "SEND: Ack reçu\n");
#endif

	credits[dest_node] += rdesc->CS.ImmediateData & CREDIT_MASK;

	/* On reposte le descripteur */
	rdesc->CS.Length = BUF_SIZE;
	rdesc->DS[0].Local.Length = BUF_SIZE;
	VERIFY(rdesc->DS[0].Local.Data.Address, recv_buf[dest_node][crd[dest_node]]);

	VERIFY(VipPostRecv(vih, rdesc, rHand), VIP_SUCCESS);

#ifdef DEBUG
	fprintf(stderr, "Re-posting rdesc[%d] = %p\n", crd[dest_node], rdesc);
#endif

	rds[dest_node][crd[dest_node]] = DESC_POSTED;
	crd[dest_node] = (crd[dest_node] + 1) & QSIZE_MASK;

      } else {
	fprintf(stderr, "Oula! C'est un ACK que j'attendais...");
	exit(1);
      }
    }

    credits[dest_node]--;
 
#endif

    /* On reclame un buffer d'émission */
    if(sds[dest_node][csd[dest_node]] != DESC_AVAIL) {

#ifdef DEBUG
      fprintf(stderr, "Waiting for sdesc[%d] = %p to be available !\n",
	      csd[dest_node], send_desc(dest_node, csd[dest_node]));
#endif

#ifdef PM2
      do {
	r = VipSendDone(viHand[dest_node], &descp);
	if(r != VIP_SUCCESS) {
	  unlock_task();
	  marcel_givehandback();
	  lock_task();
	}
      } while(r != VIP_SUCCESS);
#else
      VERIFY(VipSendWait(viHand[dest_node], VIP_INFINITE, &descp), VIP_SUCCESS);
#endif

      VERIFY(send_desc(dest_node, csd[dest_node]), descp);

    } else
      descp = send_desc(dest_node, csd[dest_node]);

#ifdef DEBUG
    fprintf(stderr, "Getting sdesc[%d] = %p\n", csd[dest_node], descp);
#endif

    /* On le remplit au maximum */
    {
      char *ptr = send_buf[dest_node][csd[dest_node]];
      int freespace = BUF_SIZE;

      descp->CS.Status = 0;
      descp->CS.Control = VIP_CONTROL_OP_SENDRECV | VIP_CONTROL_IMMEDIATE;
      descp->CS.SegCount = 1;
      descp->CS.ImmediateData = TAG_DATA | TAG_EAT_CREDIT | used_credits[dest_node];
      descp->DS[0].Local.Data.Address = ptr;
      descp->DS[0].Local.Handle = sbufHand;

      used_credits[dest_node] = 0;

      while(freespace && (v < count)) {
	int n = min(nb, freespace);

	memcpy(ptr, data, n);
	data += n; nb -= n;
	ptr += n; freespace -= n;

	if(!nb && (++v < count)) { /* On passe au vecteur suivant */
	  data = vector[v].iov_base;
	  nb = vector[v].iov_len;
	}
      }

      descp->CS.Length = BUF_SIZE - freespace;
      descp->DS[0].Local.Length = descp->CS.Length;
    }

    /* On émet enfin !*/
#ifdef DEBUG
    fprintf(stderr, "Posting one descriptor of size %d, sdesc[%d] = %p\n", 
	    descp->CS.Length, csd[dest_node], descp);
#endif
#ifdef DEBUG
    fprintf(stderr, "\t\tSEND: sending ack <%d> to %d\n", send_seq_number[dest_node]++, dest_node);
#endif

    VERIFY(VipPostSend(viHand[dest_node], descp, sHand), VIP_SUCCESS);

    sds[dest_node][csd[dest_node]] = DESC_POSTED;
    csd[dest_node] = (csd[dest_node] + 1) & QSIZE_MASK;
  }

  unlock_task();

#ifdef PM2
  marcel_mutex_unlock(&mutex[dest_node]);
#endif

  TIMING_EVENT("network_send: end");
}

static int currex;

static void send_ack(int dest_node)
{
  VIP_DESCRIPTOR *descp;
#ifdef PM2
  int r;
#endif

#ifdef DEBUG
  fprintf(stderr, "Envoi d'un accusé pour restituer des crédits\n");
#endif

  /* On reclame d'abord un buffer d'émission */
  if(sds[dest_node][csd[dest_node]] != DESC_AVAIL) {

#ifdef DEBUG
    fprintf(stderr, "Waiting for sdesc[%d] = %p to be available !\n",
	    csd[dest_node], send_desc(dest_node, csd[dest_node]));
#endif

#ifdef PM2
    do {
      r = VipSendDone(viHand[dest_node], &descp);
      if(r != VIP_SUCCESS) {
	unlock_task();
	marcel_givehandback();
	lock_task();
      }
    } while(r != VIP_SUCCESS);
#else
    VERIFY(VipSendWait(viHand[dest_node], VIP_INFINITE, &descp), VIP_SUCCESS);
#endif

    VERIFY(send_desc(dest_node, csd[dest_node]), descp);

  } else
    descp = send_desc(dest_node, csd[dest_node]);

#ifdef DEBUG
  fprintf(stderr, "Getting sdesc[%d] = %p\n", csd[dest_node], descp);
#endif

  /* On initialise le descripteur en le marquant "ACK" */
  descp->CS.Length = 0;
  descp->CS.Status = 0;
  descp->CS.Control = VIP_CONTROL_OP_SENDRECV | VIP_CONTROL_IMMEDIATE;
  descp->CS.SegCount = 0;
  descp->CS.ImmediateData = used_credits[dest_node];

  used_credits[dest_node] = 0;

  /* On poste l'accusé */
#ifdef DEBUG
  fprintf(stderr, "Posting one ACK, sdesc[%d] = %p\n", csd[dest_node], descp);
  fprintf(stderr, "SEND: sending packet <%d> to %d\n", send_seq_number[dest_node]++, dest_node);
#endif

  VERIFY(VipPostSend(viHand[dest_node], descp, sHand), VIP_SUCCESS);

  sds[dest_node][csd[dest_node]] = DESC_POSTED;
  csd[dest_node] = (csd[dest_node] + 1) & QSIZE_MASK;
}

void mad_via_network_receive(char **head)
{ 
  VIP_DESCRIPTOR *descp;
  VIP_BOOLEAN isRecv;
  VIP_VI_HANDLE vih;
  int head_size, to_read;
  char *data, *ptr = (char *)header;
  int i, n, nb;
#ifdef PM2
  int r;
#endif

  lock_task();

  /* On recherche un premier paquet (éventuellement) déjà reçu */
  for(;;) {

    currex = -1;

    for(i=0; i<confsize; i++)
      if(rds[i][crd[i]] == DESC_AVAIL) {
	currex = i;
	vih = viHand[currex];
	descp = recv_desc(i, crd[i]);
#ifdef DEBUG
	fprintf(stderr, "HEADER: Un paquet a déjà été reçu en provenance de %d, rdesc[%d] = %p\n",
		i, crd[i], descp);
#endif
	break;
      }

    /* Sinon, on attend qu'un paquet arrive */
    if(currex == -1) {

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
#ifdef DEBUG
	      fprintf(stderr, "CQWait(%x) -> vi = %p, isRecv = %d",
		      (unsigned)cqHand, vih, isRecv);
#endif
#endif
      TIMING_EVENT("network_receive: paquet recu");

      for(currex=0; vih != viHand[currex]; currex++) ;

#ifdef DEBUG
      fprintf(stderr, "RECV: receiving packet <%d> from %d\n", recv_seq_number[currex]++, currex);
#endif

      VERIFY(VipRecvDone(vih, &descp), VIP_SUCCESS);

#ifdef DEBUG
      fprintf(stderr, "rdesc[%d] = %p\n", lrd[currex], descp);
#endif

      rds[currex][lrd[currex]] = DESC_AVAIL; /* inutile ? */
      lrd[currex] = (lrd[currex] + 1) & QSIZE_MASK;

      VERIFY(recv_desc(currex, crd[currex]), descp);
    }

    if(!(descp->CS.ImmediateData & TAG_DATA)) { /* Nous sommes en présence d'un ACK */

#ifdef DEBUG
      fprintf(stderr, "HEADER: Ack reçu\n");
#endif

      credits[currex] += descp->CS.ImmediateData & CREDIT_MASK;

      /* On reposte le descripteur */
      descp->CS.Length = BUF_SIZE;
      descp->DS[0].Local.Length = BUF_SIZE;
      VERIFY(descp->DS[0].Local.Data.Address, recv_buf[currex][crd[currex]]);

      VERIFY(VipPostRecv(vih, descp, rHand), VIP_SUCCESS);

#ifdef DEBUG
	fprintf(stderr, "Re-posting rdesc[%d] = %p\n", crd[currex], descp);
#endif

      rds[currex][crd[currex]] = DESC_POSTED;
      crd[currex] = (crd[currex] + 1) & QSIZE_MASK;

    } else
      break;
  }

  /* On extrait la taille du header */
  data = descp->DS[0].Local.Data.Address;
  nb = descp->CS.Length;
  to_read = head_size = *((long *)data);

  do {
#ifdef DEBUG
    fprintf(stderr, "HEADER: J'ai reçu un descripteur de taille %d\n", nb);
#endif

    /* Traitement sur les crédits */
    credits[currex] += descp->CS.ImmediateData & CREDIT_MASK;
    VERIFY((descp->CS.ImmediateData & TAG_EAT_CREDIT), TAG_EAT_CREDIT);
    if(++used_credits[currex] == CREDITS_CRITICAL)
      send_ack(currex);

    /* On recopie dans le header */
    n = min(to_read, nb);
    memcpy(ptr, data, n);
    to_read -= n; data += n; ptr += n; nb -= n;

#ifdef DEBUG
    fprintf(stderr, "HEADER: Extraction de %d octets du paquet\n", n);
#endif

    if(!nb) { /* A-t-on tout consommé ? */
      /* Si oui, on reposte le descripteur */
      descp->CS.Length = BUF_SIZE;
      descp->DS[0].Local.Length = BUF_SIZE;
      VERIFY(descp->DS[0].Local.Data.Address, recv_buf[currex][crd[currex]]);

      VERIFY(VipPostRecv(vih, descp, rHand), VIP_SUCCESS);

#ifdef DEBUG
	fprintf(stderr, "Re-posting rdesc[%d] = %p\n", crd[currex], descp);
#endif

      rds[currex][crd[currex]] = DESC_POSTED;
      crd[currex] = (crd[currex] + 1) & QSIZE_MASK;

      if(to_read) {
#ifdef DEBUG
	fprintf(stderr, "HEADER: Besoin d'un paquet supplémentaire\n");
#endif
	/* Il faut encore recevoir au moins un paquet supplémentaire */
	for(;;) {
	  if(rds[currex][crd[currex]] == DESC_AVAIL) {
	    descp = recv_desc(currex, crd[currex]);
#ifdef DEBUG
	    fprintf(stderr, "HEADER: Le prochain paquet a déjà été reçu en provenance de %d, rdesc[%d] = %p\n",
		    i, crd[currex], descp);
#endif
	  } else {
	    int ex;

	    do {

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
#ifdef DEBUG
	      fprintf(stderr, "CQWait -> vi = %p, isRecv = %d", vih, isRecv);
#endif

#endif

	      TIMING_EVENT("network_receive: paquet recu");

	      for(ex=0; vih != viHand[ex]; ex++) ;
#ifdef DEBUG
	      fprintf(stderr, "RECV: receiving packet <%d> from %d\n", recv_seq_number[ex]++, ex);
#endif

	      VERIFY(VipRecvDone(vih, &descp), VIP_SUCCESS);

#ifdef DEBUG
	      fprintf(stderr, "rdesc[%d] = %p\n", lrd[ex], descp);
#endif

	      rds[ex][lrd[ex]] = DESC_AVAIL;
	      lrd[ex] = (lrd[ex] + 1) & QSIZE_MASK;

	    } while(ex != currex);
	  }
	  /* Est-ce bien un paquet contenant des données ? */
	  if(!(descp->CS.ImmediateData & TAG_DATA)) { /* Nous sommes en présence d'un ACK */

#ifdef DEBUG
	    fprintf(stderr, "HEADER: Ack reçu\n");
#endif

	    credits[currex] += descp->CS.ImmediateData & CREDIT_MASK;

	    /* On reposte le descripteur */
	    descp->CS.Length = BUF_SIZE;
	    descp->DS[0].Local.Length = BUF_SIZE;
	    VERIFY(descp->DS[0].Local.Data.Address, recv_buf[currex][crd[currex]]);

	    VERIFY(VipPostRecv(vih, descp, rHand), VIP_SUCCESS);

#ifdef DEBUG
	    fprintf(stderr, "Re-posting rdesc[%d] = %p\n", crd[currex], descp);
#endif
	    rds[currex][crd[currex]] = DESC_POSTED;
	    crd[currex] = (crd[currex] + 1) & QSIZE_MASK;

	  } else
	    break;
	}
	data = descp->DS[0].Local.Data.Address;
	nb = descp->CS.Length;
      }
    } else {
      /* Si non, alors il faut le marquer "AVAIL" */
      rds[currex][crd[currex]] = DESC_AVAIL;
      descp->CS.ImmediateData = TAG_DATA; /* Plus de crédits à comptabiliser */
      descp->CS.Length = nb; /* Il ne reste plus que nb octets dans le segment */
      descp->DS[0].Local.Length = nb;
      descp->DS[0].Local.Data.Address = data;
    }

  } while(to_read);

  unlock_task();

  TIMING_EVENT("network_receive: appel du handler");

  *head = (char *)header;
}

void mad_via_network_receive_data(struct iovec *vector, size_t count)
{
  VIP_DESCRIPTOR *descp;
  char *ptr;
  int v, freespace;
  VIP_BOOLEAN isRecv;
  VIP_VI_HANDLE vih = viHand[currex];
#ifdef PM2
  int r;
#endif

  lock_task();

  v = 0;
  ptr = vector[0].iov_base;
  freespace = vector[0].iov_len;

  while(v < count) { /* Il reste des données à recevoir */

    /* On reclame d'abord un paquet du réseau */
    for(;;) {
      if(rds[currex][crd[currex]] == DESC_AVAIL) {
	descp = recv_desc(currex, crd[currex]);
#ifdef DEBUG
	fprintf(stderr, "BODY: Le prochain paquet a déjà été reçu en provenance de %d, rdsc[%d] = %p\n",
		currex, crd[currex], descp);
#endif
      } else {
	int ex;

	do {

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
#ifdef DEBUG
	      fprintf(stderr, "CQWait(%x) -> vi = %p, isRecv = %d",
		      (unsigned)cqHand, vih, isRecv);
#endif
#endif

	  for(ex=0; vih != viHand[ex]; ex++) ;
#ifdef DEBUG
	  fprintf(stderr, "BODY: receiving packet <%d> from %d\n", recv_seq_number[ex]++, ex);
#endif

	  VERIFY(VipRecvDone(vih, &descp), VIP_SUCCESS);

#ifdef DEBUG
	  fprintf(stderr, "rdesc[%d] = %p\n", lrd[ex], descp);
#endif

	  rds[ex][lrd[ex]] = DESC_AVAIL;
	  lrd[ex] = (lrd[ex] + 1) & QSIZE_MASK;

	} while(ex != currex);
      }
      /* Est-ce bien un paquet contenant des données ? */
      if(!(descp->CS.ImmediateData & TAG_DATA)) { /* Nous sommes en présence d'un ACK */

#ifdef DEBUG
	fprintf(stderr, "BODY: Ack reçu\n");
#endif

	credits[currex] += descp->CS.ImmediateData & CREDIT_MASK;

	/* On reposte le descripteur */
	descp->CS.Length = BUF_SIZE;
	descp->DS[0].Local.Length = BUF_SIZE;
	VERIFY(descp->DS[0].Local.Data.Address, recv_buf[currex][crd[currex]]);

	VERIFY(VipPostRecv(vih, descp, rHand), VIP_SUCCESS);

#ifdef DEBUG
	fprintf(stderr, "Re-posting rdesc[%d] = %p\n", crd[currex], descp);
#endif

	rds[currex][crd[currex]] = DESC_POSTED;
	crd[currex] = (crd[currex] + 1) & QSIZE_MASK;

      } else
	break;
    }

    /* Traitement sur les crédits */
    credits[currex] += descp->CS.ImmediateData & CREDIT_MASK;
    if((descp->CS.ImmediateData & TAG_EAT_CREDIT) &&
       ++used_credits[currex] == CREDITS_CRITICAL)
      send_ack(currex);

    /* On l'exploite au maximum */
    {
      char *data = descp->DS[0].Local.Data.Address;
      int nb = descp->CS.Length;

      while(nb) {
	int n = min(nb, freespace);

#ifdef DEBUG
	fprintf(stderr, "BODY: Extraction de %d octets du paquet\n", n);
#endif

	memcpy(ptr, data, n);
	data += n; nb -= n;
	ptr += n; freespace -= n;

	if(!freespace && (++v < count)) { /* On passe au vecteur suivant */
	  ptr = vector[v].iov_base;
	  freespace = vector[v].iov_len;
	}
      }

      /* A présent, il faut reposter le descripteur */
      descp->CS.Length = BUF_SIZE;
      descp->DS[0].Local.Length = BUF_SIZE;
      descp->DS[0].Local.Data.Address = recv_buf[currex][crd[currex]];

      VERIFY(VipPostRecv(vih, descp, rHand), VIP_SUCCESS);

      rds[currex][crd[currex]] = DESC_POSTED;
      crd[currex] = (crd[currex] + 1) & QSIZE_MASK;
      
    }
  }

  unlock_task();
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
  argv[j] = NULL;
}

static void spawn_procs(char *argv[], int port)
{ 
  char cmd[1024], arg[128];
  int i;

  sprintf(cmd, "%s/bin/via/madspawn %s %d %d %s",
	  get_mad_root(), my_name, confsize, port, cons_image[cons]);
  i=0;
  while(argv[i]) {
    sprintf(arg, " %s ", argv[i]);
    strcat(cmd, arg);
    i++;
  }
  system(cmd);
}

void mad_via_network_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int i, f;
  char output[64];
  char *_argv[128]; /* sauvegarde */
  char execname[1024];

  get_machines();

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
      int ret = system("exit `cat ${MAD1_ROOT-${PM2_ROOT}/mad1}/.madconf | wc -w`");

      confsize = WEXITSTATUS(ret);
    }

    my_name = machine_name_of_process(0);

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

      if(cons != NO_CONSOLE)
	confsize++;

#ifdef DEBUG
    fprintf(stderr, "Process %d started on %s\n", pm2self, my_name);
#endif

    if(confsize > 1) {

      if(_argv[0][0] != '/') {
	getcwd(execname, 1024);
	strcat(execname, "/");
	strcat(execname, _argv[0]);
	_argv[0] = execname;
	if(strlen(execname) >= 1024)
	  fprintf(stderr, "master: Ouille !!!\n");
      }

      spawn_procs(argv, 0);
    }

  } else { /* slave process */

    parse_cmd_line(argc, argv);

    pm2self = atoi(getenv("MAD_MY_NUMBER"));
    confsize = atoi(getenv("MAD_NUM_NODES"));
    my_name = machine_name_of_process(pm2self);

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
  }

  initialiser_via();

#ifdef PM2
  for(i=0; i<confsize; i++)
    marcel_mutex_init(&mutex[i], NULL);
#endif

  if(cons != NO_CONSOLE)
    *nb = confsize - 1;
  else
    *nb = confsize;

  *whoami = pm2self;
  for(i=0; i<*nb; i++)
    tids[i] = i;
}

void mad_via_network_exit()
{
  int i;

  FREE(localAddr);
  FREE(remoteAddr);

  for(i=0; i<confsize; i++) {
    if(pm2self != i) {
      VERIFY(VipDisconnect(viHand[i]), VIP_SUCCESS);
      VERIFY(VipDestroyVi(viHand[i]), VIP_SUCCESS);
#ifdef DEBUG
      fprintf(stderr, "credits[%d] = %d\n", i, credits[i]);
      fprintf(stderr, "used_credits[%d] = %d\n", i, used_credits[i]);
#endif
    }
  }

  VERIFY(VipDestroyCQ(cqHand), VIP_SUCCESS);

  VERIFY(VipDeregisterMem(nicHand, recv_buf, rbufHand), VIP_SUCCESS);
  VERIFY(VipDeregisterMem(nicHand, send_buf, sbufHand), VIP_SUCCESS);
  VERIFY(VipDeregisterMem(nicHand, _sdesc, sHand), VIP_SUCCESS);
  VERIFY(VipDeregisterMem(nicHand, _rdesc, rHand), VIP_SUCCESS);
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

