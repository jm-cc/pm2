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

/*********** include sbf header files ******************/

#include <dedbuff.h>
#include <sbp.h>
#include <sbp_messages.h>

/*******************************************************/

#include "mad_timing.h"

#ifdef PM2
#include "marcel.h"
#endif

#include "sys/netinterf.h"

#define SBP_SMALL_SIZE   (MAX_CS_USER_MESSAGE/sizeof(int))
#define SBP_HEADER_SIZE  (sizeof(SbpHeader_t)+sizeof(MsgHeader_t)+sizeof(int))

#define nb_words(x)      x/sizeof(int)+((x%sizeof(int))?1:0)

enum                     {REQUEST, ACK, DATA} ;

static int               sender_node ;
static int               small_iov_base [SBP_SMALL_SIZE] ;
static int	         *large_iov_base = NULL;

static DediMsgBuffer_t   **waiting_requests ;

static int               sbp_mynode  ;
static int               sbp_numnodes ;

static int               wait_ack_node = 0 ;
#ifdef PM2
static marcel_t         receiver_thread ;
static marcel_sem_t     *mutex_send  ;
static marcel_sem_t     *waiting_ack ;
#endif

static unsigned long     in_buffer_key  = 0 ;
static unsigned long     out_buffer_key = 0 ;

/*****************************************************************

            Low Level functions (send, receive, polling) 

******************************************************************/

static DediMsgBuffer_t *get_sbpfullbuffer ()
{
 DediMsgBuffer_t *in_buffer ;

 in_buffer = 0 ;

#ifdef PM2
 lock_task () ;
#endif

   in_buffer = (DediMsgBuffer_t *) pickupfullbuffer (in_buffer_key, 0) ;

#ifdef PM2
 unlock_task () ;
#endif

 while (in_buffer == 0)
    {
#ifdef PM2
     marcel_givehandback_np () ;
#endif

#ifdef PM2
     lock_task () ;
#endif

        in_buffer = (DediMsgBuffer_t *) pickupfullbuffer (in_buffer_key,0) ;

#ifdef PM2
     unlock_task () ;
#endif
    }
 return in_buffer ;
}

static DediMsgBuffer_t *get_sbpemptybuffer ()
{
 DediMsgBuffer_t *out_buffer ;

 out_buffer = 0 ;

#ifdef PM2
 lock_task () ;
#endif

  out_buffer = (DediMsgBuffer_t *) pickupemptybuffer (out_buffer_key, 0) ;

#ifdef PM2
 unlock_task () ;
#endif

 while (out_buffer == 0)
  {      
#ifdef PM2
   marcel_givehandback_np () ;
#endif

#ifdef PM2
   lock_task () ;
#endif

     out_buffer = (DediMsgBuffer_t *) pickupemptybuffer(out_buffer_key, 0) ;

#ifdef PM2
   unlock_task () ;
#endif
  }
 return out_buffer ;
}

static void send_sbpfullbuffer (DediMsgBuffer_t *out_buffer)
{
 int status ;

#ifdef PM2
 lock_task () ;
#endif

    status = returnfullbuffer(out_buffer_key, (unsigned long)out_buffer) ;

#ifdef PM2
 unlock_task () ;
#endif

 if (status == 0)
   {
    fprintf (stderr, "Error send_to_network sender %d\n", sbp_mynode) ;
    exit (-1) ;
   }
}

static void release_sbpemptybuffer (DediMsgBuffer_t *in_buffer)
{
 int status ;

#ifdef PM2
 lock_task () ;
#endif

    status = returnemptybuffer (in_buffer_key, (unsigned long) in_buffer) ;

#ifdef PM2
 unlock_task () ;
#endif

 if (status == 0) 
   {
    fprintf (stderr, "Error  %d release_sbpemptybuffer\n", sbp_mynode) ;
    exit (-1) ;
   }
}

