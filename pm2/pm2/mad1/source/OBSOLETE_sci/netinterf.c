

/* These defines should be defined in the makefile */
/* #define HEADER */
/* #define DEBUG */
/* #define DMA */

/* HEAD_OF_HEADER *must* be less or equal to PIGGYBACK_AREA_LEN! */
#define HEAD_OF_HEADER       0

/* Size of SCI buffers (bytes) */
#define BUFFER_SIZE (16*1024)

/* Timeout (seconds) for the spawning */
#define TIMEOUT 30

/* Number of retries to map remote segment */
#define RETRIES 3

/* SCI Segments states of life */
#define NOT_INITIALIZED 0
#define SEGMENT_MADE 1
#define SEGMENT_MAPPED 2
#define SEGMENT_MMAPPED 3
#define SEGMENT_DONE 4

#include <sys/netinterf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
/*#include <sys/wait.h>*/
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sciio.h>
#include <scierrno.h>
#include <errno.h>

#ifdef DMA
  #include <priv_io.h>
#endif

#ifdef MARCEL
  #include <pthread.h>
  static pthread_sem_t mutex_send[MAX_MODULES];
#else
  #include <malloc.h>
  #define lock_task()
  #define unlock_task()
  #define tmalloc malloc
  #define tfree free
  #define tfprintf fprintf
#endif

typedef struct tSegment
{
  int *flow;      /* Real address of shared memory segment, 2 ints for flow control. */
  char *base;     /* Base address in memory to access this segment, save flow control. */
  ushort state;   /* State of segment */
  ushort device;  /* SCI device used. */
  int fd;         /* File descriptor used to access the device. */
  int node;       /* Node ID where this segment is connected to. */
  int key;        /* Key to identify this segemnt. */
  uint size;      /* Size in bytes. */
} tSegment;
static tSegment reception[MAX_MODULES]={{NULL,NULL,0,}};
static tSegment envoi[MAX_MODULES]={{NULL,NULL,0,}};

#define SEND_KEY(to) (confsize*pm2self+to)
#define RECV_KEY(from) (confsize*from+pm2self)

#define min(a, b)  ((a)<(b) ? (a) : (b))
#define max(a, b)  ((a)>(b) ? (a) : (b))

static int pm2self=0, confsize=0, current_sender;

static char my_name[128];

/*****************************************************************************/
/* GESTION DES ERREURS ET DEBOGUAGE                                          */
/*****************************************************************************/

typedef void (*tSigHandler)(int);
tSigHandler oldSIGINThandler=SIG_ERR;
tSigHandler oldSIGBUShandler=SIG_ERR;
tSigHandler oldSIGSEGVhandler=SIG_ERR;

#ifdef DEBUG
  void log(char *s, ...)
  {
    va_list l;
    va_start(l,s);
    vfprintf(stderr,s,l);
    fflush(stderr);
    va_end(l);
  }
  #define TRACE(x) do{x;}while(0)
#else
  #define TRACE(x)
#endif

