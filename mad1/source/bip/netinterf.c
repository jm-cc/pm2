 
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/time.h>

#include <bip.h>
#include "sys/netinterf.h"
#include "mad_timing.h"

#include "tbx.h"

#ifndef MARCEL
#define lock_task()
#define unlock_task()
#endif

#define nb_words(x) x/sizeof(int)+((x%sizeof(int))?1:0)
enum {
  REQUEST,
  ACK,
  DATA,
  CREDITS
};

static int              sender_node ;
static int              small_iov_base [BIPSMALLSIZE] ;
static int	        *large_iov_base = NULL;


#ifdef PM2
static marcel_sem_t    *mutex_send ;

static marcel_sem_t    *mes_credits ;


static int              *credits_a_rendre ;


#define MAX_CREDITS     10
static int              status_credits ;
static int              credits_rendus ;

#endif


/*****************************************************************

            Low Level functions (send, receive, polling) 

******************************************************************/

static int receive_from_network (int *src, int tag, int *buff, int buff_len)
{
 int status    ;
 int received  ;
#ifdef PM2
 int received_credits ;
 int src_credits ;
 static void update_credits (int) ;
#endif

 lock_task () ;
   status = bip_tirecv (tag, buff, buff_len) ;
   received = bip_rtestx (status, src) ;
 unlock_task () ;

 while (received == -1)
   { 
#ifdef PM2
    lock_task () ;
      received_credits = bip_rtestx (status_credits, &src_credits) ;
    unlock_task () ;

    if (received_credits != -1)
       update_credits (src_credits) ;
    marcel_yield () ;
#endif
    lock_task () ;
          received = bip_rtestx (status, src) ;
    unlock_task () ;
   }

 return received ;
}

static void send_to_network (int dest, int tag, int *buff, int buff_len)
{
 int status ;
 int cr ;

 lock_task () ;
   status = bip_tisend (dest, tag, buff, buff_len) ;
   cr = bip_stest (status) ;
 unlock_task () ;   


 while (cr == 0)
    {
#ifdef PM2
     marcel_yield () ;
#endif
     lock_task () ;
        cr = bip_stest (status) ;
     unlock_task () ;   
    }
}


/*****************************************************************

            User functions (send, receive, receive_data) 

******************************************************************/

void mad_bip_network_init (int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int i, f;
  char output[64];

  /* bip_taginit (REQUEST, 10, BIPSMALLSIZE) ; 
  bip_taginit (ACK, 10, 1) ;  */

  bip_init () ;

  if(bip_mynode) {
#ifdef PM2
    sprintf(output, "/tmp/%s-pm2log-%d", getenv("USER"), bip_mynode);
#else
    sprintf(output, "/tmp/%s-madlog-%d", getenv("USER"), bip_mynode);
#endif
    dup2((f = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0600)), STDOUT_FILENO);
    dup2(STDOUT_FILENO, STDERR_FILENO);
    close(f);
  }

  *whoami = bip_mynode ;
  *nb     = bip_numnodes ;

#ifdef PM2
  mutex_send = (marcel_sem_t *) malloc ((*nb) * sizeof (marcel_sem_t)) ;
  mes_credits = (marcel_sem_t *) malloc ((*nb) * sizeof (marcel_sem_t)) ;

  credits_a_rendre = (int *) malloc ((*nb) * sizeof(int)) ;
#endif

  for (i = 0 ; i < *nb ; i ++)
    {
      tids [i] = i ;
#ifdef PM2
      marcel_sem_init (&mutex_send [i], 1) ;
      marcel_sem_init (&mes_credits [i], MAX_CREDITS) ;
      credits_a_rendre [i] = 0 ;
#endif
    }

#ifdef PM2
  status_credits = bip_tirecv (CREDITS, &credits_rendus, 1) ;
#endif
}

void mad_bip_network_exit ()
{
#ifdef PM2
 int i ;
 int src_credits ;
 int received_credits ;

 static void update_credits (int) ;

 for (i=0; i < bip_numnodes ; i++)
   {
    if (mes_credits[i].value == 0)
      {
       src_credits = -1 ;
       received_credits = bip_rtestx (status_credits, &src_credits) ;
       while (src_credits != i)
	 {
          if (received_credits != -1)
            update_credits (src_credits) ;

          received_credits = bip_rtestx (status_credits, &src_credits) ; 
	 }               
      }
   }
#endif

}


