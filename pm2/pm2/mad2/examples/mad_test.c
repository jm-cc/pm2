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

______________________________________________________________________________
$Log: mad_test.c,v $
Revision 1.6  2000/05/22 13:44:56  oaumage
- ajout du support application_spawn & leonie_spawn

Revision 1.5  2000/03/15 16:58:33  oaumage
- support APPLICATION_SPAWN
- mad_test.c: demo `application spawn'

Revision 1.4  2000/03/02 14:27:55  oaumage
- support d'un protocole par defaut

Revision 1.3  2000/03/02 09:52:22  jfmehaut
pilote Madeleine II/BIP

Revision 1.2  2000/03/01 14:09:22  oaumage
- suppression du fichier de configuration

Revision 1.1  2000/03/01 11:45:29  oaumage
- deplacement de mad_ping.c, mad_test.c

Revision 1.8  2000/02/03 17:37:36  oaumage
- mad_channel.c : correction de la liberation des donnees specifiques aux
                  connections
- mad_sisci.c   : support DMA avec double buffering

Revision 1.7  2000/01/31 15:50:56  oaumage
- retour a TCP

Revision 1.6  2000/01/13 14:43:59  oaumage
- Makefile: adaptation pour la prise en compte de la toolbox
- mad_ping.c: mise a jour relative aux commandes de timing

Revision 1.5  2000/01/10 10:18:09  oaumage
- TCP par defaut

Revision 1.4  2000/01/04 16:45:51  oaumage
- mise a jour pour support de MPI
- mad_test.c: support multiprotocole MPI+TCP

Revision 1.3  1999/12/15 17:31:08  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * mad_test.c
 * ==========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <madeleine.h>

/* #define BI_PROTO */
#define NB_CHANNELS 1
#define STR_BUFFER_LEN 64

#ifdef PM2
int marcel_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
  p_mad_madeleine_t madeleine = NULL;
#ifndef LEONIE_SPAWN
  p_mad_adapter_set_t adapter_set;
#endif /* LEONIE_SPAWN */
  char str_buffer[STR_BUFFER_LEN];
  int k ;
  p_mad_channel_t channel[NB_CHANNELS];

#ifdef LEONIE_SPAWN
  mad_init(&argc, argv);
  DISP("Returned from mad_init");
  exit(EXIT_SUCCESS);
#else  /* LEONIE_SPAWN */
#ifdef BI_PROTO
  /*  adapter_set =
      mad_adapter_set_init(2, mad_VIA, "/dev/via_lo", mad_TCP, NULL);*/
  /* adapter_set =
     mad_adapter_set_init(2, mad_SISCI, NULL, mad_TCP, NULL); */
  adapter_set = 
    mad_adapter_set_init(2, mad_MPI, NULL, mad_TCP, NULL); 
#else /* BI_PROTO */
  /* VIA - loopback 
     adapter_set = mad_adapter_set_init(1, mad_VIA, "/dev/via_lo"); */
  /* VIA - ethernet 
     adapter_set = mad_adapter_set_init(1, mad_VIA, "/dev/via_eth0"); */
  /* TCP 
     adapter_set = mad_adapter_set_init(1, mad_TCP, NULL); */
  /* SISCI 
     adapter_set = mad_adapter_set_init(1, mad_SISCI, NULL); */
  /* SBP 
     adapter_set = mad_adapter_set_init(1, mad_SBP, NULL); */
  /* MPI 
     adapter_set = mad_adapter_set_init(1, mad_MPI, NULL); */
  /* BIP 
     adapter_set = mad_adapter_set_init(1, mad_BIP, NULL); */
  /* Default */
  adapter_set = mad_adapter_set_init(1, mad_DRIVER_DEFAULT, NULL); 
#endif /* BI_PROTO */

#ifdef APPLICATION_SPAWN
  {
    char *url;
    
    url = mad_pre_init(adapter_set);
    if (argc > 1)
      {
	madeleine = mad_init(-1, /* rank, -1 = default */
			     NULL, /* Configuration file */
			     argv[1] /* URL */
			     );
      }
    else
      {
	DISP("Master: url = %s\n", url);
	madeleine = mad_init(-1, NULL, NULL);
      }
    
    // exit(EXIT_SUCCESS);
  }
#else /* APPLICATION_SPAWN */
  madeleine = mad_init(&argc, argv, NULL, adapter_set);
#endif /* APPLICATION_SPAWN */
#endif /* LEONIE_SPAWN */  
  for (k = 0 ; k < NB_CHANNELS ; k++)
    {
#ifdef BI_PROTO
      /*      if ((k + 1) <= NB_CHANNELS /2)
	{
	  channel[k] = mad_open_channel(madeleine, 1);
	}
      else
      {
      channel[k] = mad_open_channel(madeleine, 0);
      }*/
      channel[k] = mad_open_channel(madeleine, k & 1);
#else /* BI_PROTO */
      channel[k] = mad_open_channel(madeleine, 0);
#endif /* BI_PROTO */
    }

  for (k = 0 ; k < NB_CHANNELS ; k++)
    {
      int j;

      fprintf(stderr, "adapter: %s\n", channel[k]->adapter->name);

      for (j = 0 ; j < madeleine->configuration.size ; j++)
	{
      
	  if (madeleine->configuration.local_host_id == j)
	    {
	      /* Receiver */
	      int i ;
	  
	      for (i = 1 ;
		   i < madeleine->configuration.size ;
		   i++)
		{
		  int test_1 = 0;
		  int test_2 = 0;
		  int test_3 = 0;
		  int test_4 = 0;
		  int test_5 = 0;
		  int test_6 = 0;
		  int len = 0;
		  p_mad_connection_t connection;

		  /* mark the buffer */
		  sprintf(str_buffer,
			  "----------------------------------------\n");
		  connection = mad_begin_unpacking(channel[k]);
	      
		  mad_unpack(connection,
			     &test_1,
			     sizeof(int),
			     mad_send_SAFER,
			     mad_receive_EXPRESS);
		  fprintf(stderr, "Express data 1 : %d\n", test_1);
		  
		  mad_unpack(connection,
			     &test_2,
			     sizeof(int),
			     mad_send_CHEAPER,
			     mad_receive_EXPRESS);
		  fprintf(stderr, "Express data 2 : %d\n", test_2);

		  mad_unpack(connection,
			     &test_3,
			     sizeof(int),
			     mad_send_SAFER,
			     mad_receive_CHEAPER);

		  mad_unpack(connection,
			     &test_4,
			     sizeof(int),
			     mad_send_CHEAPER,
			     mad_receive_CHEAPER);

		  mad_unpack(connection,
			     &test_5,
			     sizeof(int),
			     mad_send_LATER,
			     mad_receive_EXPRESS);
		  fprintf(stderr, "Express data 5 : %d\n", test_5);

		  mad_unpack(connection,
			     &test_6,
			     sizeof(int),
			     mad_send_LATER,
			     mad_receive_CHEAPER);

		  mad_unpack(connection,
			     &len,
			     sizeof(int),
			     mad_send_CHEAPER,
			     mad_receive_EXPRESS);
		  fprintf(stderr, "Express data (len) : %d\n", len);

		  mad_unpack(connection,
			     str_buffer,
			     len,
			     mad_send_CHEAPER,
			     mad_receive_CHEAPER);
		  
		  mad_end_unpacking(connection);
		  
		  fprintf(stderr, "Received data : %d, %d, %d, %d, %d, %d\n",
			  test_1,
			  test_2,
			  test_3,
			  test_4,
			  test_5,
			  test_6);
		  fprintf(stderr, "len = %d\n", len);
		  fprintf(stderr, str_buffer);
		}
	    }
	  else
	    {
	      /* Sender(s) */
	      int test_1 = 1;
	      int test_2 = 2;
	      int test_3 = 3;
	      int test_4 = 4;
	      int test_5 = 5;
	      int test_6 = 6;
	      int len = 0 ;
	      p_mad_connection_t connection;

	      sprintf(str_buffer,
		      "The sender node of this message is %d\n",
		      (int)madeleine->configuration.local_host_id);

	      len = strlen(str_buffer) + 1 ;
	      
	      connection = mad_begin_packing(channel[k], j);
	  
	      mad_pack(connection,
		       &test_1,
		       sizeof(int),
		       mad_send_SAFER,
		       mad_receive_EXPRESS);
	      mad_pack(connection,
		       &test_2,
		       sizeof(int),
		       mad_send_CHEAPER,
		       mad_receive_EXPRESS);			     
	      mad_pack(connection,
		       &test_3,
		       sizeof(int),
		       mad_send_SAFER,
		       mad_receive_CHEAPER);
	      mad_pack(connection,
		       &test_4,
		       sizeof(int),
		       mad_send_CHEAPER,
		       mad_receive_CHEAPER);
	      mad_pack(connection,
		       &test_5,
		       sizeof(int),
		       mad_send_LATER,
		       mad_receive_EXPRESS);
	      mad_pack(connection,
		       &test_6,
		       sizeof(int),
		       mad_send_LATER,
		       mad_receive_CHEAPER);
	      mad_pack(connection,
		       &len,
		       sizeof(int),
		       mad_send_CHEAPER,
		       mad_receive_EXPRESS);
	      mad_pack(connection,
		       str_buffer,
		       len,
		       mad_send_CHEAPER,
		       mad_receive_CHEAPER);

	      mad_end_packing(connection);
	    }
	}
    }
  mad_exit(madeleine);
  return 0;
}