static void erreurSCI(int error)
{
  if(error&SCI_ERR_MASK)
  {
    error&=~SCI_ERR_MASK;
    switch(error)
    {
      case 0: fprintf(stderr,"Unknown remote error: "); break;
      case 0x900: fprintf(stderr,"SCI bus error: "); break;
      case 0x901: fprintf(stderr,"SCI pending error: "); break;
      case 0x902: fprintf(stderr,"SCI generic error: "); break;
      case 0x903: fprintf(stderr,"SCI link timeout error: "); break;
      case 0x904: fprintf(stderr,"SCI exdev error: "); break;
      case 0x905: fprintf(stderr,"SCI remote error: "); break;
      case 0x906: fprintf(stderr,"SCI mbx busy error: "); break;
      case 0x907: fprintf(stderr,"SCI DMA error: "); break;
      case 0x908: fprintf(stderr,"SCI disabled DMA error: "); break;
      case 0x909: fprintf(stderr,"SCI software mbx send failed: "); break;
      case 0x90A: fprintf(stderr,"SCI hardware mbx send failed: "); break;
      case 0xA00: fprintf(stderr,"SCI connection error, has no session: "); break;
      case 0xA01: fprintf(stderr,"SCI connection error, refused session: "); break;
      case 0xA02: fprintf(stderr,"SCI connection error, no remote valid session:"); break;
      case 0xA03: fprintf(stderr,"SCI connection error, disabled session: "); break;
      case 0xA04: fprintf(stderr,"SCI error, node is closed: "); break;
      case 0xA05: fprintf(stderr,"SCI error, node is disabled: "); break;
      case 0xA06: fprintf(stderr,"SCI local master error: "); break;
      case 0xA07: fprintf(stderr,"SCI remote master error: "); break;
      case 0xA08: fprintf(stderr,"SCI error, illegal command received: "); break;
      case 0xA09: fprintf(stderr,"SCI error, illegal command sent: "); break;
      case 0xB00: fprintf(stderr,"SCI connection refused: "); break;
      case 0xB01: fprintf(stderr,"SCI node not responding: "); break;
      case 0xB02: fprintf(stderr,"SCI node connected: "); break;
      case 0xB03: fprintf(stderr,"SCI host unreachable: "); break;
      case 0xB04: fprintf(stderr,"SCI invalid user/key/port: "); break;
      case 0xB05: fprintf(stderr,"SCI invalid remote user/key/port: "); break;
      case 0xB06: fprintf(stderr,"SCI node error: "); break;
      case 0xB07: fprintf(stderr,"SCI remote node error: "); break;
      case 0xB08: fprintf(stderr,"SCI no space left: "); break;
      case 0xB09: fprintf(stderr,"SCI no remote space left: "); break;
      case 0xB0A: fprintf(stderr,"SCI no DMA space left: "); break;
      case 0xB0B: fprintf(stderr,"SCI no remote DMA space left: "); break;
      case 0xC00: fprintf(stderr,"SCI error, not mapped: "); break;
      case 0xC01: fprintf(stderr,"SCI error, is mapped: "); break;
      case 0xD00: fprintf(stderr,"SCI error, isn't initialized: "); break;
      case 0xD01: fprintf(stderr,"SCI parameter error: "); break;
      case 0xD02: fprintf(stderr,"SCI no free vc: "); break;
      case 0xD03: fprintf(stderr,"SCI no remote free vc: "); break;
      case 0xE00: fprintf(stderr,"SCI no local access: "); break;
      case 0xE01: fprintf(stderr,"SCI local resource busy: "); break;
      case 0xE02: fprintf(stderr,"SCI local resource exists: "); break;
      case 0xE03: fprintf(stderr,"SCI no local resource: "); break;
      case 0xE04: fprintf(stderr,"SCI not connected: "); break;
      case 0xE05: fprintf(stderr,"SCI local error: "); break;
      case 0xE06: fprintf(stderr,"SCI error, not a valid node id.: "); break;
      case 0xE07: fprintf(stderr,"SCI error, not supported: "); break;
      case 0xE08: fprintf(stderr,"SCI timeout: "); break;
      case 0xE09: fprintf(stderr,"SCI message size error: "); break;
      case 0xE0A: fprintf(stderr,"SCI error, no local LC access: "); break;
      case 0xE0B: fprintf(stderr,"SCI invalid ATT error: "); break;
      case 0xF00: fprintf(stderr,"SCI link error, no link: "); break;
      case 0xF01: fprintf(stderr,"SCI link error, no remote link: "); break;
      case 0xF02: fprintf(stderr,"SCI error, no such node: "); break;
      case 0xF03: fprintf(stderr,"SCI error, user access is disabled: "); break;
      case 0xF04: fprintf(stderr,"SCI hardware avoids deadlock: "); break;
      case 0xF05: fprintf(stderr,"SCI potential error: "); break;
      case 0xF06: fprintf(stderr,"SCI fenced: "); break;
      case 0xF07: fprintf(stderr,"SCI switch hardware failure: "); break;
      case 0xF08: fprintf(stderr,"SCI switch, wrong blink id.: "); break;
      case 0xF09: fprintf(stderr,"SCI input/output error: "); break;
      case 0xF0A: fprintf(stderr,"SCI bad address: "); break;
      case 0xF0B: fprintf(stderr,"SCI resource busy: "); break;
      case 0xF0C: fprintf(stderr,"SCI invalid argument: "); break;
      case 0xF0D: fprintf(stderr,"SCI no such device or address: "); break;
      case 0xF0E: fprintf(stderr,"SCI file exists: "); break;
      case 0xF0F: fprintf(stderr,"SCI error, would block: "); break;
      case 0xF15: fprintf(stderr,"SCI resource temporarily unavailable, try again: "); break;
      case 0xF16: fprintf(stderr,"SCI out of range error: "); break;
      case 0xF17: fprintf(stderr,"SCI error, function not implemented: "); break;
      case 0xF18: fprintf(stderr,"SCI interrupted function call: "); break;
      default:
        if(error<0x100)
          fprintf(stderr,"Signal %d caught on remote node: ",error);
        else if(error&ESCI_REMOTE_MASK)
          fprintf(stderr,"SCI remote error 0x%X occurred: ",error);
        else
          fprintf(stderr,"SCI error 0x%X occurred: ",error);
    }
  }
}

