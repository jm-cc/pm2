
/*
 * mad_ping.c
 * ==========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <madeleine.h>

/* parameters */
#define PENTIUM_TIMING

/* parameter values */
static const int param_control_receive   = 0;
static const int param_send_mode         = mad_send_CHEAPER;
static const int param_receive_mode      = mad_receive_CHEAPER;
static const int param_nb_samples        = 1000;
static const int param_min_size          = 1;
static const int param_max_size          = 2*1024*1024;
static const int param_step              = 0; /* 0 = progression log. */
static const int param_nb_tests          = 5;
static const int param_bandwidth         = 1;
static const int param_no_zero           = 1;
static const int param_fill_buffer       = 1;
static const int param_fill_buffer_value = 1;
static const int param_one_way           = 1;


/* static variables */
static unsigned char *buffer;

/* Prototypes */
static void fill_buffer(int len);
static void clear_buffer(int len);
static void control_buffer(int len);
static void master(p_mad_madeleine_t madeleine);
static void slave(p_mad_madeleine_t madeleine);
static void master_ctrl(p_mad_madeleine_t madeleine);

/* Functions */
static void fill_buffer(int len)
{
  int i;
  unsigned int n;
  
  for (n = 0, i = 0;
       i < len;
       i++)
    {
      unsigned char c;

      n += 7;
      c = (unsigned char)(n % 256);

      buffer[i] = c;
    }
}

static void clear_buffer(int len)
{
  memset(buffer, 0, len);
}

static void control_buffer(int len)
{
  int i;
  unsigned int n;
  int ok = 1;
  
  for (n = 0, i = 0;
       i < len;
       i++)
    {
      unsigned char c;

      n += 7;
      c = (unsigned char)(n % 256);

      if (buffer[i] != c)
	{
	  int v1, v2;

	  v1 = c;
	  v2 = buffer[i];
	  fprintf(stderr, "Bad data at byte %d: expected %d, received %d\n",
		  i, v1, v2);
	  ok = 0;
	}
    }

  if (ok == 0)
    {
      fprintf(stderr, "%d bytes reception failed\n", len);
    }
  else
    {
      fprintf(stderr, "%d bytes received correctly\n", len);
    }
}

static void master(p_mad_madeleine_t madeleine)
{
  p_mad_channel_t channel = NULL;
  double          sum;
  double          mean;
  int             size;
  int             test;
  tbx_tick_t      t1;
  tbx_tick_t      t2;
  
  channel = mad_open_channel(madeleine, 0);

  for (size = param_min_size;
       size <= param_max_size;
       size = param_step?size + param_step:size * 2)
    {
      const int _size = (!size && param_no_zero)?1:size;
      int dummy = 0;
     
      sum = mean = 0;      
      
      if (param_one_way)
	{
	  p_mad_connection_t connection = NULL;
	  const int          nb_tests   =
	    param_nb_tests * param_nb_samples;

	  TBX_GET_TICK(t1);
	  for (test = 0; test < nb_tests; test++)
	    {
	      connection = mad_begin_packing(channel, 1);
	      mad_pack(connection, buffer, _size,
		       param_send_mode, param_receive_mode);
	      mad_end_packing(connection);
	    }

	  connection = mad_begin_unpacking(channel);
	  mad_unpack(connection, &dummy, sizeof(dummy),
		     param_send_mode, param_receive_mode);
	  mad_end_unpacking(connection);
	  TBX_GET_TICK(t2);       
	  
	  sum += TBX_TIMING_DELAY(t1, t2);	  
	}
      else
	{
	  for (test = 0; test < param_nb_tests; test++)
	    {
	      int i;

	      TBX_GET_TICK(t1);
	      for(i = 0; i < param_nb_samples; i++) 
		{
		  p_mad_connection_t connection;

		  connection = mad_begin_packing(channel, 1);
		  mad_pack(connection,
			   buffer,
			   _size,
			   param_send_mode,
			   param_receive_mode);
		  mad_end_packing(connection);

		  connection = mad_begin_unpacking(channel);
		  mad_unpack(connection,
			     buffer,
			     _size,
			     param_send_mode,
			     param_receive_mode);
		  mad_end_unpacking(connection);
		}
	      TBX_GET_TICK(t2);       
	      sum += TBX_TIMING_DELAY(t1, t2);
	    }
	}

      if (param_bandwidth)
	{
	  double f = 2 - param_one_way;

	  mean = (_size * (double)param_nb_tests * (double)param_nb_samples)
		   / (sum / f);
	  fprintf(stderr, "%8d %8.3f %8.3f\n",
		  _size, mean, mean / 1.048576);
	}
      else
	{
	  double f = 2 - param_one_way;

	  mean = sum / param_nb_tests / param_nb_samples / f;
	  fprintf(stderr, "%8d %5.3f\n", _size, mean);
	}
    }
}

