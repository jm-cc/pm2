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
$Log: swann.c,v $
Revision 1.3  2000/03/27 12:54:05  oaumage
- progression des fonctionnalites:
  * support reseau
  * support execution
  * extension du support fichier

Revision 1.2  2000/02/17 09:26:29  oaumage
- Fichier principal de Swann


______________________________________________________________________________
*/

/*
 * swann.c
 * =======
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "swann.h"

int 
main(int   argc   __attribute__((unused)),
     char *argv[] __attribute__((unused)))
{
  p_swann_net_server_t   net_server         = NULL;
  int                    param_id           = 0;
  swann_net_server_id_t  id                 = -1;
  int                    param_master       = 0;
  char                  *master_host_name   = NULL;
  int                    param_cnx          = 0;
  ntbx_connection_data_t connection_data    = {0};

  /* Toolboxes initialization */
  tbx_init();
  ntbx_init();

  /* Command line parsing */
  {
    int i;

    i = 1;
    
    while (i < argc)
      {
	if(!strcmp(argv[i], "-id"))
	{
	  if (param_id)
	    FAILURE("id parameter already specified");

	  param_id++;

	  if (i == (argc - 1))
	    FAILURE("-id option must be followed "
		    "by the id of the process");

	  id = atoi(argv[i + 1]);
	  i++;
	}
      else if (!strcmp(argv[i], "-cnx"))
	{
	  if (param_cnx)
	    FAILURE("cnx parameter already specified");
	  
	  param_cnx++;

	  if (i == (argc - 1))
	    FAILURE("-cnx must be followed "
		    "by the master node connection parameter");

	  strcpy(connection_data, argv[i + 1]);
	  i++;
	}
      else if (!strcmp(argv[i], "-master"))
	{
	  if (param_master)
	    FAILURE("master parameter already specified");

	  param_master++;
	  if(i == (argc - 1))
	    FAILURE(" -master must be followed by the master host name");

	  master_host_name = TBX_MALLOC(strlen(argv[i + 1]) + 1);
	  CTRL_ALLOC(master_host_name);
	    
	  strcpy(master_host_name,
		 argv[i + 1]);
	  i++;
	}
      else
	FAILURE("invalid parameter");
	
      i++;
    }
  }

  /* Net server initialization */
  net_server = swann_net_server_init(id,
				     master_host_name,
				     &connection_data);
  
  return 0;
}
