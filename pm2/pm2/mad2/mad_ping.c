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
/*#define MGS_MODE_1 mad_send_SAFER*/
#define MGS_MODE_1 mad_send_CHEAPER
#define MGS_MODE_2 mad_send_CHEAPER
#define MGR_MODE_1 mad_receive_CHEAPER
#define MGR_MODE_2 mad_receive_CHEAPER

/* parameter values */
static int param_control_receive = 1;
static int param_send_mode = mad_send_CHEAPER;
static int param_receive_mode = mad_receive_CHEAPER;
static int param_nb_echantillons = 10;
static int param_min_size = 0;
static int param_max_size = 256;
static int param_step = 1;
static int param_nb_tests = 5;
static int param_bandwidth = 0;
static int param_no_zero = 1;
static int param_simul_migr = 0;

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
  int i ;
  unsigned int n ;
  
  for (n = 0, i = 0 ;
       i < len ;
       i++)
    {
      unsigned char c ;

      n += 7 ;
      c = (unsigned char)(n % 256);

      buffer[i] = c ;
    }
}

static void clear_buffer(int len)
{
  memset(buffer, 0, len);
}

static void control_buffer(int len)
{
  int i ;
  unsigned int n ;
  int ok = 1 ;
  
  for (n = 0, i = 0 ;
       i < len ;
       i++)
    {
      unsigned char c ;

      n += 7 ;
      c = (unsigned char)(n % 256);

      if (buffer[i] != c)
	{
	  int v1, v2 ;

	  v1 = c ;
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
  double            tst_somme ;
  double            tst_moyenne ;
  int               tst_taille_courante ;
  int               tst_test_courant ;
  mad_tick_t        tst_t1 ;
  mad_tick_t        tst_t2 ;
  p_mad_channel_t   channel;
  
  channel = mad_open_channel(madeleine, 0);

  if (param_simul_migr)
    {
      tst_somme = 0 ;
      tst_moyenne = 0 ;      
      
      for (tst_test_courant = 0 ;
	   tst_test_courant < param_nb_tests ;
	   tst_test_courant++)
	{
	  int i ;
	  
	  MAD_GET_TICK(tst_t1);
	  for(i = 0 ; i < param_nb_echantillons ; i++) 
	    {
	      static char b1[4];
	      static char b2[4];
	      static char b3[16];
	      static char b4[608];
	      static char b5[4];
	      p_mad_connection_t connection;

	      connection = mad_begin_packing(channel, 1);
	      mad_pack(connection, b1, 4,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_pack(connection, b2, 4,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_pack(connection, b3, 16,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_pack(connection, b4, 608,
		       MGS_MODE_2, MGR_MODE_2);
	      mad_pack(connection, b5, 4,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_end_packing(connection);

	      connection = mad_begin_unpacking(channel);
	      mad_unpack(connection, b1, 4,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_unpack(connection, b2, 4,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_unpack(connection, b3, 16,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_unpack(connection, b4, 608,
		       MGS_MODE_2, MGR_MODE_2);
	      mad_unpack(connection, b5, 4,
			 MGS_MODE_1, MGR_MODE_1);
	      mad_end_unpacking(connection);
	    }
	  MAD_GET_TICK(tst_t2);       

	  tst_somme += MAD_TIMING_DELAY(tst_t1, tst_t2);
	}

      tst_moyenne = tst_somme
	/ param_nb_tests
	/ param_nb_echantillons
	/ 2;
      fprintf(stderr, "migration: %5.3f\n", tst_moyenne);
    }
  else
    {
      for (tst_taille_courante = param_min_size ;
	   tst_taille_courante <= param_max_size;
	   tst_taille_courante += param_step)
	{
	  int taille_courante = ((tst_taille_courante == 0)
				 && (param_no_zero))?1:tst_taille_courante;
     
	  tst_somme = 0 ;
	  tst_moyenne = 0 ;      
      
	  for (tst_test_courant = 0 ;
	       tst_test_courant < param_nb_tests ;
	       tst_test_courant++)
	    {
	      int i ;
	  
	      MAD_GET_TICK(tst_t1);
	      for(i = 0 ; i < param_nb_echantillons ; i++) 
		{
		  p_mad_connection_t connection;

		  connection = mad_begin_packing(channel, 1);
		  mad_pack(connection,
			   buffer,
			   taille_courante,
			   param_send_mode,
			   param_receive_mode);
		  mad_end_packing(connection);

		  connection = mad_begin_unpacking(channel);
		  mad_unpack(connection,
			     buffer,
			     taille_courante,
			     param_send_mode,
			     param_receive_mode);
		  mad_end_unpacking(connection);
		}
	      MAD_GET_TICK(tst_t2);       

	      tst_somme += MAD_TIMING_DELAY(tst_t1, tst_t2);
	    }

	  if (param_bandwidth)
	    {
	      tst_moyenne = (taille_courante
			     * param_nb_tests
			     * param_nb_echantillons)
		/ (tst_somme / 2);
	    }
	  else
	    {
	      tst_moyenne = tst_somme
		/ param_nb_tests
		/ param_nb_echantillons
		/ 2 ;
	    }
	  fprintf(stderr, "%5d %5.3f\n", taille_courante, tst_moyenne);
	}
    }
}

static void slave(p_mad_madeleine_t madeleine)
{
  p_mad_channel_t   channel;
  int               tst_taille_courante ;
  int               tst_test_courant ;

  channel = mad_open_channel(madeleine, 0);
  if (param_simul_migr)
    {
      for (tst_test_courant = 0 ;
	   tst_test_courant < param_nb_tests ;
	   tst_test_courant++)
	{
	  int i ;
	  
	  for(i = 0; i < param_nb_echantillons ; i++) 
	    {
	      static char b1[4];
	      static char b2[4];
	      static char b3[16];
	      static char b4[608];
	      static char b5[4];
	      p_mad_connection_t connection;

	      connection = mad_begin_unpacking(channel);
	      mad_unpack(connection, b1, 4,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_unpack(connection, b2, 4,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_unpack(connection, b3, 16,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_unpack(connection, b4, 608,
		       MGS_MODE_2, MGR_MODE_2);
	      mad_unpack(connection, b5, 4,
			 MGS_MODE_1, MGR_MODE_1);
	      mad_end_unpacking(connection);

	      connection = mad_begin_packing(channel, 0);
	      mad_pack(connection, b1, 4,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_pack(connection, b2, 4,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_pack(connection, b3, 16,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_pack(connection, b4, 608,
		       MGS_MODE_2, MGR_MODE_2);
	      mad_pack(connection, b5, 4,
		       MGS_MODE_1, MGR_MODE_1);
	      mad_end_packing(connection);
	    }
	}
    }
  else
    {      
      for (tst_taille_courante = param_min_size;
	   tst_taille_courante <= param_max_size;
	   tst_taille_courante+= param_step)
	{
	  int taille_courante = ((tst_taille_courante == 0)
				 && (param_no_zero))?1:tst_taille_courante;
      
	  for (tst_test_courant = 0 ;
	       tst_test_courant < param_nb_tests ;
	       tst_test_courant++)
	    {
	      int i ;
	  
	      for(i = 0; i < param_nb_echantillons ; i++) 
		{
		  p_mad_connection_t connection;

		  connection = mad_begin_unpacking(channel);
		  mad_unpack(connection,
			     buffer,
			     taille_courante,
			     param_send_mode,
			     param_receive_mode);	      
		  mad_end_unpacking(connection);

		  connection = mad_begin_packing(channel, 0);
		  mad_pack(connection,
			   buffer,
			   taille_courante,
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
  double            tst_somme ;
  double            tst_moyenne ;
  int               tst_taille_courante ;
  int               tst_test_courant ;
  mad_tick_t        tst_t1 ;
  mad_tick_t        tst_t2 ;
  p_mad_channel_t   channel;

  channel = mad_open_channel(madeleine, 0);

  for (tst_taille_courante = param_min_size;
       tst_taille_courante <= param_max_size;
       tst_taille_courante+= param_step)
    {
      int taille_courante = (   (tst_taille_courante == 0)
			     && (param_no_zero))?1:tst_taille_courante;
      tst_somme = 0;
      
      for (tst_test_courant = 0 ;
	   tst_test_courant < param_nb_tests ;
	   tst_test_courant++)
	{
	  int i ;
	  
	  MAD_GET_TICK(tst_t1);
	  for(i = 0; i < param_nb_echantillons ; i++) 
	    {
	      p_mad_connection_t connection;
	      
	      fill_buffer(taille_courante);	      
	      connection = mad_begin_packing(channel, 1);
	      mad_pack(connection,
		       buffer,
		       taille_courante,
		       param_send_mode,
		       param_receive_mode);
	      mad_end_packing(connection);

	      clear_buffer(taille_courante);

	      connection = mad_begin_unpacking(channel);
	      mad_unpack(connection,
			 buffer,
			 taille_courante,
			 param_send_mode,
			 param_receive_mode);
	      mad_end_unpacking(connection);
	      control_buffer(taille_courante);
	    }
	  MAD_GET_TICK(tst_t2);       
	  tst_somme += MAD_TIMING_DELAY(tst_t1, tst_t2);
	}

      if (param_bandwidth)
	{
	  tst_moyenne = (taille_courante
			 * param_nb_tests
			 * param_nb_echantillons)
	    / (tst_somme / 2);
	}
      else
	{
	  tst_moyenne = tst_somme
	    / param_nb_tests
	    / param_nb_echantillons
	    / 2 ;
	}
      
      fprintf(stderr, "%5d %5.3f\n", taille_courante, tst_moyenne);
    }
}

#ifdef PM2
int marcel_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
  p_mad_madeleine_t   madeleine;
  p_mad_adapter_set_t adapter_set;
  
  /* VIA - loopback
     adapter_list = mad_adapter_list_init(1, mad_VIA, "/dev/via_lo"); */
  /* VIA - ethernet 
     adapter_list = mad_adapter_list_init(1, mad_VIA, "/dev/via_eth0"); */
  /* TCP */
  adapter_set = mad_adapter_set_init(1, mad_TCP, NULL); 
  /* SISCI
     adapter_set = mad_adapter_set_init(1, mad_SISCI, NULL); */
  /* SBP 
     adapter_set = mad_adapter_set_init(1, mad_SBP, NULL); */
  /* MPI 
     adapter_set = mad_adapter_set_init(1, mad_MPI, NULL); */

  mad_timing_init();
  madeleine = mad_init(&argc, argv, "mad2_conf", adapter_set);

  /* command line args analysis */
  {
    int i = 1 ;

    while (i < argc)
      {
	if (!strcmp(argv[i], "-control_receive_on"))
	  {
	    param_control_receive = 1;
	  }
	else if (!strcmp(argv[i], "-control_receive_off"))
	  {
	    param_control_receive = 0;
	  }
	else if (!strcmp(argv[i], "-no_zero_on"))
	  {
	    param_no_zero = 1;
	  }
	else if (!strcmp(argv[i], "-no_zero_off"))
	  {
	    param_no_zero = 0;
	  }
	else if (!strcmp(argv[i], "-bandwidth_on"))
	  {
	    param_bandwidth = 1;
	  }
	else if (!strcmp(argv[i], "-bandwidth_off"))
	  {
	    param_bandwidth = 0;
	  }
	else if (!strcmp(argv[i], "-send_safer"))
	  {
	    param_send_mode = mad_send_SAFER;
	  }
	else if (!strcmp(argv[i], "-send_later"))
	  {
	    param_send_mode = mad_send_LATER;
	  }
	else if (!strcmp(argv[i], "-send_cheaper"))
	  {
	    param_send_mode = mad_send_CHEAPER;
	  }
	else if (!strcmp(argv[i], "-receive_express"))
	  {
	    param_receive_mode = mad_receive_EXPRESS;
	  }
	else if (!strcmp(argv[i], "-receive_cheaper"))
	  {
	    param_receive_mode = mad_receive_CHEAPER;
	  }
	else if (!strcmp(argv[i], "-nb_echantillons"))
	  {
	    i++ ;

	    if (i < argc)
	      {
		param_nb_echantillons = atoi(argv[i]);
	      }
	    else
	      {
		FAILURE("not enough args");
	      }
	  }
	else if (!strcmp(argv[i], "-max_size"))
	  {
	    i++ ;

	    if (i < argc)
	      {
		param_max_size = atoi(argv[i]);
	      }
	    else
	      {
		FAILURE("not enough args");
	      }
	  }
	else if (!strcmp(argv[i], "-min_size"))
	  {
	    i++ ;

	    if (i < argc)
	      {
		param_min_size = atoi(argv[i]);
	      }
	    else
	      {
		FAILURE("not enough args");
	      }
	  }
	else if (!strcmp(argv[i], "-step"))
	  {
	    i++ ;

	    if (i < argc)
	      {
		param_step = atoi(argv[i]);
	      }
	    else
	      {
		FAILURE("not enough args");
	      }
	  }
	else if (!strcmp(argv[i], "-nb_tests"))
	  {
	    i++ ;

	    if (i < argc)
	      {
		param_nb_tests = atoi(argv[i]);
	      }
	    else
	      {
		FAILURE("not enough args");
	      }
	  }
	else
	  {
	    FAILURE("unexpected arg");
	  }

	i++;
      }
  }

  if (!param_simul_migr)
    {
      buffer = mad_aligned_malloc(param_max_size, 64);
    }
  
  if (madeleine->configuration.local_host_id == 0)
    {
      /* Master */
      if ((!param_simul_migr) && param_control_receive)
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
  if (!param_simul_migr)
    {
      mad_aligned_free(buffer, 64);
    }
  
  return 0;
}