static int receive_from_network (int *src, int tag, int *buff, int buff_len)
{
 int             received = 0  ;
 int             i, found ;
 DediMsgBuffer_t *in_buffer = 0 ;
 int             n = 0 ;
 int             sender ;

 switch (tag)
   {
    case ACK :
              found = 0 ;
              while (found == 0)
   	        {
                 in_buffer = get_sbpfullbuffer () ;
                 sender = in_buffer->Msg.header.origin ;

                 if (in_buffer->Msg.mtype == ACK)
		    {
                     release_sbpemptybuffer (in_buffer) ;
                     if (sender == wait_ack_node)
                       found = 1 ;
#ifdef PM2
                      else
                       marcel_sem_V (&waiting_ack [sender]) ;
#endif
		    }           
                   else
     	            {
                     sender = in_buffer->Msg.header.origin ;
                     waiting_requests [sender] = in_buffer ;
		    }        
		}
             *src = sender ;
             return 1 ;

    case REQUEST:
              found = 0 ; i = 0 ;
              while ((i < sbp_numnodes) && (found == 0))
		{
                 if (waiting_requests [i] != 0)
                    found = 1 ;
                  else
                    i++ ;
	 	}
              if (found == 1)
		{
                 in_buffer = waiting_requests [i] ;
                 waiting_requests [i] = 0 ;

                 *src = in_buffer->Msg.header.origin ;
                 received = in_buffer->Msg.header.sbplength - SBP_HEADER_SIZE ;
                 memcpy (buff, in_buffer->Msg.buf, received) ;

                 release_sbpemptybuffer (in_buffer) ;

                 return (received/sizeof (int)) ; 
                }
               else
        	{
                 while (found == 0)
		   {
                    in_buffer = get_sbpfullbuffer () ;

                    if (in_buffer->Msg.mtype == ACK)
		      {
#ifdef PM2
                       int origin ;

                       origin = in_buffer->Msg.header.origin ;
                       marcel_sem_V (&waiting_ack[origin]) ;
#endif

                       release_sbpemptybuffer (in_buffer) ;
		      }
                     else 
	              {
                       if (in_buffer->Msg.mtype == REQUEST)
			 {
                          found = 1 ;
                         }
                        else
			 {
                          fprintf (stderr,"Error on node %d in message type %ld\n", sbp_mynode,in_buffer->Msg.mtype) ;
                          exit (-1) ;
			 }
		      }
		   }
                 *src = in_buffer->Msg.header.origin ;
                 received = in_buffer->Msg.header.sbplength - SBP_HEADER_SIZE ;
                 memcpy (buff, in_buffer->Msg.buf, received) ;

                 release_sbpemptybuffer (in_buffer) ;

                 return (received/sizeof(int)) ;
		}
    case DATA:
              for (n=0; n < buff_len;)
		{
                 in_buffer = get_sbpfullbuffer () ;

                 if (in_buffer->Msg.mtype == ACK)
	           {
#ifdef PM2
                    int origin ;
 
                    origin = in_buffer->Msg.header.origin ;
                    marcel_sem_V (&waiting_ack[origin]) ;
#endif

                    release_sbpemptybuffer (in_buffer) ;
		   }
                  else
		   {
                    if (in_buffer->Msg.mtype == REQUEST)
		      {
                       int sender ;

                       sender = in_buffer->Msg.header.origin ;
                       waiting_requests [sender] = in_buffer ;
		      }
                     else
		      {
		       /* on a forcement un buffer de donnees */
                       int l ;

                       l = in_buffer->Msg.header.sbplength - SBP_HEADER_SIZE ;
                       *src = in_buffer->Msg.header.origin ;

                       memcpy (((char *)buff)+(n*sizeof(int)), in_buffer->Msg.buf, l) ;

                       release_sbpemptybuffer (in_buffer) ;

                       n += (l/sizeof(int)) ;
		      }
		   }
		}
            return n ;
   default:
           fprintf (stderr, "erreur dans le tag du receive from network %d\n", tag) ;
           exit (-1) ;
           return 0 ;
   }
}