static void slave(p_mad_madeleine_t madeleine)
{
  p_mad_channel_t channel = NULL;
  int             size;
  int             test;

  channel = mad_open_channel(madeleine, 0);
  for (size = param_min_size;
       size <= param_max_size;
       size = param_step?size + param_step:size * 2)
    {
      const int _size = (!size && param_no_zero)?1:size;
      
      if (param_one_way)
	{
	  p_mad_connection_t connection = NULL;
	  const int          nb_tests   =
	    param_nb_tests * param_nb_samples;
          int dummy = 0;

	  for (test = 0; test < nb_tests; test++)
	    {
	      connection = mad_begin_unpacking(channel);
	      mad_unpack(connection, buffer, _size,
			 param_send_mode, param_receive_mode);	      
	      mad_end_unpacking(connection);
	    }	  

	  connection = mad_begin_packing(channel, 0);
	  mad_pack(connection, &dummy, sizeof(dummy),
		   param_send_mode, param_receive_mode);
	  mad_end_packing(connection);
	}
      else
	{
	  for (test = 0; test < param_nb_tests; test++)
	    {
	      int i;
	  
	      for(i = 0; i < param_nb_samples; i++) 
		{
		  p_mad_connection_t connection;

		  connection = mad_begin_unpacking(channel);
		  mad_unpack(connection,
			     buffer,
			     _size,
			     param_send_mode,
			     param_receive_mode);	      
		  mad_end_unpacking(connection);

		  connection = mad_begin_packing(channel, 0);
		  mad_pack(connection,
			   buffer,
			   _size,
			   param_send_mode,
			   param_receive_mode);
		  mad_end_packing(connection);
		}
	    }	  
	}
    }
}

static void master_ctrl(p_mad_madeleine_t madeleine)
{
  double            sum;
  double            mean;
  int               size;
  int               test;
  tbx_tick_t        t1;
  tbx_tick_t        t2;
  p_mad_channel_t   channel;

  channel = mad_open_channel(madeleine, 0);

  for (size = param_min_size;
       size <= param_max_size;
       size = param_step?size + param_step:
	 size * 2)
    {
      int _size = (   (size == 0)
			     && (param_no_zero))?1:size;
      sum = 0;
      
      for (test = 0;
	   test < param_nb_tests;
	   test++)
	{
	  int i;
	  
	  TBX_GET_TICK(t1);
	  for(i = 0; i < param_nb_samples; i++) 
	    {
	      p_mad_connection_t connection;
	      
	      fill_buffer(_size);	      
	      connection = mad_begin_packing(channel, 1);
	      mad_pack(connection,
		       buffer,
		       _size,
		       param_send_mode,
		       param_receive_mode);
	      mad_end_packing(connection);

	      clear_buffer(_size);

	      connection = mad_begin_unpacking(channel);
	      mad_unpack(connection,
			 buffer,
			 _size,
			 param_send_mode,
			 param_receive_mode);
	      mad_end_unpacking(connection);
	      control_buffer(_size);
	    }
	  TBX_GET_TICK(t2);       
	  sum += TBX_TIMING_DELAY(t1, t2);
	}

      if (param_bandwidth)
	{
	  mean = (_size * (double)param_nb_tests * (double)param_nb_samples
		  * (double)2) / sum;
	}
      else
	{
	  mean = sum / param_nb_tests / param_nb_samples / 2;
	}
      
      fprintf(stderr, "%5d %5.3f\n", _size, mean);
    }
}

#ifdef PM2
int marcel_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
  p_mad_madeleine_t   madeleine = NULL;
#ifdef LEONIE_SPAWN
  mad_init(&argc, argv);
  DISP("Returned from mad_init");
  exit(EXIT_SUCCESS);
#else  /* LEONIE_SPAWN */
  p_mad_adapter_set_t adapter_set;
  
  /* VIA - loopback
     adapter_list = mad_adapter_list_init(1, mad_VIA, "/dev/via_lo"); */
  /* VIA - ethernet 
     adapter_list = mad_adapter_list_init(1, mad_VIA, "/dev/via_eth0"); */
  /* TCP
     adapter_set = mad_adapter_set_init(1, mad_TCP, NULL); */
  /* SISCI 
     adapter_set = mad_adapter_set_init(1, mad_SISCI, NULL); */
  /* SBP 
     adapter_set = mad_adapter_set_init(1, mad_SBP, NULL); */
  /* MPI 
     adapter_set = mad_adapter_set_init(1, mad_MPI, NULL);  */
  /* BIP 
     adapter_set = mad_adapter_set_init(1, mad_BIP, NULL);  */
  /* Default */
  adapter_set = mad_adapter_set_init(1, mad_DRIVER_DEFAULT, NULL); 

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

  buffer = tbx_aligned_malloc(param_max_size, 64);
  if (param_fill_buffer)
    {
      memset(buffer, param_fill_buffer_value, param_max_size);
    }
  
  if (!madeleine->configuration->local_host_id)
    {
      /* Master */
      if (param_control_receive)
	{
	  master_ctrl(madeleine);
	}
      else
	{
	  master(madeleine);
	}
    }
  else
    {
      /* Slave */
      slave(madeleine);      
    }

  mad_exit(madeleine);
  tbx_aligned_free(buffer, 64);
    
  return 0;
}
