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
$Log: mad_leonie_spawn.c,v $
Revision 1.1  2000/05/31 14:24:01  oaumage
- Spawn Leonie


______________________________________________________________________________
*/

/*
 * Mad_leonie_spawn.c
 * ==================
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <errno.h>
#include <netdb.h>
#ifdef PM2
#include <sys/wait.h>
#endif /* PM2 */

/* #define DEBUG */
/* #define TIMING */
#include "madeleine.h"

#ifdef LEONIE_SPAWN


/*
 * Types locaux
 * ------------
 */
typedef struct
{
  tbx_bool_t              leonie_flag;
  tbx_bool_t              swann_flag;
  int                     id;
  ntbx_connection_data_t  connection_data;
  char                   *master_host_name;
} mad_command_line_t, *p_mad_command_line_t;


/*
 *
 *
 */
typedef struct
{
  int             id;
  p_ntbx_client_t client;
} mad_net_client_t, *p_mad_net_client_t;


/*
 * Objet Madeleine
 * ---------------
 *
static mad_madeleine_t main_madeleine;
 */

/*
 * Objet Netserver
 * ---------------
 */
static p_mad_net_client_t mad_net_client = NULL;


/*
 * Initialisation du client reseau
 * -------------------------------
 */
static p_mad_net_client_t
mad_net_client_init(int                       id,
		    char                     *master_host_name,
		    p_ntbx_connection_data_t  connection_data)
{
  p_mad_net_client_t net_client = NULL;
  ntbx_status_t      ntbx_status;

  LOG_IN();
  net_client = TBX_MALLOC(sizeof(mad_net_client_t));
  CTRL_ALLOC(net_client);
  net_client->id     = id;
  net_client->client = TBX_MALLOC(sizeof(ntbx_client_t));
  CTRL_ALLOC(net_client->client);
  net_client->client->state = ntbx_client_state_uninitialized;
  ntbx_tcp_client_init(net_client->client);
  ntbx_status = ntbx_tcp_client_connect(net_client->client,
					master_host_name,
					connection_data);
  if (ntbx_status != ntbx_success)
    FAILURE("ntbx_tcp_client_connect failed");
  
  LOG_OUT();
  return net_client;
}


/*
 * Analyse des parametres en ligne de commande
 * -------------------------------------------
 */
static p_mad_command_line_t
mad_parse_command_line(int                *argc,
		       char              **argv)
{
  p_mad_command_line_t command_line    = NULL;
  int                  i;
  int                  j;
  
  LOG_IN();
  command_line = TBX_MALLOC(sizeof(mad_command_line_t));
  CTRL_ALLOC(command_line);

  command_line->leonie_flag          = tbx_false;
  command_line->swann_flag           = tbx_false;
  command_line->id                   = -1;
  command_line->connection_data.data[0] = 0;
  command_line->master_host_name     = NULL;

  i = j = 1;    
  
  while (i < (*argc))
    {
      if (!strcmp(argv[i], "-leonie"))
	{
	  command_line->leonie_flag = tbx_true;
	}
      else if (!strcmp(argv[i], "-swann"))
	{
	  command_line->swann_flag = tbx_true;
	}
      else if(!strcmp(argv[i], "-id"))
	{
	  if (i == ((*argc) - 1))
	    FAILURE("-id must be followed "
		    "by the id of the process");

	  command_line->id = atoi(argv[i + 1]);
	  i++;
	}
      else if (!strcmp(argv[i], "-cnx"))
	{
	  if (i == ((*argc) - 1))
	    FAILURE("-cnx must be followed "
		    "by the connection data");

	  strcpy(command_line->connection_data.data, argv[i + 1]);
	  i++;
	}
      else if (!strcmp(argv[i], "-master"))
	{
	  if(i == ((*argc) - 1))
	    FAILURE(" -master must be followed by the master host name");

	  command_line->master_host_name
	    = TBX_MALLOC(strlen(argv[i + 1]) + 1);
	  CTRL_ALLOC(command_line->master_host_name);
	    
	  strcpy(command_line->master_host_name, argv[i + 1]);
	  LOG_STR("mad_init: master_parameter",
		  command_line->master_host_name);
	  i++;
	}
      else
	{
	  argv[j++] = argv[i];
	}
      i++;
    }
  *argc = j;

  LOG_OUT();
  return command_line;
}


/*
 * Initialisation de Madeleine
 * ---------------------------
 */
p_mad_madeleine_t
mad_init(int   *argc,
	 char **argv)
{
  p_mad_command_line_t command_line = NULL;

  LOG_IN();
  command_line = mad_parse_command_line(argc, argv);
  if (command_line->leonie_flag)
    {
      mad_net_client = mad_net_client_init(0,
					   command_line->master_host_name,
					   &command_line->connection_data);
      DISP("connection with Leonie session manager has been established");
    }
  else
    FAILURE("swann connection not yet supported");
  
  LOG_OUT();
  return NULL;
}

#endif /* LEONIE_SPAWN */