static void send_to_network (int dest, int tag, int *buff, int buff_len)
{
 DediMsgBuffer_t *out_buffer ;
 int             n ;

 for (n=0; n <buff_len;)
   {
    out_buffer = get_sbpemptybuffer () ;

    out_buffer->Msg.header.destn_node    = dest ;
    out_buffer->Msg.header.origin        = sbp_mynode ;

    if ((buff_len - n) <= SBP_SMALL_SIZE)
      {
       out_buffer->Msg.header.dataLen = (buff_len - n) * sizeof (int);
       memcpy (out_buffer->Msg.buf, ((char *)buff)+(n*sizeof(int)), out_buffer->Msg.header.dataLen) ;
       n = buff_len ;
      }
    else
      {
       out_buffer->Msg.header.dataLen = SBP_SMALL_SIZE * sizeof (int) ;
       memcpy (out_buffer->Msg.buf, ((char *)buff)+(n*sizeof(int)),out_buffer->Msg.header.dataLen) ;
       n += SBP_SMALL_SIZE ;
      }

    out_buffer->Msg.header.sbplength     = out_buffer->Msg.header.dataLen + SBP_HEADER_SIZE ;
    out_buffer->Msg.mtype                = tag ;

    send_sbpfullbuffer (out_buffer) ;
   }
}

/*****************************************************************

            User functions (send, receive, receive_data) 

******************************************************************/

void mad_sbp_network_init (int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int i, f;
  char output[64];

  sbp_initialize (
                  0,
                  0, 
                  sizeof(struct Dedi_Msg_Buffer), 
                  nb,
 		  whoami, 
                  &in_buffer_key, 
                  &out_buffer_key
                 );

  sbp_mynode   = *whoami ;
  sbp_numnodes = *nb ;

  if(sbp_mynode) {
#ifdef PM2
      sprintf(output, "/tmp/%s-pm2log-%d", "pm2_sbp", sbp_mynode);
#else
      sprintf(output, "/tmp/%s-madlog-%d", "pm2_sbp", sbp_mynode);
#endif
    dup2((f = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0600)), STDOUT_FILENO);
    dup2(STDOUT_FILENO, STDERR_FILENO);
    close(f);
  }

#ifdef PM2
  mutex_send = (marcel_sem_t *) malloc ((*nb) * sizeof (marcel_sem_t)) ;
  waiting_ack = (marcel_sem_t *) malloc ((*nb) * sizeof (marcel_sem_t)) ;
  for (i=0; i < *nb; i++) 
    {
     marcel_sem_init (&mutex_send [i], 1) ;
     marcel_sem_init (&waiting_ack [i], 0) ;
    }
#endif

#ifdef PM2
  waiting_requests = (DediMsgBuffer_t **) tmalloc (*nb * sizeof (DediMsgBuffer_t *)) ;
#else
  waiting_requests = (DediMsgBuffer_t **) malloc (*nb * sizeof (DediMsgBuffer_t *)) ;
#endif

  for (i = 0 ; i < *nb ; i ++)
    {
      tids [i] = i ;
      waiting_requests [i] = 0 ;
    }
}

void mad_sbp_network_exit ()
{
 int i ;
 int status ;

 for (i = 0; i < sbp_numnodes; i++)
   {
    if (waiting_requests [i] != 0)
      { 
#ifdef PM2
       lock_task () ;
#endif

          status = returnemptybuffer (in_buffer_key, (unsigned long) waiting_requests [i]) ;

#ifdef PM2
       unlock_task () ;
#endif
      }
   }
}


