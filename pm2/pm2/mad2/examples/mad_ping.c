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
$Log: mad_ping.c,v $
Revision 1.9  2000/06/09 13:18:10  rnamyst
Max size reduced to 2MB in mad_ping.c

Revision 1.8  2000/06/07 08:11:18  oaumage
- Ajout du calcul en Megaoctets

Revision 1.7  2000/06/06 13:41:43  oaumage
- rien de particulier

Revision 1.6  2000/05/22 13:44:55  oaumage
- ajout du support application_spawn & leonie_spawn

Revision 1.5  2000/03/02 14:27:54  oaumage
- support d'un protocole par defaut

Revision 1.4  2000/03/02 09:52:21  jfmehaut
pilote Madeleine II/BIP

Revision 1.3  2000/03/01 14:20:07  oaumage
- restauration de mad_ping (remplissage du buffer de test, pour sisci)

Revision 1.2  2000/03/01 14:09:21  oaumage
- suppression du fichier de configuration

Revision 1.1  2000/03/01 11:45:27  oaumage
- deplacement de mad_ping.c, mad_test.c

Revision 1.9  2000/02/04 16:47:32  oaumage
- restauration des reglages par defaut

Revision 1.8  2000/02/03 17:37:35  oaumage
- mad_channel.c : correction de la liberation des donnees specifiques aux
                  connections
- mad_sisci.c   : support DMA avec double buffering

Revision 1.7  2000/01/31 15:50:54  oaumage
- retour a TCP

Revision 1.6  2000/01/13 14:43:59  oaumage
- Makefile: adaptation pour la prise en compte de la toolbox
- mad_ping.c: mise a jour relative aux commandes de timing

Revision 1.5  2000/01/10 10:18:09  oaumage
- TCP par defaut

Revision 1.4  2000/01/04 16:45:50  oaumage
- mise a jour pour support de MPI
- mad_test.c: support multiprotocole MPI+TCP

Revision 1.3  1999/12/15 17:31:08  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
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
static int param_control_receive = 0;
static int param_send_mode       = mad_send_CHEAPER;
static int param_receive_mode    = mad_receive_CHEAPER;
static int param_nb_echantillons = 100;
static int param_min_size        = 1;
static int param_max_size        = 2*1024*1024;
static int param_step            = 0; /* 0 = progression log. */
static int param_nb_tests        = 5;
static int param_bandwidth       = 1;
static int param_no_zero         = 1;
static int param_simul_migr      = 0;
static int param_fill_buffer     = 1;
static int param_fill_buffer_value = 1;


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
  tbx_tick_t        tst_t1 ;
  tbx_tick_t        tst_t2 ;
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
	  
	  TBX_GET_TICK(tst_t1);
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
	  TBX_GET_TICK(tst_t2);       

	  tst_somme += TBX_TIMING_DELAY(tst_t1, tst_t2);
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
	   tst_taille_courante = param_step?tst_taille_courante + param_step:
	     tst_taille_courante * 2)
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
	  
	      TBX_GET_TICK(tst_t1);
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
	      TBX_GET_TICK(tst_t2);       

	      tst_somme += TBX_TIMING_DELAY(tst_t1, tst_t2);
	    }

	  if (param_bandwidth)
	    {
	      tst_moyenne = (((double)taille_courante)
			     * param_nb_tests
			     * param_nb_echantillons)
		/ (tst_somme / 2);
	      fprintf(stderr, "%8d %8.3f %8.3f\n",
		      taille_courante,
		      tst_moyenne,
		      tst_moyenne / 1.048576);
	    }
	  else
	    {
	      tst_moyenne = tst_somme
		/ param_nb_tests
		/ param_nb_echantillons
		/ 2 ;
	      fprintf(stderr, "%8d %5.3f\n", taille_courante, tst_moyenne);
	    }
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
	   tst_taille_courante = param_step?tst_taille_courante + param_step:
	     tst_taille_courante * 2)
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
  tbx_tick_t        tst_t1 ;
  tbx_tick_t        tst_t2 ;
  p_mad_channel_t   channel;

  channel = mad_open_channel(madeleine, 0);

  for (tst_taille_courante = param_min_size;
       tst_taille_courante <= param_max_size;
       tst_taille_courante = param_step?tst_taille_courante + param_step:
	 tst_taille_courante * 2)
    {
      int taille_courante = (   (tst_taille_courante == 0)
			     && (param_no_zero))?1:tst_taille_courante;
      tst_somme = 0;
      
      for (tst_test_courant = 0 ;
	   tst_test_courant < param_nb_tests ;
	   tst_test_courant++)
	{
	  int i ;
	  
	  TBX_GET_TICK(tst_t1);
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
	  TBX_GET_TICK(tst_t2);       
	  tst_somme += TBX_TIMING_DELAY(tst_t1, tst_t2);
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
      buffer = tbx_aligned_malloc(param_max_size, 64);
      if (param_fill_buffer)
	{
	  memset(buffer, param_fill_buffer_value, param_max_size);
	}
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
      tbx_aligned_free(buffer, 64);
    }
  
  return 0;
}
