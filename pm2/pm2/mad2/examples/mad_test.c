
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

  fprintf(stderr, "main -->\n");

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

      for (j = 0 ; j < madeleine->configuration->size ; j++)
	{
      
	  if (madeleine->configuration->local_host_id == j)
	    {
	      /* Receiver */
	      int i ;
	  
	      for (i = 1 ;
		   i < madeleine->configuration->size ;
		   i++)
		{
		  char test_1 = 0;
		  char test_2 = 0;
		  char test_3 = 0;
		  char test_4 = 0;
		  char test_5 = 0;
		  char test_6 = 0;
		  char len = 0;
		  p_mad_connection_t connection;

		  /* mark the buffer */
		  sprintf(str_buffer,
			  "----------------------------------------\n");
		  connection = mad_begin_unpacking(channel[k]);
	      
		  mad_unpack(connection,
			     &test_1,
			     sizeof(char),
			     mad_send_SAFER,
			     mad_receive_EXPRESS);
		  fprintf(stderr, "Express data 1 : %d\n", test_1);
		  
		  mad_unpack(connection,
			     &test_2,
			     sizeof(char),
			     mad_send_CHEAPER,
			     mad_receive_EXPRESS);
		  fprintf(stderr, "Express data 2 : %d\n", test_2);

		  mad_unpack(connection,
			     &test_3,
			     sizeof(char),
			     mad_send_SAFER,
			     mad_receive_CHEAPER);

		  mad_unpack(connection,
			     &test_4,
			     sizeof(char),
			     mad_send_CHEAPER,
			     mad_receive_CHEAPER);

		  mad_unpack(connection,
			     &test_5,
			     sizeof(char),
			     mad_send_LATER,
			     mad_receive_EXPRESS);
		  fprintf(stderr, "Express data 5 : %d\n", test_5);

		  mad_unpack(connection,
			     &test_6,
			     sizeof(char),
			     mad_send_LATER,
			     mad_receive_CHEAPER);

		  mad_unpack(connection,
			     &len,
			     sizeof(char),
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
	      char test_1 = 1;
	      char test_2 = 2;
	      char test_3 = 3;
	      char test_4 = 4;
	      char test_5 = 5;
	      char test_6 = 6;
	      char len = 0 ;
	      p_mad_connection_t connection;

	      sprintf(str_buffer,
		      "The sender node of this message is %d\n",
		      (int)madeleine->configuration->local_host_id);

	      len = strlen(str_buffer) + 1 ;
	      
	      connection = mad_begin_packing(channel[k], j);
	  
	      mad_pack(connection,
		       &test_1,
		       sizeof(char),
		       mad_send_SAFER,
		       mad_receive_EXPRESS);
	      mad_pack(connection,
		       &test_2,
		       sizeof(char),
		       mad_send_CHEAPER,
		       mad_receive_EXPRESS);			     
	      mad_pack(connection,
		       &test_3,
		       sizeof(char),
		       mad_send_SAFER,
		       mad_receive_CHEAPER);
	      mad_pack(connection,
		       &test_4,
		       sizeof(char),
		       mad_send_CHEAPER,
		       mad_receive_CHEAPER);
	      mad_pack(connection,
		       &test_5,
		       sizeof(char),
		       mad_send_LATER,
		       mad_receive_EXPRESS);
	      mad_pack(connection,
		       &test_6,
		       sizeof(char),
		       mad_send_LATER,
		       mad_receive_CHEAPER);
	      mad_pack(connection,
		       &len,
		       sizeof(char),
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

  fprintf(stderr, "main <--\n");

  return 0;
}