void mad_sbp_network_send (int dest_node, struct iovec *vector, size_t count)
{
 int reply_node    ;
 int ack_message   ;
 int i             ;
 int first_vec_len ;

 if (count == 0)
#ifdef PM2
   RAISE(PROGRAM_ERROR) ;
#else
   {
    fprintf (stderr, "Error in network send zero number of parameters %d\n", count) ;
    exit (-1) ;
   }
#endif


#ifdef PM2
 marcel_sem_P (&mutex_send [dest_node]) ;
#endif

 first_vec_len = nb_words (vector[0].iov_len) ;
 
 if (first_vec_len < SBP_SMALL_SIZE)
   {
    send_to_network (
                     dest_node, 
                     REQUEST, 
                     (int *) vector[0].iov_base, 
                     first_vec_len
                    ) ;
   }
  else
   {
    int sauve ;

    sauve = ((int *) vector[0].iov_base) [SBP_SMALL_SIZE-1] ; 
    ((int *) vector[0].iov_base) [SBP_SMALL_SIZE-1] = first_vec_len ;

    send_to_network (
                     dest_node, 
                     REQUEST, 
                     (int *) vector[0].iov_base, 
                     SBP_SMALL_SIZE
                    ) ;

#ifdef PM2
    if (receiver_thread != marcel_self ())
      {
       marcel_sem_P (&waiting_ack[dest_node]) ;

      }
     else
#endif
       {
       wait_ack_node = dest_node ;

       receive_from_network (
                             &reply_node, 
                             ACK, 
                             (int *) &ack_message, 
                             1
                            ) ;
       }

   ((int *) vector[0].iov_base) [SBP_SMALL_SIZE-1] = sauve ;
   send_to_network (
                     dest_node, 
                     DATA, 
                     &((int *) vector[0].iov_base) [SBP_SMALL_SIZE-1], 
                     first_vec_len-SBP_SMALL_SIZE+1
                    ) ;
   }

 if (count > 1)
   {
#ifdef PM2
    if (receiver_thread != marcel_self ())
      {
       marcel_sem_P (&waiting_ack [dest_node]) ;
      }
    else
#endif
      {
       wait_ack_node = dest_node ;

       receive_from_network (
                             &reply_node, 
                             ACK,
                             (int *) &ack_message, 
                             1
                            ) ;
      }
    
    for (i=1; i < count ; i++)
      {
       send_to_network (
                        dest_node, 
                        DATA, 
                        vector[i].iov_base, 
                        nb_words (vector[i].iov_len)
                       ) ;

      }
  }
#ifdef PM2
 marcel_sem_V (&mutex_send[dest_node]) ;
#endif

}

void mad_sbp_network_receive_data (struct iovec *mvec, size_t count)
{
 int  ack_message ;
 int  i ;
 int  sender ;

 send_to_network (
                  sender_node,
                  ACK,
                  (int *) &ack_message,
                  1
                 ) ;

 for (i=0; i < count ; i++)
   {
    int l ;

    l = receive_from_network (
                          &sender, 
                          DATA, 
                          mvec[i].iov_base, 
                          nb_words (mvec[i].iov_len)
                         ) ;
   }
}

void mad_sbp_network_receive (network_handler func)
{
 int   ack_message   ;
 int   first_vec_len ;

#ifdef PM2
 receiver_thread = marcel_self () ;
#endif

 first_vec_len = receive_from_network (
		                     &sender_node,
		                     REQUEST,
		                     small_iov_base,
		                     SBP_SMALL_SIZE
                                    ) ;

 if (first_vec_len < SBP_SMALL_SIZE)
    (*func) (small_iov_base, first_vec_len*sizeof(int)) ;
   else
    {
     send_to_network (
                      sender_node,
                      ACK,
                      (int *) &ack_message,
                      1
                     ) ;

    first_vec_len = ((int *) small_iov_base) [SBP_SMALL_SIZE-1] ;

#ifdef PM2
    large_iov_base = tmalloc (first_vec_len*sizeof(int)) ;
#else
    large_iov_base = malloc (first_vec_len*sizeof(int)) ;
#endif

    memcpy (large_iov_base, small_iov_base, (SBP_SMALL_SIZE-1)*sizeof(int)) ;

    receive_from_network (
                          &sender_node,
		          DATA,
                          &((int *) large_iov_base) [SBP_SMALL_SIZE-1],
                          first_vec_len-SBP_SMALL_SIZE+1
                         ) ;
    (*func) (large_iov_base, first_vec_len*sizeof(int)) ;
#ifdef PM2
    tfree (large_iov_base) ;
#else
    free (large_iov_base) ;
#endif

    }
}

static netinterf_t mad_sbp_netinterf = {
  mad_sbp_network_init,
  mad_sbp_network_send,
  mad_sbp_network_receive,
  mad_sbp_network_receive_data,
  mad_sbp_network_exit
};

netinterf_t *mad_sbp_netinterf_init()
{
  return &mad_sbp_netinterf;
}

