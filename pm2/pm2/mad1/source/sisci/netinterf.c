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

/* #define DEBUG */
#define NEW_IMPL

#include "sisci_api.h"
#include "sisci_types.h"
#include "sisci_error.h"

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

#include <sys/debug.h>
#include <mad_types.h>
#include <safe_malloc.h>


#define NO_FLAGS            0
#define NO_CALLBACK         0
#define LOCAL_BUFFER_SIZE   (128*1024)

typedef struct box {
  sci_error_t             error;
  sci_desc_t              sd;
  sci_local_segment_t     localSegment;
  sci_remote_segment_t    remoteSegment;
  sci_map_t               localMap, remoteMap;
  sci_sequence_t          sequence;
  sci_dma_queue_t         dmaQueue;
  sci_dma_queue_state_t   dmaQueueState;
  unsigned int            localAdapterNo;  
  int                     localNodeId, remoteNodeId ;
  unsigned int            localSegmentId, remoteSegmentId;
  unsigned int            segmentSize;
  volatile void           *localMapAddr, *remoteMapAddr;
  volatile unsigned       local_writeptr, local_readptr;
} SCI_Box;

#define ALIGNED_64(addr) (((unsigned long)(addr) + 63) & ~(63L))
#define ALIGNED_32(addr) (((unsigned long)(addr) + 31) & ~(31L))
#define ALIGNED_4(addr)  (((unsigned long)(addr) + 3)  & ~(3L))

union _CONTROL_MSG_SIZE {
  char foo[64];
#ifdef NEW_IMPL
  volatile unsigned remote_writeptr;
#else
  volatile int size;
#endif
} _glob_recv;

union _CONTROL_SEND_FLAG {
  char foo[64];
#ifdef NEW_IMPL
  volatile unsigned remote_readptr;
#else
  volatile int status;
#endif
} _glob_send;

struct _CONTROL_HEADER {
  union _CONTROL_MSG_SIZE recv;
  union _CONTROL_SEND_FLAG send;
};

#define _MAD_CONTROL_HEADER      sizeof(struct _CONTROL_HEADER)
#define _MAX_DATA (LOCAL_BUFFER_SIZE - _MAD_CONTROL_HEADER)

#undef min
#define min(a, b)  ((a) < (b) ? (a) : (b))

#ifdef NEW_IMPL

#define MAX_PAQUET (8*1024)
#define MIN_DIST   MAX_PAQUET

#define REM_CTRL_STRUCT(box) ((struct _CONTROL_HEADER *)(box)->remoteMapAddr)
#define LOC_CTRL_STRUCT(box) ((struct _CONTROL_HEADER *)(box)->localMapAddr)

#define READ_OFFSET(box) (_MAD_CONTROL_HEADER + (box)->local_readptr)
#define READ_ADDR(box)   ((char *)((box)->localMapAddr) + READ_OFFSET(box))

#define WRITE_OFFSET(box) (_MAD_CONTROL_HEADER + (box)->local_writeptr)
#define WRITE_ADDR(box)   ((char *)((box)->remoteMapAddr) + WRITE_OFFSET(box))

#define UPDATE_READPTR(box) \
   REM_CTRL_STRUCT(box)->send.remote_readptr = (box)->local_readptr

#define UPDATE_WRITEPTR(box) \
   REM_CTRL_STRUCT(box)->recv.remote_writeptr = (box)->local_writeptr

static __inline__ unsigned MAX_READABLE(SCI_Box *box)
{
  unsigned ecart, res;

  ecart = (LOC_CTRL_STRUCT(box)->recv.remote_writeptr +
	   _MAX_DATA - box->local_readptr) % _MAX_DATA;
  res = min(ecart, _MAX_DATA - box->local_readptr);
  return res;
}

static __inline__ unsigned MAX_WRITABLE(SCI_Box *box)
{
  unsigned ecart, res;

  ecart = (LOC_CTRL_STRUCT(box)->send.remote_readptr +
	   _MAX_DATA - box->local_writeptr) % _MAX_DATA;
  if(!ecart)
    ecart = _MAX_DATA;
  else if(ecart < MIN_DIST)
    return 0;
  
  res = min(min(ecart - MIN_DIST, _MAX_DATA - box->local_writeptr),
	    MAX_PAQUET);
  return res;
}

#else

/* reception */
#define _MAD_FLAG_NO_MESSAGE     0

