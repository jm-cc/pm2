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

 
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <errno.h>

#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sched.h>

#include <unistd.h>
#include <sys/netinterf.h>

#define MAX_SIZE_SEGMENT  8196

#ifdef PM2

#include <marcel.h>

#endif

static int           pm2self  ;
static int           confsize ;
static char          my_name [128] ;
static int           nb_procs ;

typedef struct {
                int tag    ;
                int first  ;
                int total  ;
               } requete ;


static char         *memory [MAX_MODULES] ;

#ifdef PM2
static marcel_sem_t mutex_send           ;
#endif


static int          MAX_BUFFER ;
static char         *buffer [MAX_MODULES] ;

static int          *completed [MAX_MODULES] ;
static int          *received  [MAX_MODULES] ;
static int          *ack       [MAX_MODULES] ;
static requete      *req       [MAX_MODULES] ;


/**********************************************************

              Memoire partagee

***********************************************************/

static char *create_segment (int key, int size)
{
 int   seg_id ;
 char  *p ;

 seg_id = shmget ((key_t) key+200, size, IPC_CREAT|IPC_EXCL|0644) ;

 if (seg_id == -1)
   {
    perror ("create_segment_shmget ") ;
    exit (-1) ;
   }

 p = shmat (seg_id, 0, 0) ;
 if (p == (char *) -1)
   {
    perror ("create_segment_shmat ") ;
    exit (-1) ;
   }
 return p ;
}


static int delete_segment (int key)
{
 int seg_id ;
 int status ;

 seg_id = shmget ((key_t) key+200, MAX_SIZE_SEGMENT, 0644) ;

 if (seg_id == -1)
   {
    perror ("attach_segment_shmget ") ;
    exit (-1) ;
   }

 status = shmctl (seg_id, IPC_RMID, NULL) ;

 if (status == -1)
  {
   perror ("delete_segment_shmctl ") ;
   exit (-1) ;
  }

 return status ;
}

static char *attach_segment (int key)
{
 int  seg_id ;
 char *p ;


 seg_id = shmget ((key_t) key+200, MAX_SIZE_SEGMENT, 0644) ;

 if (seg_id == -1)
   {
    perror ("attach_segment_shmget ") ;
    exit (-1) ;
   }

 p = shmat (seg_id, NULL, 0) ;
 if (p == (char *) -1)
   {
    perror ("create_segment_shmat ") ;
    exit (-1) ;
   }
 return p ;
}

#ifdef DEBUG
static void debug_memory () 
{
 int i, j ;

    for (i=0; i < confsize; i++)
      {
       printf ("completed [%d] %d\n", i, *(completed [i])) ;
       for (j=0; j < confsize; j++)
	 {
          printf ("tag [%d,%d]=%d first %d \n", i, j, (req [i]+j)->tag,  (req [i]+j)->first) ;
	 }
       printf ("===================================\n") ;
      }
}
#endif


/*****************************************************************

            User functions (send, receive, receive_data) 

******************************************************************/

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

static void spawn_procs(char *argv[])
{ 
  char cmd[1024], arg[128];
  int i;

  sprintf(cmd, "%s/bin/shm/madspawn %d", get_mad_root(), 
               confsize);

  i=0;
  while(argv[i]) {
    sprintf(arg, " %s ", argv[i]);
    strcat(cmd, arg);
    i++;
  }

  system(cmd);
}