static void erreur(const char *format, ...)
{
  va_list ap;
  int code=errno, i, k;

  signal(SIGINT,SIG_IGN);
  signal(SIGBUS,SIG_IGN);
  signal(SIGSEGV,SIG_IGN);

  lock_task();
/*
  TRACE( for(i=0;i<confsize;i++)
  {
    fprintf(stderr,"Node #%d reception = {%5d,%5d, \"%s\"}\n",
      i,reception[i].flow[0],reception[i].flow[1],reception[i].base);
    fprintf(stderr,"Node #%d envoi     = {%5d,%5d, \"%s\"}\n",
      i,envoi[i].flow[0],envoi[i].flow[1],envoi[i].base);
  } );
*/
  va_start(ap,format);
  if((code>0)&&(code&SCI_ERR_MASK))
  {
    erreurSCI(code);
    vfprintf(stderr,format,ap);
    fprintf(stderr,"\n");
  }
  else if(code>0)
  {
    char s[1024];
    if(vsnprintf(s,1024,format,ap)==1024) s[1023]='\0';
    errno=code; perror(s);
  }
  else
  {
    vfprintf(stderr,format,ap);
    fprintf(stderr,"\n");
  }
  fprintf(stderr,"\n");
  fflush(stderr);
  va_end(ap);
  if(code>0) k=-code;
  else k=-(-code|SCI_ERR_MASK);
  TRACE(log("Sending code %d (0x%lX) to others\n",-k,-k));
  for(i=0; i<confsize; i++)
    if(envoi[i].state==SEGMENT_DONE && i!=pm2self)
      envoi[i].flow[0]=k;
  TRACE(log("Exiting\n"));
  network_exit();
  if(code>=0) exit(code);
  if(-code==SIGINT&&oldSIGINThandler!=SIG_ERR) oldSIGINThandler(SIGINT);
  if(-code==SIGBUS&&oldSIGBUShandler!=SIG_ERR) oldSIGBUShandler(SIGBUS);
  if(-code==SIGSEGV&&oldSIGSEGVhandler!=SIG_ERR) oldSIGSEGVhandler(SIGSEGV);
  exit(-1);
}

void sighandler(int sig)
{
  errno=-sig;
  if(sig==SIGINT) erreur("Signal SIGINT caught: user interrupt");
  if(sig==SIGBUS) erreur("Signal SIGBUS caught: bus error");
  if(sig==SIGSEGV) erreur("Signal SIGSEGV caught: segmentation fault");
  erreur("Signal %d caught although it couldn't !",sig);
}

/*****************************************************************************/
/* CREATION DES SEGMENTS S.C.I. EN RECEPTION / EN EMISSION                   */
/*****************************************************************************/

static int openSCIdevice(ushort *device)
{
  int fd;
  int i;
  char tmp[15];
  for(i=0; i<256; i++)
  {
    sprintf(tmp,"/dev/SCI/%d",i);
    if(device) *device=i;
    fd=open(tmp,O_RDWR);
    if(fd>0) break;
  }
  if(i>=256) erreur("No free device for SCI segment");
  return fd;
}