#define _MSG_SIZE(box) \
   (((struct _CONTROL_HEADER *)(box)->localMapAddr)->recv.size)

#define _READ_IMPOSSIBLE(box) \
   (((struct _CONTROL_HEADER *)(box)->localMapAddr)->recv.size == \
    _MAD_FLAG_NO_MESSAGE  )

#define _SET_READ_IMPOSSIBLE(box) \
   ((struct _CONTROL_HEADER *)(box)->localMapAddr)->recv.size = \
    _MAD_FLAG_NO_MESSAGE

#define _SET_READ_POSSIBLE(box, len) \
   ((struct _CONTROL_HEADER *)(box)->remoteMapAddr)->recv.size = (len)


/* emission */
#define _MAD_FLAG_MAILBOX_EMPTY  0
#define _MAD_FLAG_MAILBOX_FULL   1

#define _WRITE_IMPOSSIBLE(box) \
   (((struct _CONTROL_HEADER *)(box)->localMapAddr)->send.status == \
    _MAD_FLAG_MAILBOX_FULL)

#define _SET_WRITE_IMPOSSIBLE(box) \
   ((struct _CONTROL_HEADER *)(box)->localMapAddr)->send.status = \
    _MAD_FLAG_MAILBOX_FULL

#define _SET_WRITE_POSSIBLE(box) \
   ((struct _CONTROL_HEADER *)(box)->remoteMapAddr)->send.status = \
    _MAD_FLAG_MAILBOX_EMPTY

#endif

#define _FLUSH_WRITES(box) \
   SCIStoreBarrier((box)->sequence, NO_FLAGS, &(box)->error), \
   mdebug("Flushing...\n")


static SCI_Box        SCI_box_tab[MAX_MODULES];
static int            SCI_NodeId[MAX_MODULES];

#ifdef PM2
static marcel_mutex_t mutex[MAX_MODULES];
#endif
static int send_sock[MAX_MODULES];
static int private_pipe[2];
static int pm2self, confsize;
static fd_set read_fds;
static int nb_fds;

static char my_name[128];

#undef min
#define min(a, b)  ((a) < (b) ? (a) : (b))
#undef max
#define max(a, b)  ((a) > (b) ? (a) : (b))

#define MAX_IOV    16

static char cons_image[1024];

#define  TEST_ERROR(text)                                         \
   if (error != SCI_ERR_OK)   {                                   \
     fprintf(stderr,"%s failed - Error code 0x%x\n",text,error);  \
     exit(0);                                                     \
   }

void my_memcpy(volatile char *dst, volatile char *src, int size)
{
  while(size--)
    *dst++ = *src++;
}

static SCI_Box *current_expeditor;

#ifdef NEW_IMPL

static long header[MAX_HEADER/sizeof(long)];
static unsigned header_len;

static __inline__ void MAD_SISCI_COPY(SCI_Box *box, char *src, unsigned size)
{
  if(size % 4) {
    mdebug("mad_send: warning: inefficient transmission"
	   " of unaligned data...\n");
    my_memcpy(WRITE_ADDR(box), src, size);
  } else {
    SCIMemCopy(src,
	       box->remoteMap, WRITE_OFFSET(box),
	       size,
	       NO_FLAGS, &box->error);
  }
  mdebug("Sent %d bytes at @ %p\n",
	 size,
	 WRITE_ADDR(box));
}

static unsigned nb_send = 0;

void mad_sisci_network_send(int dest_node,
			    struct iovec *vector, size_t count)
{
  SCI_Box *box;
  unsigned len, index, max_w;
  char *ptr;
  
#ifdef PM2
  marcel_mutex_lock(&mutex[dest_node]);
#endif

