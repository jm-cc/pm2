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
$Log: swann_run.c,v $
Revision 1.2  2000/03/27 12:54:06  oaumage
- progression des fonctionnalites:
  * support reseau
  * support execution
  * extension du support fichier

Revision 1.1  2000/02/17 09:29:32  oaumage
- ajout des fichiers constitutifs de Swann


______________________________________________________________________________
*/

/*
 * swann_run.c
 * ===========
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "swann.h"

swann_status_t
swann_run_command(p_swann_file_t  file,
		  char           *argv[],
		  char           *envp[],
		  int            *return_code)
{
  pid_t child_pid;

  if (   file->state != swann_file_state_initialized
	 && file->state != swann_file_state_closed)
    FAILURE("invalid file state");

  if (file->mode != swann_file_mode_uninitialized)
    FAILURE("invalid file mode");

  if (!file->exist)
    FAILURE("file not found");

  if (!file->executable)
    FAILURE("file not executable");

  child_pid = fork();

  if (child_pid == -1)
    {
      perror("fork");
      FAILURE("swann_run_command");
    }

  if (child_pid)
    {
      /* Father */
      pid_t result;
      int   child_status;

      result = waitpid(child_pid, &child_status, 0);

      if (result == -1)
	{
	  perror("waitpid");
	  FAILURE("swann_run_command");
	}

      if (WIFSIGNALED(child_status))
	FAILURE("child killed by unhandled signal, aborting");

      if (WIFEXITED(child_status))
	{
	  *return_code = WEXITSTATUS(child_status);
	}
      else
	{
	  *return_code = 0;
	}

      return swann_success;
    }
  else
    {
      /* Child */
      int status;

      status = execve(file->pathname, argv, envp);

      if (status == -1)
	{
	  perror("execve");
	  FAILURE("swann_run_command");
	}
      else
	FAILURE("invalid execve syscal return value");
    }
}

p_swann_command_t
swann_run_async_command(p_swann_file_t  file,
			char           *argv[],
			char           *envp[],
			p_swann_file_t  file_in,
			p_swann_file_t  file_out,
			p_swann_file_t  file_err)
{
  pid_t child_pid;

  if (   file->state != swann_file_state_initialized
	 && file->state != swann_file_state_closed)
    FAILURE("invalid file state");

  if (file->mode != swann_file_mode_uninitialized)
    FAILURE("invalid file mode");

  if (!file->exist)
    FAILURE("file not found");

  if (!file->executable)
	FAILURE("file not executable");

  child_pid = fork();

  if (child_pid == -1)
    {
      perror("fork");
      FAILURE("swann_run_async_command");
    }

  if (child_pid)
    {
      /* Father */
      p_swann_command_t command;

      command = malloc(sizeof(swann_command_t));
      CTRL_ALLOC(command);

      command->command_state = swann_command_state_running;
      command->file          = file;
      command->argv          = argv;
      command->pid           = child_pid;
      command->return_value  = 0;
      command->file_in       = file_in;
      command->file_out      = file_out;
      command->file_err      = file_err;

      return swann_success;
    }
  else
    {
      /* Child */
      int status;

      if (file_in)
	{
	  int fd;

	  while ((fd = dup2(file_in->descriptor, 0)) == -1)
	    if (errno != EINTR)
	      {
		perror("dup2");
		FAILURE("swann_run_async_command");
	      }

	  if (fd != 0)
	    FAILURE("dup2 returned a wrong fd");
	}

      if (file_out)
	{
	  int fd;

	  while ((fd = dup2(file_out->descriptor, 1)) == -1)
	    if (errno != EINTR)
	      {
		perror("dup2");
		FAILURE("swann_run_async_command");
	      }

	  if (fd != 1)
	    FAILURE("dup2 returned a wrong fd");
	}

      if (file_err)
	{
	  int fd;

	  while ((fd = dup2(file_err->descriptor, 2)) == -1)
	    if (errno != EINTR)
	      {
		perror("dup2");
		FAILURE("swann_run_async_command");
	      }

	  if (fd != 2)
	    FAILURE("dup2 returned a wrong fd");
	}

      status = execve(file->pathname, argv, envp);

      if (status == -1)
	{
	  perror("execve");
	  FAILURE("swann_run_async_command");
	}
      else
	FAILURE("invalid execve syscal return value");
    }  
}