void createReceiveSegments(int seg)
{
  int i=0;
  struct Connection cnx;

  TRACE(log("createReceiveSegments(%d)\n",seg));
  reception[seg].state=NOT_INITIALIZED;
  reception[seg].fd=openSCIdevice(&(reception[seg].device));
  if(!reception[seg].fd) erreur("SCI segment device opening");

  cnx.nodeId=reception[seg].node=reception[0].node;
  cnx.userId=reception[seg].key=RECV_KEY(seg);
  cnx.bufferSize=reception[seg].size=BUFFER_SIZE;

  for(i=0; i<RETRIES; i++)
    if(ioctl(reception[seg].fd,MK_SHM_BUF,&cnx)>=0) break;
  if(i>=RETRIES) erreur("ioctl MK_SHM_BUF");
  reception[seg].state=SEGMENT_MADE;

  for(i=0; i<RETRIES; i++)
    if(ioctl(reception[seg].fd,MAP_SHM_BUF,&cnx)>=0) break;
  if(i>=RETRIES) erreur("ioctl MAP_SHM_BUF");
  reception[seg].state=SEGMENT_MAPPED;

  reception[seg].flow=mmap(NULL, cnx.map.size, PROT_READ|PROT_WRITE,
    MAP_PRIVATE, reception[seg].fd, cnx.map.offset);
  if(reception[seg].flow<0) erreur("mmap reception");
  if(reception[seg].flow==NULL) erreur("mmap reception => NULL");
  reception[seg].state=SEGMENT_MMAPPED;

  memset(reception[seg].flow,0,BUFFER_SIZE);
  reception[seg].base=(char *)&reception[seg].flow[2];
  reception[seg].state=SEGMENT_DONE;
}

void createSendSegment(int seg,int node)
{
  int i=0, k=0;
  struct Connection cnx;

  TRACE(log("createSendSegments(%d,%d)\n",seg,node));
  envoi[seg].state=NOT_INITIALIZED;
  envoi[seg].fd=openSCIdevice(&(envoi[seg].device));
  if(!envoi[seg].fd) erreur("openSCIdevice failed");
  cnx.nodeId=envoi[seg].node=node;
  cnx.userId=envoi[seg].key=SEND_KEY(seg);

  for(i=0; i<RETRIES; i++)
  {
    do k=ioctl(envoi[seg].fd, MAP_SHM_BUF, &cnx);
    while(k<0&&SCI_ERR(errno)==ESCI_REMOTE_NO_SUCH_USER_ID);
    if(k>=0) break;
  }
  if(i>=RETRIES) erreur("ioctl MAP_SHM_BUF (node=%d,user=%d)",cnx.nodeId,cnx.userId);
  envoi[seg].state=SEGMENT_MAPPED;

  envoi[seg].flow=mmap(NULL, cnx.map.size, PROT_READ|PROT_WRITE,
    MAP_PRIVATE, envoi[seg].fd, cnx.map.offset);
  envoi[seg].state=SEGMENT_MMAPPED;

  envoi[seg].base=(char *)&envoi[seg].flow[2];
  envoi[seg].state=SEGMENT_DONE;
}

/*****************************************************************************/
/* FONCTIONS DE COMMUNICATION DE BAS NIVEAU                                  */
/*****************************************************************************/

#define DELAY 10000000L
#define MS(t) ((double)t*1000/CLOCKS_PER_SEC)
static const int buffer_free=BUFFER_SIZE-2*sizeof(int);

static inline void llSend(const int receiver, void *packet, const size_t length)
{
  #ifdef DMA
    #define p packet
    #define remain length
  #else
    void *p=packet;
    size_t remain=length;
  #endif
  int size;
  
  #ifdef DEBUG
    clock_t ticks=clock();
  #endif
  
  while(remain>0)
  {
    while(reception[receiver].flow[1]>0);
    lock_task();
    size=reception[receiver].flow[1];
    if(size==0)
    {
      if(remain<=buffer_free) memmove(envoi[receiver].base,p,remain);
      else
      {
        #ifdef DMA
          TRACE(log("      Size %d > %d, sending by DMA\n",remain,buffer_free));
          if(write(envoi[receiver].fd,p,remain)<0)
            erreur("DMA sending packet of %d bytes",remain);
        #else
          TRACE(log("      Remaining size %d > %d, sending this amount instead\n",
            remain,buffer_free));
          memmove(envoi[receiver].base,p,buffer_free);
        #endif
      }
      reception[receiver].flow[1]=remain;
      envoi[receiver].flow[0]=remain;
      unlock_task();
      #ifdef DMA
        TRACE(if(remain>buffer_free) log("      DMA finished\n"));
        break;
      #else
        p+=buffer_free;
        if(remain<=buffer_free) remain=0;
        else remain-=buffer_free;
        TRACE(log("      %d bytes remaining\n",remain));
      #endif
    }
    else
    {
      unlock_task();
      if(size<0) erreur("Remote error while sending");
    }
  }
  
  #ifdef DEBUG
    ticks=clock()-ticks;
  #endif
  TRACE(log("    %d bytes sent in %gms\n",length,MS(ticks)));
}