void mad_bip_network_send (int dest_node, struct iovec *vector, size_t count)
{
 int reply_node    ;
 int ack_message   ;
 int i             ;
 int first_vec_len ;
 static void check_send (int, int *) ;
 static void check_receive (int, int *) ;

#ifdef PM2
 if (count == 0)
   MARCEL_EXCEPTION_RAISE(PROGRAM_ERROR) ;

 marcel_sem_P (&mutex_send [dest_node]) ;
#endif

 TIMING_EVENT("network_send'begin");

 first_vec_len = nb_words (vector[0].iov_len) ;
 
 if (first_vec_len < BIPSMALLSIZE)
   {
    check_send (dest_node, (int *) vector[0].iov_base) ;

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

    sauve = ((int *) vector[0].iov_base) [BIPSMALLSIZE-1] ; 
    ((int *) vector[0].iov_base) [BIPSMALLSIZE-1] = first_vec_len ;

    check_send (dest_node, (int *) vector[0].iov_base) ;

    send_to_network (
                     dest_node, 
                     REQUEST, 
                     (int *) vector[0].iov_base, 
                     BIPSMALLSIZE
                    ) ;

    receive_from_network (
                          &reply_node, 
                          ACK, 
                          (int *) &ack_message, 
                          1
                         ) ;
    check_receive (reply_node, &ack_message) ;

   ((int *) vector[0].iov_base) [BIPSMALLSIZE-1] = sauve ;
   send_to_network (
                     dest_node, 
                     DATA, 
                     &((int *) vector[0].iov_base) [BIPSMALLSIZE-1], 
                     first_vec_len-BIPSMALLSIZE+1
                    ) ;
   }

 if (count > 1)
   {
    receive_from_network (
                          &reply_node, 
                          ACK,
                          (int *) &ack_message, 
                          1
                         ) ;

    check_receive (reply_node, &ack_message) ;

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

 TIMING_EVENT("network_send'end");

#ifdef PM2
 marcel_sem_V (&mutex_send [dest_node]) ;
#endif
}

void mad_bip_network_receive_data (struct iovec *mvec, size_t count)
{
 int  ack_message ;
 int  i ;
 int  sender ;
 static void check_send (int, int *) ;

if (count != 0)
  {
     check_send (sender_node, &ack_message) ;


     send_to_network (
                    sender_node,
                    ACK,
                    (int *) &ack_message,
                    1
                   ) ;

     for (i=0; i < count ; i++)
      {
       receive_from_network (
                           &sender, 
                           DATA, 
                           mvec[i].iov_base, 
                           nb_words (mvec[i].iov_len)
                          ) ;
      }
  }
}

void mad_bip_network_receive (char **head)
{
 int   ack_message   ;
 int   first_vec_len ;
 static void  check_send (int, int *) ;
 static void  check_receive (int, int *) ;

 first_vec_len = receive_from_network (
		                     &sender_node,
		                     REQUEST,
		                     small_iov_base,
		                     BIPSMALLSIZE
                                    ) ;

 check_receive (sender_node, (int *) small_iov_base) ;

 if (first_vec_len < BIPSMALLSIZE)
   /*   (*func) (small_iov_base, first_vec_len*sizeof(int)) ; */
   *head = (char *)small_iov_base;
   else
    {

     check_send (sender_node, &ack_message) ;

     send_to_network (
                      sender_node,
                      ACK,
                      (int *) &ack_message,
                      1
                     ) ;

    first_vec_len = ((int *) small_iov_base) [BIPSMALLSIZE-1] ;
    large_iov_base = TBX_MALLOC (first_vec_len*sizeof(int)) ;
    memcpy (large_iov_base, small_iov_base, (BIPSMALLSIZE-1)*sizeof(int)) ;

    receive_from_network (
                          &sender_node,
		          DATA,
                          &((int *) large_iov_base) [BIPSMALLSIZE-1],
                          first_vec_len-BIPSMALLSIZE+1
                         ) ;
    /* TODO: Free large_iov_base at the beginning of receive_data
    (*func) (large_iov_base, first_vec_len*sizeof(int)) ;
    TBX_FREE (large_iov_base) ;
    */
    *head = (char *)large_iov_base;
    }
}

/*****************************************************************

            Gestion de flux, credits 

******************************************************************/

static void update_credits (int src_credits)
{
 int i ;

#ifdef PM2
 for (i = 0 ; i < credits_rendus ; i++)
  marcel_sem_V (&mes_credits [src_credits]) ;

 status_credits = bip_tirecv (CREDITS, &credits_rendus, 1) ;
#endif
}

static void check_send (int dest, int *message)
{
#ifdef PM2
 marcel_sem_P (&mes_credits[dest]) ;

 lock_task();

    message [0] = credits_a_rendre [dest] ;
    credits_a_rendre [dest] = 0 ;

 unlock_task();
#endif
}

static void check_receive (int sender, int *message)
{
#ifdef PM2

 int i ;
 int nb_credits_rendus ;
 int x = 0 ;

 nb_credits_rendus = message [0] ;

 for (i = 0 ; i < nb_credits_rendus ; i++)
   marcel_sem_V (&mes_credits [sender]) ;

 lock_task();

   credits_a_rendre [sender] ++ ;

   if (credits_a_rendre [sender] == MAX_CREDITS)
     {
      x = MAX_CREDITS ;
      credits_a_rendre [sender] = 0 ;
     }

 unlock_task();

 if (x == MAX_CREDITS)
  send_to_network (sender, CREDITS, &x, 1) ;
#endif

}

static netinterf_t mad_bip_netinterf = {
  mad_bip_network_init,
  mad_bip_network_send,
  mad_bip_network_receive,
  mad_bip_network_receive_data,
  mad_bip_network_exit
};

netinterf_t *mad_bip_netinterf_init()
{
  return &mad_bip_netinterf;
}

