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
 * mad_test.c
 * ==========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <madeleine.h>

/* #define BI_PROTO */
#define NB_CHANNELS 4
#define STR_BUFFER_LEN 64

#ifdef PM2
int marcel_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
  p_mad_madeleine_t madeleine;
  p_mad_adapter_set_t adapter_set;
  char str_buffer[STR_BUFFER_LEN];
  int k ;
  p_mad_channel_t channel[NB_CHANNELS];
  
#ifdef BI_PROTO
  /*  adapter_set =
      mad_adapter_set_init(2, mad_VIA, "/dev/via_lo", mad_TCP, NULL);*/
  adapter_set =
    mad_adapter_set_init(2, mad_SISCI, NULL, mad_TCP, NULL);
#else /* BI_PROTO */
  /* VIA - loopback 
     adapter_set = mad_adapter_set_init(1, mad_VIA, "/dev/via_lo"); */
  /* VIA - ethernet 
     adapter_set = mad_adapter_set_init(1, mad_VIA, "/dev/via_eth0"); */
  /* TCP */
  adapter_set = mad_adapter_set_init(1, mad_TCP, NULL);
  /* SISCI
     adapter_set = mad_adapter_set_init(1, mad_SISCI, NULL); */
  /* SBP 
     adapter_set = mad_adapter_set_init(1, mad_SBP, NULL); */
  /* MPI 
     adapter_set = mad_adapter_set_init(1, mad_MPI, NULL); */
#endif /* BI_PROTO */

  madeleine = mad_init(&argc, argv, "mad2_conf", adapter_set);

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