  if(dest_node != pm2self) {

    ++nb_send;

    box = &SCI_box_tab[dest_node];

    index = 0;
    max_w = 0;

    /* header */
    len = ALIGNED_64(vector[0].iov_len);

    ptr = vector[0].iov_base;

    /* Utilisation de la zone de piggyback */
    ((long *)ptr)[0] = len;

    while(index < count) {

      if(!max_w) {
	mdebug("Waiting for write permission...\n");

	while((max_w = MAX_WRITABLE(box)) == 0) {
#ifdef PM2
	  marcel_givehandback();
#else
	  _FLUSH_WRITES(box);
#endif
	}

	mdebug("Permission granted for %d bytes\n", max_w);
      }

      if(max_w < len) {

	MAD_SISCI_COPY(box, ptr, max_w);

	box->local_writeptr = (box->local_writeptr + max_w) % _MAX_DATA;

	len -= max_w;
	ptr += max_w;

	max_w = 0;

	UPDATE_WRITEPTR(box);
	_FLUSH_WRITES(box);

      } else {

	MAD_SISCI_COPY(box, ptr, len);

	box->local_writeptr = (box->local_writeptr +
				 ALIGNED_64(len)) % _MAX_DATA;

	max_w -= ALIGNED_64(len);

	if(++index < count) {
	  ptr = vector[index].iov_base;
	  len = vector[index].iov_len;
	} else {
	  UPDATE_WRITEPTR(box);
	  _FLUSH_WRITES(box);
	}
      }
    }
  }

#ifdef PM2
  marcel_mutex_unlock(&mutex[dest_node]);
#endif
}

static unsigned nb_recv = 0;

void mad_sisci_network_receive(char **head)
{ 
  static int i = -1;
  unsigned max_r;

  do {
    i = (i+1) % confsize;
#ifdef PM2
    marcel_givehandback();
#endif
  } while((i == pm2self) || ((max_r = MAX_READABLE(&SCI_box_tab[i])) == 0));

  ++nb_recv;

  mdebug("Receiving one header...\n");

  current_expeditor = &SCI_box_tab[i];

  header_len = ((long *)READ_ADDR(current_expeditor))[0];

  mdebug("Header size = %d bytes\n", header_len);

  if(header_len > _MAX_DATA - current_expeditor->local_readptr) {
    /* Il faut recopier dans un emplacement contigu */
    char *ptr = (char *)header;

    do {
      mdebug("read ptr = %d, write ptr = %d\n",
	     current_expeditor->local_readptr,
	     LOC_CTRL_STRUCT(current_expeditor)->recv.remote_writeptr);

      mdebug("Reading %d bytes\n", max_r);

      memcpy(ptr, READ_ADDR(current_expeditor), max_r);
      current_expeditor->local_readptr = (current_expeditor->local_readptr +
					  ALIGNED_64(max_r)) % _MAX_DATA;
      UPDATE_READPTR(current_expeditor);

      ptr += max_r;
      header_len -= max_r;

      if(header_len) {

	mdebug("now, I'm expecting the last %d bytes\n", header_len);

	mdebug("read ptr = %d, write ptr = %d\n",
	       current_expeditor->local_readptr,
	       LOC_CTRL_STRUCT(current_expeditor)->recv.remote_writeptr);

	while((max_r = MAX_READABLE(current_expeditor)) == 0) {
#ifdef PM2
	  marcel_givehandback();
#else
	  _FLUSH_WRITES(current_expeditor);
#endif
	}

	max_r = min(max_r, header_len);
      }

    } while(header_len);

    *head = (char *)header;

  } else {
    /* Il suffit d'attendre que toutes les données soient disponibles */
    while(max_r < header_len) {
#ifdef PM2
      marcel_givehandback();
#else
      _FLUSH_WRITES(current_expeditor);
#endif
      max_r = MAX_READABLE(current_expeditor);
    }

    mdebug("Possibility to read %d bytes (header = %d)\n", max_r, header_len);

    *head = (char *)READ_ADDR(current_expeditor);

    current_expeditor->local_readptr = (current_expeditor->local_readptr +
					ALIGNED_64(header_len)) % _MAX_DATA;
    UPDATE_READPTR(current_expeditor);
  }
}

void mad_sisci_network_receive_data(struct iovec *vector, size_t count)
{
  char *ptr;
  unsigned index = 0, len, max_r;

  ptr = vector[0].iov_base;
  len = vector[0].iov_len;
  
  while(index < count) {

    while((max_r = MAX_READABLE(current_expeditor)) == 0) {
#ifdef PM2
      marcel_givehandback();
#else
	SCIFlushReadBuffers(current_expeditor->sequence);
	_FLUSH_WRITES(current_expeditor);
#endif
    }

    if(max_r < len) {

      mdebug("Reading %d bytes\n", max_r);

      memcpy(ptr, READ_ADDR(current_expeditor), max_r);
      current_expeditor->local_readptr = (current_expeditor->local_readptr +
					  max_r) % _MAX_DATA;
      UPDATE_READPTR(current_expeditor);

      len -= max_r;
      ptr += max_r;

    } else {

      mdebug("Reading %d bytes\n", len);

      memcpy(ptr, READ_ADDR(current_expeditor), len);
      current_expeditor->local_readptr = (current_expeditor->local_readptr +
					  ALIGNED_64(len)) % _MAX_DATA;
      UPDATE_READPTR(current_expeditor);

      if(++index < count) {
	ptr = vector[index].iov_base;
	len = vector[index].iov_len;
      }
    }
  }
}  

