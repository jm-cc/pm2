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

______________________________________________________________________________
$Log: leonie.c,v $
Revision 1.5  2000/05/15 13:51:56  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/

/*
 * Leonie.c
 * ========
 */ 
/* #define DEBUG */
/* #define TRACING */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "leonie.h"

/*
 * Constants and Macros
 * --------------------
 */
#define MAX_FILENAME_SIZE 1024

/*
 * Static Variables
 * ----------------
 */
static p_leonie_t main_leonie = NULL;

/*
 * Functions
 * ==========================================================================
 */

p_leonie_t leo_init()
{
  p_leonie_t leonie = NULL;
  
  leonie = TBX_MALLOC(sizeof(leonie_t));
  CTRL_ALLOC(leonie);

  leonie->cluster_counter          = 0;
  
  tbx_list_init(&leonie->swann_modules);

  return leonie;
}



/*
 * Initialization
 * --------------
 */
int
main (int argc, char *argv[])
{
  p_leo_app_application_t  application       = NULL;
  p_leo_clu_cluster_file_t local_cluster_def = NULL;
  char                    *filename          = NULL;

  LOG_IN();
  tbx_init();
  ntbx_init();
  main_leonie = leo_init();
  leo_parser_init();
 
  if (argc < 2)
    FAILURE("no application description file specified");

  application = leo_parse_application_file(argv[1]);
  DISP("Starting application %s", application->id);

  main_leonie->net_server = leo_net_server_init();

  /*
   * Local cluster information retrieval
   * -----------------------------------
   */
  filename = malloc(MAX_FILENAME_SIZE);
  strcpy(filename, getenv("HOME"));
  strcat(filename, "/.pm2/leo_cluster");
  local_cluster_def = leo_parse_cluster_file(NULL, filename);

  /*
   * Cluster initialization
   * ---------------------
   */
  leo_cluster_setup(main_leonie, application, local_cluster_def);

  LOG_OUT();
  return 0;
}