static void master(char *argv[])
{
  char execname[1024];
  int  i, j;

  for (i = 0; i < confsize; i++)
    {
     memory [i] = create_segment (i, MAX_SIZE_SEGMENT) ;

     completed [i] = (int *) memory [i] ;
     *(completed [i]) = 0 ;

     received  [i] = (int *) (memory [i] + sizeof (int)) ;
     for (j=0; j < confsize; j++)
           *(received [i] + j) = 0 ;

     ack [i] = (int *) (memory [i] + ((confsize * sizeof(int)) + sizeof (int))) ;
     for (j=0; j < confsize; j++)
           *(ack [i] + j) = 0 ;
     

     req   [i] = (requete *) (memory [i] + ((2*confsize)+1) * sizeof (int)) ;
     for (j=0; j < confsize; j++)
           (*(req [i] + j)).tag = 0 ;
      
     buffer [i] = memory [i] + (((2*confsize)+1) * sizeof (int)+ (confsize*sizeof(requete))) ;
    }

  MAX_BUFFER = MAX_SIZE_SEGMENT - (((2*confsize)+1) * sizeof (int)+ (confsize*sizeof(requete))) ;

  if(argv[0][0] != '/') {
    getcwd(execname, 1024);
    strcat(execname, "/");
    strcat(execname, argv[0]);
    argv[0] = execname;
  }
  spawn_procs(argv);
}

static void slave()
{
  int i ;

  /* Etablissement des connections avec les autres */

  for (i = 0 ; i < confsize; i++)
    {
     memory [i] = attach_segment (i) ;

     completed [i] = (int *) memory [i] ;
     received  [i] = (int *) (memory [i] + sizeof (int)) ;
     ack [i]       = (int *) (memory [i] + ((confsize * sizeof(int)) + sizeof (int))) ;
     req   [i]     = (requete *) (memory [i] + ((2*confsize)+1) * sizeof (int)) ;

     buffer [i]    = memory [i] + (((2*confsize)+1) * sizeof (int)+ (confsize*sizeof(requete))) ;
    }

  MAX_BUFFER = MAX_SIZE_SEGMENT - (((2*confsize)+1) * sizeof (int)+ (confsize*sizeof(requete))) ;
}