#else /* !NEW_IMPL */

static void *the_body;

void mad_sisci_network_send(int dest_node,
			    struct iovec *vector, size_t count)
{
  SCI_Box *box;
  unsigned hlen, len, i, offset;
  
#ifdef PM2
  marcel_mutex_lock(&mutex[dest_node]);
#endif

  if(dest_node != pm2self) {

      box = &SCI_box_tab[dest_node];

      offset = _MAD_CONTROL_HEADER;
      mdebug("Attend autorisation d'emettre...\n");

      while(_WRITE_IMPOSSIBLE(box)) {
	// S'aranger pour que la boucle ne soit pas trop rapide
	// sinon il n'y a pas mise a jour des buffer.
#ifdef PM2
	marcel_givehandback();
#else
	SCIFlushReadBuffers(box->sequence);
#endif
      }

      mdebug("Autorisation recue...\n");

      _SET_WRITE_IMPOSSIBLE(box);

      /* header */
      hlen = ALIGNED_64(vector[0].iov_len);
      SCIMemCopy(vector[0].iov_base,
		 box->remoteMap, offset,
		 hlen, NO_FLAGS, &box->error);
      mdebug("Sended %d bytes at @ %p\n",
	     hlen,
	     box->remoteMapAddr + offset);
      offset += ALIGNED_64(hlen);

      /* body */
      for(i=1; i<count; i++) {
	len = vector[i].iov_len;
	if(len % 4) {
	  mdebug("mad_send: warning: inefficient transmission"
		 " of unaligned data...\n");
	  my_memcpy(box->remoteMapAddr + offset,
		    vector[i].iov_base,
		    len);
	} else {
	  SCIMemCopy(vector[i].iov_base,
		     box->remoteMap, offset,
		     len, NO_FLAGS, &box->error);
#ifdef DEBUG
	  if(box->error != SCI_ERR_OK) {
	    mdebug("SCIMemCopy failed - Error code 0x%x\n",
		   box->error);
	    exit(1);
	  }
#endif
	}
	mdebug("Sended %d bytes at @ %p\n",
	       len,
	       box->remoteMapAddr + offset);
	offset += ALIGNED_4(len);
      }

      /* _FLUSH_WRITES(box); */ /* header + body */

      _SET_READ_POSSIBLE(box, hlen); /* mailbox flag */
      _FLUSH_WRITES(box);
    }
#ifdef PM2
  marcel_mutex_unlock(&mutex[dest_node]);
#endif
}

void mad_sisci_network_receive(char **head)
{
  static int i = -1;

  do {
    i = (i+1)%confsize;
#ifdef PM2
    marcel_givehandback();
#endif
  } while((i == pm2self) || _READ_IMPOSSIBLE(&SCI_box_tab[i]));

  mdebug("message recu...\n");

  current_expeditor = &SCI_box_tab[i];

  *head = (char *)SCI_box_tab[i].localMapAddr + _MAD_CONTROL_HEADER;

  the_body = *head + ALIGNED_64(_MSG_SIZE(&SCI_box_tab[i]));

  mdebug("Received %d bytes at @ %p (header)\n",
	 _MSG_SIZE(&SCI_box_tab[i]),
	 *head);

  _SET_READ_IMPOSSIBLE(&SCI_box_tab[i]);
}

void mad_sisci_network_receive_data(struct iovec *vector, size_t count)
{
  unsigned i, len;

  for(i=0; i<count; i++) {
    len = vector[i].iov_len;
    memcpy(vector[i].iov_base, the_body, len);
    mdebug("Received %d bytes at @ %p (body)\n",
	   len,
	   the_body);
    the_body += ALIGNED_4(len);
  }

  _SET_WRITE_POSSIBLE(current_expeditor);
  /* _FLUSH_WRITES(current_expeditor); */

  mdebug("Emetteur a nouveau autorise...\n");
}  

#endif /* NEW_IMPL */