static inline void llReceive(int *sender, void *packet, const size_t length)
{
  #ifdef DMA
    #define p packet
    #define remain length
  #else
    void *p=packet;
    size_t remain=length;
  #endif
  int size=0;
  int i=0, envoyeur=-1;
  #ifdef DEBUG
    long n=0;
    clock_t ticks=clock();
  #endif
  
  if(sender!=NULL) if(*sender>=0) i=envoyeur=*sender;
  while(remain>0) /* polling */
  {
    TRACE( if(--n<0) { n=DELAY; log("r"); } );
    /* while(reception[i].flow[0]==0) { int n=10000; while(n--); } */
    lock_task();
    size=reception[i].flow[0];
    if(size>0)
    {
      if(envoyeur<0)
      {
        TRACE(log("      Sender found on node #%d\n",i));
        envoyeur=i;
        if(sender!=NULL) *sender=envoyeur;
      }
      
      if(remain<=buffer_free) memmove(p,reception[i].base,remain);
      else
      {
        #ifdef DMA
          TRACE(log("      Expected size %d > %d, receiving by DMA\n",
            remain,buffer_free));
          if(read(reception[i].fd,packet,remain)<0)
            erreur("DMA receiving packet of %d bytes",remain);
        #else
          TRACE(log("      Remaining size %d > %d, receiving this amount instead\n",
            remain,buffer_free));
          memmove(p,reception[i].base,buffer_free);
        #endif
      }
      reception[i].flow[0]=0;
      envoi[i].flow[1]=0;
      unlock_task();
      #ifdef DMA
        if(remain>buffer_free) TRACE(log("      DMA finished\n") );
        break;
      #else
        p+=buffer_free;
        if(remain<=buffer_free) remain=0;
        else remain-=buffer_free;
        TRACE(log("      %d bytes remaining\n",remain));
      #endif
    }
    else
    {
      unlock_task();
      if(size<0) { errno=-size; erreur("node #%d caused an error",i); }
      pthread_givehandback_np();
    }
    if(envoyeur<0) if(++i>=confsize) i=0;
  }
  
  #ifdef DEBUG
    ticks=clock()-ticks;
  #endif
  TRACE(log("    %d bytes received in %gms\n",length,MS(ticks)));
}

/*****************************************************************************/
/* FONCTIONS D'INITIALISATION POUR S.C.I.                                    */
/*****************************************************************************/

static void master(char *argv[])
{
  char execname[1024], cmd[1024], tmp[128];
  int i, nb, timeout=TIMEOUT;

  for(i=0; i<confsize; i++) createReceiveSegments(i);

  /* Spawning des autres */
  if(argv[0][0] != '/')
  {
    getcwd(execname, 1024);
    strcat(execname, "/");
    strcat(execname, argv[0]);
    argv[0] = execname;
  }
  sprintf(cmd, "%s/bin/sci/madspawn %s %d %d",
    getenv("MADELEINE_ROOT"), my_name, confsize, reception[0].node);
  for(i=0;argv[i];i++)
  {
    sprintf(tmp, " %s", argv[i]);
    strcat(cmd, tmp);
  }
  system(cmd);

  /* Attente du lancement des autres */
  envoi[0].node=reception[0].node;
  for(nb=1; nb<confsize; nb++) envoi[nb].node=0;
  for(nb=1; nb<confsize; )
  {
    int trouve=0;
    for(i=1; i<confsize; i++)
    {
      if(envoi[i].node==0&&reception[i].flow[1]!=0)
      {
        envoi[i].node=reception[i].flow[1];
        reception[i].flow[1]=0;
        trouve=i;
      }
    }
    if(trouve) { nb++; timeout=TIMEOUT; }
    else if(timeout==0) { errno=0; erreur("timeout while spawning"); }
    else { sleep(1); timeout--; }
  }

  /* Mapping des autres */
  createSendSegment(0,envoi[0].node);
  for(nb=1; nb<confsize; nb++)
  {
    createSendSegment(nb,envoi[nb].node);
    for(i=1; i<confsize; i++)
      envoi[nb].flow[i+2]=envoi[i].node;
  }
  for(nb=1; nb<confsize; nb++)
    for(i=1; i<confsize; i++)
      while(envoi[nb].flow[i+2])
        if(reception[nb].flow[0]<0)
          erreur("node #%d caused an error",nb);

}

