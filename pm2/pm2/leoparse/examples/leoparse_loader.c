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
$Log: leoparse_loader.c,v $
Revision 1.1  2000/12/19 16:57:45  oaumage
- finalisation de leoparse
- exemples pour leoparse
- modification des macros de logging
- version typesafe de certaines macros
- finalisation des tables de hachage
- finalisation des listes de recherche

Revision 1.2  2000/11/16 13:24:03  oaumage
- mise a jour initialisation


______________________________________________________________________________
*/

/*
 * leoparse_loader.c
 * =================
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leoparse.h"

const char *pm2_conf               = "pm2conf";
const char *pm2_load               = "pm2load";
const char *pm2_conf_flavor_option = "-f";
const char *pm2_load_flavor_option = "-f";

void
usage(void)
{
  fprintf(stderr, "usage: leoparse_loader <conf_file>\n");
  exit(EXIT_FAILURE);
}

void
terminate(char *msg)
{
  fprintf(stderr, "error: %s\n", msg);
  exit(EXIT_FAILURE);
}

void
error(char *command)
{
  perror(command);
  exit(EXIT_FAILURE);
}

void
call_execvp(const char *      file,
	          char *const argv[])
{
  pid_t pid = -1;

  pid = fork();

  if (pid == -1)
    {
      error("fork");
    }

  if (pid)
    {
      pid_t result = -1;
      int   status =  0;
      
      result = waitpid(pid, &status, 0);
      if (result == -1)
	{
	  error("waitpid");
	}

      if (!WIFEXITED(status) || WEXITSTATUS(status))
	{
	  terminate("subprocess aborted unexpectedly");
	}
    }
  else
    {
      execvp(file, argv);
      error("execvp");
    }
}

void
process(p_tbx_htable_t application)
{
  p_tbx_slist_t               configuration = NULL;
  char                       *name          = NULL;
  char                       *flavor        = NULL;
  char                      **command       = NULL;
  char                      **ptr           = NULL;
  const int                   extra_fields  =    3;

  configuration = leoparse_read_slist(application, "configuration");

  name   = leoparse_read_id(application, "name");
  TRACE_STR("Name", name);
  
  flavor = leoparse_read_id(application, "flavor");
  TRACE_STR("Flavor", flavor);
  
  if (!tbx_slist_is_nil(configuration))
    {
      int length = tbx_slist_get_length(configuration);
      int i      = extra_fields;

      command = malloc((length + extra_fields + 1) * sizeof(char *));
      CTRL_ALLOC(command);

      tbx_slist_ref_to_head(configuration);

      do
	{
	  p_leoparse_object_t  object = NULL;
	  char                *host   = NULL;
	    
	  object = tbx_slist_ref_get(configuration);
	  host = leoparse_get_string(object);

	  command[i] = malloc(strlen(host) + 1);
	  CTRL_ALLOC(command[i]);

	  strcpy(command[i], host);
	  TRACE_STR("Host", host);
	  i++;
	}
      while (tbx_slist_ref_forward(configuration));
      
      command[i] = NULL;
    }
  else
    {
      terminate("parse error : no configuration");
    }

  command[0] = malloc(strlen(pm2_conf) + 1);
  CTRL_ALLOC(command[0]);
  strcpy(command[0], pm2_conf);
  
  command[1] = malloc(strlen(pm2_conf_flavor_option) + 1);
  CTRL_ALLOC(command[1]);
  strcpy(command[1], pm2_conf_flavor_option);

  command[2] = malloc(strlen(flavor) + 1);
  CTRL_ALLOC(command[2]);
  strcpy(command[2], flavor);

  call_execvp(pm2_conf, command);

  ptr = command;
  while (*ptr)
    {
      free(*ptr);
      *ptr = NULL;
      ptr++;
    }

  free(command);
  command = NULL;

  command = malloc((1 + extra_fields + 1) * sizeof(char *));
  CTRL_ALLOC(command);

  command[0] = malloc(strlen(pm2_load) + 1);
  CTRL_ALLOC(command[0]);
  strcpy(command[0], pm2_load);
  
  command[1] = malloc(strlen(pm2_load_flavor_option) + 1);
  CTRL_ALLOC(command[1]);
  strcpy(command[1], pm2_load_flavor_option);

  command[2] = malloc(strlen(flavor) + 1);
  CTRL_ALLOC(command[2]);
  strcpy(command[2], flavor);

  command[3] = malloc(strlen(name) + 1);
  CTRL_ALLOC(command[3]);
  strcpy(command[3], name);

  command[4] = NULL;
  
  call_execvp(pm2_load, command);

  ptr = command;
  while (*ptr)
    {
      free(*ptr);
      *ptr = NULL;
      ptr++;
    }

  free(command);
  command = NULL;  
}

int
main(int    argc,
     char **argv)
{
  p_tbx_htable_t            result = NULL;
  p_leoparse_htable_entry_t entry  = NULL;

  common_init(&argc, argv);
  
  tbx_init(argc, argv);
  leoparse_init(argc, argv);

  tbx_purge_cmd_line(&argc, argv);
  leoparse_purge_cmd_line(&argc, argv);

  argc--; argv++;
  if (!argc)
    usage();

  result = leoparse_parse_local_file(*argv);
  TRACE("Parsing succeeded");

  entry = tbx_htable_get(result, "application");
  
  process(leoparse_get_htable(leoparse_get_object(entry)));
    
  return 0;
}