void mad_shm_network_init (int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int i, f;
  char output[64];
  int  status ;

#ifdef IRIX_SYS
  nb_procs = sysconf (_SC_NPROC_CONF) ;
#elif LINUX_SYS
  nb_procs = sysconf (_SC_NPROCESSORS_CONF) ;
#else
  nb_procs = 1 ;
#endif



  if(getenv("MAD_MY_NUMBER") == NULL) { /* master process */

    pm2self = 0;

    setbuf(stdout, NULL);
    dup2(STDERR_FILENO, STDOUT_FILENO);

    status = system("exit `cat ${MAD1_ROOT-${PM2_ROOT}/mad1}/.madconf | wc -w`") ;
    confsize = WEXITSTATUS(status); 


    {
      FILE *f;

      sprintf(output, "%s/.madconf", get_mad_root());
      f = fopen(output, "r");
      if(f == NULL) {
	perror("fichier madconf");
	exit(1);
      }
      fscanf(f, "%s", my_name); /* first line of .madconf */
      fclose(f);
    }

    if(nb_proc == NETWORK_ASK_USER) {
      do {
#ifdef PM2
	tprintf("Config. size (1-%d or %d for one proc. per node) : ", MAX_MODULES, NETWORK_ONE_PER_NODE);
#else
        printf("Config. size (1-%d or %d for one proc. per node) : ", MAX_MODULES, NETWORK_ONE_PER_NODE);
#endif
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

    pm2self = atoi(getenv("MAD_MY_NUMBER"));
    confsize = atoi(getenv("MAD_NUM_NODES"));
    strcpy(my_name, getenv("MAD_MY_NAME"));

    sprintf(output, "/tmp/%s-pm2log-%d", getenv("USER"), pm2self);
    dup2((f = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0600)), STDOUT_FILENO);
    dup2(STDOUT_FILENO, STDERR_FILENO);
    close(f);

#ifdef DEBUG
    fprintf(stderr, "Process %d started on %s\n", pm2self, my_name);
#endif

    slave();
  }

  *nb = confsize;
  *whoami = pm2self;
  for(i=0; i<confsize; i++)
    tids[i] = i;

#ifdef PM2
 marcel_sem_init (&mutex_send, 1) ;
#endif
}

void mad_shm_network_exit ()
{
 int i ;

 if (pm2self == 0)
   {
    for (i=0; i < confsize; i++)
      {
       delete_segment (i) ;
      }
   }
}


void mad_shm_network_send (int dest_node, struct iovec *vector, size_t count)
{
 int      i       = 0 ;
 int      total   = 0 ;
 int      current = 0 ;
 int      l       = 0 ;
 requete *preq        ;
 int     *pack        ;
 int     *prec        ;
#ifdef PM2
 marcel_t curtask = marcel_self();
#endif

#ifdef PM2
 marcel_sem_P (&mutex_send) ;
#endif

   for (i=0; i < count; i++) total += vector[i].iov_len ;

   preq = req [dest_node] + pm2self ;
   preq->first  = vector[0].iov_len ;
   preq->total  = total ;
   preq->tag    = 1 ; 

   pack = ack [pm2self]+dest_node ;

#ifdef PM2
   while (*pack == 0) 
     {
       if (nb_procs > 1)
          marcel_givehandback () ;
       else
	 {
	   lock_task () ;
	   if (curtask->next == curtask)
	     {
	       unlock_task () ;
	       usleep (1) ; 
	     }
	   else
	     {
	       unlock_task () ;
	       marcel_givehandback();
	     }
	 }
     }
#else
   while (*pack == 0) sched_yield ();
#endif

   *pack = 0 ;

   i = 0 ;
   while (l < total)
    {
     if ((current + vector [i].iov_len) < MAX_BUFFER)
       {
        memcpy (
                buffer [dest_node]+current, 
                vector [i].iov_base, 
                vector[i].iov_len
               ) ;

        l += vector [i].iov_len ; 
        current += vector [i].iov_len ;
        i++ ;
       }      
     else
       {
        /* vecteur a cheval sur plusieurs segments consecutifs */
        int lv = 0 ;

        while (lv < vector [i].iov_len)
	  {               

           if ((vector [i].iov_len - lv) < (MAX_BUFFER - current))
	     {
              memcpy (
                      buffer [dest_node]+current,
                      vector [i].iov_base+lv,
                      vector [i].iov_len - lv
                  ) ;

              l  += (vector [i].iov_len - lv) ;
              current += (vector [i].iov_len - lv) ;
              lv += (vector [i].iov_len - lv) ;
              i++ ;
	     }
            else
	      {
               memcpy (
                       buffer [dest_node]+current,
                       vector [i].iov_base+lv,
                       MAX_BUFFER - current                     
                      ) ;

               lv += (MAX_BUFFER - current) ;
               l  += (MAX_BUFFER - current) ;

               *(completed [dest_node]) = 1 ;

               prec = received [pm2self]+dest_node ;
#ifdef PM2
               while (*prec == 0) 
		 {
		   if (nb_procs > 1)
		     marcel_givehandback () ;
		   else
		     {
		       lock_task () ;
		       if (curtask->next == curtask)
			 {
			   unlock_task () ;
			   usleep (1) ; 
			 }
		       else
			 {
			   unlock_task () ;
			   marcel_givehandback();
			 }
		     }
		 }
#else
               while (*prec == 0) sched_yield () ;
#endif

               *prec = 0 ;
               current = 0 ;               
	      }
          }
       } 
    }

  *(completed [dest_node]) = 1 ;

#ifdef PM2
 marcel_sem_V (&mutex_send) ;
#endif
}

static  int current_sender   ;
static  int total_to_receive ;
static  int first_iovec_len  ;

void mad_shm_network_receive_data (struct iovec *vector, size_t count)
{
 int    i       = 0 ;
 int    l       = first_iovec_len;
 int    current = l ;
 int    *pcomp      ;
#ifdef PM2
 marcel_t curtask = marcel_self();
#endif

 while (l < total_to_receive)
   {
    if ((current + vector [i].iov_len) < MAX_BUFFER)
      {
        memcpy (
                vector [i].iov_base, 
                buffer [pm2self]+current, 
                vector[i].iov_len
               ) ;
        l += vector [i].iov_len ; 
        current += vector [i].iov_len ;
        i++ ;
      }
     else
      {
       /* vecteur a cheval sur plusieurs segments consecutifs */
       int lv = 0 ;

       while (lv < vector [i].iov_len)
	  {
           if ((vector [i].iov_len - lv) < (MAX_BUFFER - current))
	     {
              memcpy (
                      vector [i].iov_base+lv,
                      buffer [pm2self]+current,
                      vector [i].iov_len - lv
                  ) ;

              l  += (vector [i].iov_len - lv) ;
              current += (vector [i].iov_len - lv) ;
              lv += (vector [i].iov_len - lv) ;
              i++ ;
	     }
            else
	      {
               memcpy (
                       vector [i].iov_base+lv,
                       buffer [pm2self]+current,
                       MAX_BUFFER - current                     
                      ) ;

               lv += (MAX_BUFFER - current) ;
               l  += (MAX_BUFFER - current) ;

               *(received [current_sender] + pm2self) = 1 ;

               pcomp = completed [pm2self] ;
#ifdef PM2
               while (*pcomp == 0) 
		 {
		   if (nb_procs>1)
		     marcel_givehandback () ;
		   else
		     {
		       lock_task () ;
		       if (curtask->next == curtask)
			 {
			   unlock_task () ;
			   usleep (1) ;
			 }
		       else
			 {
			   unlock_task () ;
			   marcel_givehandback();
			 }
		     }
		 }
#else
               while (*pcomp == 0) sched_yield () ;
#endif

               *pcomp = 0 ;
               current = 0 ;
	      }
          }
       } 
   }
}

void mad_shm_network_receive (char **head)
{
 int     found = 0 ;
 int     i         ;
 int     *pcomp    ;
 requete *preq     ;
 static  int last = 0 ;
 int     first ;
#ifdef PM2
 marcel_t curtask = marcel_self();
#endif

  first = (last+1) % confsize ;

  while (found == 0)
   {
    i     = first ;
    do
      {
       if ((req[pm2self] + i)->tag == 0)
         i = (i + 1) % confsize ;
        else
         found = 1 ;
      } while ((i != first) & (found == 0)) ;

    if (found == 1)
      {
       current_sender   = i ;
       preq = req[pm2self] + i ;
       first_iovec_len  = preq->first ;
       total_to_receive = preq->total ;
       preq->tag = 0 ; 
      }
#ifdef PM2
     else 
       {
	 if (nb_procs > 1)
	   marcel_givehandback () ;
	 else
	   {
	     lock_task () ;
	     if (curtask->next == curtask)
	       {
		 unlock_task () ;
		 usleep (1) ; 
	       }
	     else
	       {
		 unlock_task () ;
		 marcel_givehandback();
	       }
	   }
       }
#endif
   }
   
 *(ack[current_sender]+pm2self) = 1 ;

 pcomp = completed [pm2self] ; 
#ifdef PM2
 while (*pcomp == 0) 
   {
     if (nb_procs > 1)
       marcel_givehandback () ;
     else 
       {
	 lock_task () ;
	 if (curtask->next == curtask)
	   {
	     unlock_task () ;
	     usleep (1) ; 
	   }
         else
	   {
	     unlock_task () ;
	     marcel_givehandback();
	   }
       }
   }
#else
 while (*pcomp == 0) sched_yield ();
#endif
 *pcomp = 0 ;

 /* (*func) (buffer [pm2self], first_iovec_len) ; */
 *head = buffer [pm2self] ;
}

static netinterf_t mad_shm_netinterf = {
  mad_shm_network_init,
  mad_shm_network_send,
  mad_shm_network_receive,
  mad_shm_network_receive_data,
  mad_shm_network_exit
};

netinterf_t *mad_shm_netinterf_init()
{
  return &mad_shm_netinterf;
}