static void slave(int master_node)
{
  int i;

  /* Mapping du maitre */
  createSendSegment(0,master_node);
  envoi[0].flow[1]=reception[0].node;

  for(i=0; i<confsize; i++) createReceiveSegments(i);

  /* Mapping des segments distants */
  for(i=1; i<confsize; i++)
  {
    int timeout=TIMEOUT;
    while(!reception[0].flow[i+2])
      if(timeout==0) { errno=0; erreur("timeout while mapping remote segments"); }
      else { sleep(1); timeout--; }
    createSendSegment(i,reception[0].flow[i+2]);
    reception[0].flow[i+2]=0;
  }
}

/*****************************************************************************/
/* FONCTIONS MADELEINE: network_...                                          */
/*****************************************************************************/

void network_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int i, f;
  char output[64];

  #ifdef DEBUG
    log("Mode DEBUG active'\n");
    #ifdef DMA
      log("Mode DMA active'\n");
    #endif
    #ifdef HEADER
      log("Mode HEADER active'\n");
    #endif
  #endif

  oldSIGINThandler=signal(SIGINT,sighandler);
  if(oldSIGINThandler==SIG_ERR) erreur("SIGINT handler redefinition problem");

  oldSIGBUShandler=signal(SIGBUS,sighandler);
  if(oldSIGBUShandler==SIG_ERR) erreur("SIGBUS handler redefinition problem");

  oldSIGSEGVhandler=signal(SIGSEGV,sighandler);
  if(oldSIGSEGVhandler==SIG_ERR) erreur("SIGSEGV handler redefinition problem");

  setbuf(stdout, NULL);

  /*-------------------------------------------- Recherche de l'id du noeud -*/
  if(!(f=openSCIdevice(NULL))) erreur("network_init: openSCIdevice failed");
  if(ioctl(f,NC_GET_NODE_ID,&(reception[0].node))<0)
    erreur("ioctl GET_NODE_ID");
  TRACE(log("My node Id is %d - errno set to 0x%lX\n",reception[0].node,errno));

/*
 *  #ifdef DEBUG
 *    #ifdef DMA
 *      log("Informations sur les DMA:\n");
 *      if(ioctl(f,GET_DMA_INFO,NULL)<0) erreur("ioctl GET_DMA_INFO");
 *      log("-------------------------\n");
 *    #endif
 *  #endif
 */

  /*---------------------------------------------------------------- Maitre -*/
  if(getenv("MAD_MY_NUMBER") == NULL) /* Master ? */
  {
    dup2(STDERR_FILENO, STDOUT_FILENO);
    pm2self = 0;
    /*confsize = WEXITSTATUS(system("exit `cat ${HOME}/.madconf | wc -w`"));*/
    confsize = system("exit `cat ${HOME}/.madconf | wc -w`");
    {
      FILE *f;

      sprintf(output, "%s/.madconf", getenv("HOME"));
      f = fopen(output, "r");
      if(f == NULL) {
	perror("fichier ~/.madconf");
	exit(1);
      }
      fscanf(f, "%s", my_name); /* first line of .madconf */
      fclose(f);
    }
    if(nb_proc == NETWORK_ASK_USER)
    {
      do
      {
	tfprintf(stderr,"Config. size (1-%d or %d for one proc. per node) : ",
          MAX_MODULES, NETWORK_ONE_PER_NODE);
	scanf("%d", &nb_proc);
      } while((nb_proc < 0 || nb_proc > MAX_MODULES)
            && nb_proc != NETWORK_ONE_PER_NODE);
    }
    if(nb_proc != NETWORK_ONE_PER_NODE) confsize = nb_proc;
    master(argv);
  }
  /*--------------------------------------------------------------- Esclave -*/
  else
  {
    setsid();
    pm2self = atoi(getenv("MAD_MY_NUMBER"));
    confsize = atoi(getenv("MAD_NUM_NODES"));
    strcpy(my_name, getenv("MAD_MY_NAME"));

    sprintf(output, "/tmp/%s-pm2log-%d", getenv("USER"), pm2self);
    /* dup2((f = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0600)), STDOUT_FILENO); */
    dup2(STDOUT_FILENO, STDERR_FILENO);
    /* close(f); */

    slave(atoi(getenv("MAD_MASTER_ID")));
  }

  /*------------------------------- Traitement commun: renvoi des resultats -*/
  *nb = confsize;
  *whoami = pm2self;
  for(i=0; i<confsize; i++)
  {
    tids[i] = i;
    #ifdef MARCEL
      pthread_sem_init(&mutex_send[i], 1) ;
    #endif
  }

  #ifdef DEBUG
    log("Node #%d launched\n",pm2self);
    #ifdef MARCEL
    {
      int sec;
      clock(); lock_task(); sec=clock(); log("lock_task()   => %dms\n",sec);
      clock(); unlock_task(); sec=clock(); log("unlock_task() => %dms\n",sec);
      clock(); pthread_sem_P(mutex_send); sec=clock(); log("pthread_sem_P() => %dms\n",sec);
      clock(); pthread_sem_V(mutex_send); sec=clock(); log("pthread_sem_V() => %dms\n",sec);
    }
    #endif
  #endif
}