static int SCIGetNodeId()
{  
  sci_desc_t sd;
  sci_error_t error;
  struct sci_query_adapter queryAdapter;
  int number ;
  
  queryAdapter.subcommand = SCI_Q_ADAPTER_NODEID ; 
  queryAdapter.localAdapterNo = 0;
  queryAdapter.data = &number;
  
  SCIOpen(&sd, NO_FLAGS, &error);
  if(error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIOpen failed - Error code 0x%x\n",
	    error);
    number = -2; 
    return number;
  }

  SCIQuery(SCI_Q_ADAPTER,&queryAdapter,NO_FLAGS,&error);
  if(error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIQuery failed - Error code 0x%x\n",
	    error);
    number = -1;
    return number;
  }

  SCIClose(sd, NO_FLAGS, &error);
  if(error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIClose failed - Error code: 0x%x \n",
	    error);
  }
  return number;
}

void InitBox(SCI_Box *Box,int  local, int remote)
{
  Box->localNodeId = local;
  Box->remoteNodeId = remote;
  Box->localSegmentId = (Box->localNodeId << 16 ) | Box->remoteNodeId;
  Box->remoteSegmentId = (Box->remoteNodeId << 16) | Box->localNodeId;
  Box->segmentSize = LOCAL_BUFFER_SIZE;
  Box->localAdapterNo =0;
#ifdef NEW_IMPL
  Box->local_readptr = Box->local_writeptr = 0;
#endif
}

void  SCIMakeBox(SCI_Box *Box)
{ 
  SCIOpen(&(Box->sd),
	  NO_FLAGS,
	  &(Box->error));
  if(Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIOpen failed - Error code 0x%x\n",
	    Box->error);
    exit(Box->error); 
  }

  SCICreateSegment(Box->sd,
		   &(Box->localSegment),
		   Box->localSegmentId,
		   Box->segmentSize,
                   NO_CALLBACK,
		   NULL,
		   NO_FLAGS,
		   &(Box->error));
  if(Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCICreateBox failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }
  
  SCIPrepareSegment(Box->localSegment,
		    Box->localAdapterNo,
		    NO_FLAGS,
		    &(Box->error));
  if(Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIPrepareBox failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }
  
  Box->localMapAddr = SCIMapLocalSegment(Box->localSegment,
					 &(Box->localMap),
					 0,
					 Box->segmentSize,
					 NULL,
					 NO_FLAGS,
					 &(Box->error));

  if(Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIMapLocalBox failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }

#ifndef NEW_IMPL
  _SET_READ_IMPOSSIBLE(Box);  
  _SET_WRITE_IMPOSSIBLE(Box);
#endif

  SCISetSegmentAvailable(Box->localSegment,
			 Box->localAdapterNo,
			 NO_FLAGS,
			 &(Box->error));
  if(Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCISetBoxAvailable failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }
}

void  SCIConnectRemote(SCI_Box *Box) 
{
  
  do {
    SCIConnectSegment(Box->sd,
		      &(Box->remoteSegment),
		      Box->remoteNodeId,
		      Box->remoteSegmentId,
		      Box->localAdapterNo,
		      NO_CALLBACK,
		      NULL,
		      SCI_INFINITE_TIMEOUT,
		      NO_FLAGS,
		      &(Box->error));
    if(Box->error != SCI_ERR_OK) {
      mdebug("waiting for remote segment to become available (err 0x%x)\n",
	     Box->error);
      sleep(1);
    }
  } while (Box->error != SCI_ERR_OK);
  
  Box->remoteMapAddr = SCIMapRemoteSegment(Box->remoteSegment,
					   &(Box->remoteMap),
					   0,
					   Box->segmentSize,
					   NULL,
					   NO_FLAGS,
					   &(Box->error));
  if (Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIMapRemoteBox failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }
  
  SCICreateMapSequence(Box->remoteMap,
		       &(Box->sequence),
		       NO_FLAGS,
		       &(Box->error));
  
  if (Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCICreateMapSequence failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }
}

void  SCICleanRemote(SCI_Box *Box)
{
  SCIUnmapSegment(Box->remoteMap,
		  NO_FLAGS,
		  &(Box->error));
  if (Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIUnmapBox failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }
  
  SCIDisconnectSegment(Box->remoteSegment,
		       NO_FLAGS,
		       &(Box->error));
  if (Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIDisconnectBox failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }
}

