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
 suppong documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

/*
 * Leonie.c
 * ========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "leonie.h"

/*
 * Lex/Yacc and related external objects
 * -------------------------------------
 */
extern p_leo_parser_result_t  leo_parser_result;
extern                  FILE *yyin;
extern                  FILE *yyout;
extern                  int   yydebug;


int 
yylex(void);

int
yyparse(void);

/*
 * Static Variables
 * ----------------
 */

static p_leonie_t main_leonie = NULL;


/*
 * Functions
 * ---------
 */

int
leo_yy_input(char *buffer,
	     int   max_size)
{
  LOG_IN();
  if (main_leonie)
    {
      leo_file_handler_t  file_handler =
	main_leonie->parser_file_handle;
      FILE               *file_ptr     =
	main_leonie->parser_file_ptr;
      
      if (file_handler == -1)
	{
	  /* Local file */
	  if (file_ptr)
	    {
	      int status;

	      status = fread(buffer, 1, max_size, file_ptr);

	      if (status)
		{
		  LOG_OUT();
		  return status;
		}
	      else
		{
		  if (feof(file_ptr))
		    {
		      LOG_OUT();
		      return 0; /* YY_NULL */
		    }
		  else
		    {
		      perror("fread");
		      FAILURE("leo_yy_input failed");
		    }
		}
	    }
	  else
	    FAILURE("invalid local file ptr");
	}
      else
	{
	  /* Remote file */
	  FAILURE("unimplemented feature");
	}
    }
  else
    FAILURE("Leonie uninitialized");
}

static void
leo_open_local_parser_file(p_leonie_t leonie,
			   char *file_name)
{
  FILE                    *file_ptr    = NULL;

  LOG_IN();
  file_ptr = fopen(file_name, "r");

  if (!file_ptr)
    {
      perror("fopen");
      FAILURE("could not open a file for parsing");
    }

  leonie->parser_file_handle = -1;
  leonie->parser_file_ptr    = file_ptr;
  LOG_OUT();
}


int
main (int argc, char *argv[])
{
  p_leo_app_application_t  application = NULL;
  p_ntbx_server_t          net_server  = NULL;

  LOG_IN();
  tbx_init();
  ntbx_init();

  main_leonie = TBX_MALLOC(sizeof(leonie_t));
  CTRL_ALLOC(main_leonie);
  main_leonie->parser_file_handle = -1;
  main_leonie->parser_file_ptr    = NULL;

  yyin = NULL;
#ifdef DEBUG
  yydebug = 1;
#endif /* DEBUG */
 
  if (argc < 2)
    FAILURE("no application description file specified");

  /*
   * application description file parsing
   * ------------------------------------
   */
  leo_open_local_parser_file(main_leonie, argv[1]);  

  fprintf(stderr, "Parsing application description file ... \n");
  
  if (yyparse())
    FAILURE("parse error (application file)");

  if (   !leo_parser_result->application
      ||  leo_parser_result->cluster
      ||  leo_parser_result->protocol_table)
    FAILURE("parse error (application file)");

  application = leo_parser_result->application;

  fprintf(stderr, "Starting application %s\n", application->id);

  /*
   * Net server initialization
   * -------------------------
   */
  net_server = malloc(sizeof(ntbx_server_t));
  CTRL_ALLOC(net_server);
  net_server->state = ntbx_server_state_uninitialized;
  ntbx_tcp_server_init(net_server);
  fprintf(stderr,
	  "net server ready and listening at %s\n",
	  net_server->connection_data);
  /*
   * Cluster intialization
   * ---------------------
   */
  {
    tbx_list_reference_t ref;

    tbx_list_reference_init(&ref, application->cluster_list);
    do
      {
	p_leo_app_cluster_t app_cluster = tbx_get_list_reference_object(&ref);

	fprintf(stderr,
		"Initializing MadII session on cluster %s ...\n",
		app_cluster->id);
      }
    while(tbx_forward_list_reference(&ref));
  }

  LOG_OUT();
  return 0;
}