void network_send(int dest_node, struct iovec *vector, size_t count)
{
  int i=0;

  TRACE(log("  Sending %d vectors to #%d\n",count,dest_node));
  #ifdef MARCEL
    if (count==0) RAISE(PROGRAM_ERROR);
    pthread_sem_P(&mutex_send[dest_node]);
  #endif
  #ifdef HEADER
    llSend(dest_node,&vector[0].iov_len,sizeof(vector[i].iov_len));
  #endif
  do
  {
    llSend(dest_node,vector[i].iov_base,vector[i].iov_len);
  } while(++i<count);
  #ifdef MARCEL
    pthread_sem_V(&mutex_send[dest_node]);
  #endif
  TRACE(log("\n"));
}

void network_receive(network_handler func)
{
  struct iovec vector;
  #ifdef DEBUG
    clock_t ticks;
  #endif

  current_sender=-1;
  #ifdef HEADER
    llReceive(&current_sender,&vector.iov_len,sizeof(vector.iov_len));
    if(vector.iov_len<=0) vector.iov_len=1;
  #else
    vector.iov_len=8192; /* MAX_HEADER from madeleine.c !!! */
  #endif
  vector.iov_base=tmalloc(vector.iov_len);
  if(vector.iov_base<=NULL)
    erreur("memory allocation failed for %d bytes",vector.iov_len);

  llReceive(&current_sender,vector.iov_base,vector.iov_len);
  TRACE(log("(\"%s\",%d)-{\n",(char *)vector.iov_base,vector.iov_len));
  TRACE(ticks=clock());
  (*func)(vector.iov_base,vector.iov_len);
  TRACE(ticks=clock()-ticks);
  TRACE(log("}-(%gms)\n",MS(ticks)));
  tfree(vector.iov_base);
}

void network_receive_data(struct iovec *vector, size_t count)
{
  int i, envoyeur=current_sender;

  TRACE(log("  Receiving %d vectors from #%d\n",count,envoyeur));
  for(i=0;i<count;i++)
  {
    llReceive(&envoyeur,vector[i].iov_base,vector[i].iov_len);
  }
  TRACE(log("\n"));
}

void network_exit()
{
  int i;
  struct Connection cnx;
  if(oldSIGINThandler!=SIG_ERR) signal(SIGINT,oldSIGINThandler);
  if(oldSIGBUShandler!=SIG_ERR) signal(SIGBUS,oldSIGBUShandler);
  if(oldSIGSEGVhandler!=SIG_ERR) signal(SIGSEGV,oldSIGSEGVhandler);
  for(i=0;i<confsize;i++)
  {
    if(envoi[i].state>=SEGMENT_MAPPED) ioctl(envoi[i].fd,UNMAP_SHM_BUF,&cnx);
    if(envoi[i].state>NOT_INITIALIZED) close(envoi[i].fd);
    if(reception[i].state>=SEGMENT_MAPPED) ioctl(reception[i].fd,UNMAP_SHM_BUF,&cnx);
    if(reception[i].state>=SEGMENT_MADE) ioctl(reception[i].fd,RM_SHM_BUF,&cnx);
    if(reception[i].state>NOT_INITIALIZED) close(reception[i].fd);
  }
}
