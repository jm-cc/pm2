
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