void SCIRemove(SCI_Box *Box) 
 {  
  SCISetSegmentUnavailable(Box->localSegment,
			   Box->localAdapterNo,
			   NO_FLAGS,
			   &(Box->error));
  if(Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCISetBoxUnvailable failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }
  
  SCIUnmapSegment(Box->localMap,
		  NO_FLAGS,
		  &(Box->error));
  if (Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIUnmapBox failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }
  
  SCIRemoveSegment(Box->localSegment,
		   NO_FLAGS,
		   &(Box->error));
  if (Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIRemoveBox failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  }
  
  SCIRemoveSequence(Box->sequence,
		    NO_FLAGS,
		    &(Box->error));
  if (Box->error != SCI_ERR_OK) {
    fprintf(stderr,
	    "SCIRemoveSequence failed - Error code 0x%x\n",
	    Box->error);
    exit(1);
  } 
 } 

static void parse_cmd_line(int *argc, char **argv)
{
  int i, j;

  i = j = 1;
  strcpy(cons_image, "no");
  while(i<*argc) {
    if(!strcmp(argv[i], "-pm2cons")) {
      if(i == *argc-1) {
	fprintf(stderr,
		"Fatal error:"
		" -pm2cons option must be followed by <console_name>.\n");
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

static void spawn_procs(char *argv[], int port)
{ 
  char cmd[1024], arg[128];
  int i;

  sprintf(cmd, "%s/bin/sisci/madspawn %s %d %d %s",
	  getenv("MADELEINE_ROOT"),
	  my_name, confsize, port, cons_image);
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

  if((desc = socket(AF_INET, type, 0)) == -1) {
    perror("ERROR: Cannot create socket\n");
    return -1;
  }
  
  tempo.sin_family = AF_INET;
  tempo.sin_addr.s_addr = htonl(INADDR_ANY);
  tempo.sin_port = htons(port);

  if(bind(desc,
	  (struct sockaddr *)&tempo,
	  sizeof(struct sockaddr_in)) == -1) {
    perror("ERROR: Cannot bind socket\n");
    close(desc);
    return -1;
  }

  if(adresse != NULL)
    if(getsockname(desc,
		   (struct sockaddr *)adresse,
		   &length) == -1) {
      perror("ERROR: Cannot get socket name\n");
      return -1;
    }

  return desc;
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
      /* on recoit les coordonnées du processus i 
	 et on les diffuse aux autres */
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

    /* Attente que les processus aient terminés 
       leurs connections respectives... */
    for(i=1; i<confsize; i++) {
      read(send_sock[i], &n, sizeof(int));
    }

    /* C'est bon, tout le monde peut démarrer ! */
    for(i=1; i<confsize; i++) {
      write(send_sock[i], &n, sizeof(int));
    }
  }
}
/************************************************************************************/
static void slave(int serverport, char *serverhost)
{
  struct sockaddr_in adresse, serveur;
  struct hostent *host;
  char hostname[64];
  int i, j;

  if((host = gethostbyname(serverhost)) == NULL) {
    fprintf(stderr,
	    "ERROR: Cannot find internet address of %s\n",
	    serverhost);
    exit(1);
  }

  if((send_sock[0] = creer_socket(SOCK_STREAM, 0, &adresse)) == -1)
    exit(1);

  serveur.sin_family = AF_INET;
  serveur.sin_port = htons(serverport);
  memcpy(&serveur.sin_addr.s_addr,
	 host->h_addr,
	 host->h_length);

  if(connect(send_sock[0],
	     (struct sockaddr *)&serveur,
	     sizeof(struct sockaddr_in)) == -1){
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
	fprintf(stderr,
		"ERROR: Cannot find internet address of %s\n",
		hostname);
	exit(1);
      }

      if((send_sock[num] = creer_socket(SOCK_STREAM, 0, &adresse)) == -1) {
	fprintf(stderr, "Aie...\n");
	exit(1);
      }

      serveur.sin_family = AF_INET;
      serveur.sin_port = htons(port);
      memcpy(&serveur.sin_addr.s_addr,
	     host->h_addr,
	     host->h_length);

      if(connect(send_sock[num],
		 (struct sockaddr *)&serveur,
		 sizeof(struct sockaddr_in)) == -1){
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

void mad_sisci_network_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int i,j,f;
  char output[64];
  int un = 1, packet = 0x8000;
  struct linger ling = { 1, 50 };
  char *_argv[128]; /* sauvegarde */

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
      } while((nb_proc < 0 || nb_proc > MAX_MODULES) &&
	      nb_proc != NETWORK_ONE_PER_NODE);
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
      sprintf(output, "/tmp/%s-pm2log-%d",
	      getenv("USER"),
	      pm2self);
#else
      sprintf(output, "/tmp/%s-madlog-%d",
	      getenv("USER"),
	      pm2self);
#endif
      dup2((f = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0600)),
	   STDOUT_FILENO);
      dup2(STDOUT_FILENO, STDERR_FILENO);
      close(f);
    }

    mdebug("Process %d started on %s\n", pm2self, my_name);

    slave(atoi(getenv("MAD_SERVER_PORT")),
	  getenv("MAD_SERVER_HOST"));
  } /* fin du else */


  pipe(private_pipe);
  /* uniformisation de la réception */
  send_sock[pm2self] = private_pipe[0];

  for(i=0; i<confsize; i++) {
#ifndef DONT_POLL
    if(fcntl(send_sock[i], F_SETFL, O_NONBLOCK) < 0) {
      perror("fcntl");
      exit(1);
    }
#endif
    if(i != pm2self) {
       if(setsockopt(send_sock[i],
		     IPPROTO_TCP,
		     TCP_NODELAY,
		     (char *)&un,
		     sizeof(int)) == -1) {
	 perror("setsockopt failed");
	 exit(1);
       }
       if(setsockopt(send_sock[i],
		     SOL_SOCKET,
		     SO_LINGER,
		     (char *)&ling,
		     sizeof(struct linger)) == -1) {
	 perror("setsockopt failed");
	 exit(1);
       }
       if(setsockopt(send_sock[i],
		     SOL_SOCKET,
		     SO_SNDBUF,
		     (char *)&packet,
		     sizeof(int)) == -1) {
	 perror("setsockopt failed");
	 exit(1);
       }
       if(setsockopt(send_sock[i],
		     SOL_SOCKET,
		     SO_RCVBUF,
		     (char *)&packet,
		     sizeof(int)) == -1) {
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
  for(i=0; i<*nb; i++)
    tids[i] = i;

  /******************************SCI***************************************/
  
  SCI_NodeId[pm2self] = SCIGetNodeId();
  
  if(pm2self != 0)
    write(send_sock[0], &SCI_NodeId[pm2self], sizeof(int));
  else
    for(i=1;i<confsize;i++)
      while(read(send_sock[i], &SCI_NodeId[i], sizeof(int))==-1);
  
  if(pm2self == 0){
    for(i=1;i<confsize;i++)
      for(j=0;j<confsize;j++)
	if(j != i)
	  write(send_sock[i], &SCI_NodeId[j], sizeof(int));
  } else {
    for(i=0;i<confsize;i++)
      if(i != pm2self)
	while(read(send_sock[0], &SCI_NodeId[i], sizeof(int))==-1);
  }

  for(i=0;i<confsize;i++)
    if(i != pm2self) 
      InitBox(&(SCI_box_tab[i]), SCI_NodeId[pm2self], SCI_NodeId[i]);

  for(i=0;i<confsize;i++)
    if(i != pm2self)
      SCIMakeBox(&SCI_box_tab[i]);
  
  for(i=0;i<confsize;i++)
    if(i != pm2self) {
      SCIConnectRemote(&SCI_box_tab[i]);

#ifndef NEW_IMPL
      _SET_WRITE_POSSIBLE(&SCI_box_tab[i]);
      _FLUSH_WRITES(&SCI_box_tab[i]);
#endif
    }
}

void mad_sisci_network_exit()
{
  int i;

  for(i=0; i<confsize; i++) {
    close(send_sock[i]);
    if (i != pm2self) {
      SCICleanRemote(&SCI_box_tab[i]);
    }
  }
  
  for(i=0; i<confsize; i++)
    if (i != pm2self) {
      SCIRemove(&SCI_box_tab[i]);
      SCIClose(SCI_box_tab[i].sd, NO_FLAGS, &(SCI_box_tab[i].error));
    }

  close(private_pipe[1]);
}

/**************************************************************************/
static netinterf_t mad_sisci_netinterf = {
  mad_sisci_network_init,
  mad_sisci_network_send,
  mad_sisci_network_receive,
  mad_sisci_network_receive_data,
  mad_sisci_network_exit
};

netinterf_t *mad_sisci_netinterf_init()
{
  return &mad_sisci_netinterf;
}

