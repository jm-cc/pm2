
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>

#include "shell.h"

pid_t exec_single_cmd(int *output_fd, char *argv[])
{
  pid_t pid;
  int fd[2];

#ifdef DEBUG
  {
    int i;

    for(i=0; argv[i]; i++)
      fprintf(stderr, "argv[%d] = <%s>\n", i, argv[i]);
  }
#endif

  if(output_fd) {

    if(pipe(fd) == -1) {
      perror("pipe");
      exit(1);
    }

    *output_fd = fd[0];

  }

  pid=fork();

  if(pid == -1) {
    perror("fork");
    exit(1);
  }

  if(pid) { /* father process */

    if(output_fd)
      close(fd[1]);

  } else { /* child process */

    if(output_fd) {

      close(fd[0]);
      dup2(fd[1], STDOUT_FILENO);
      dup2(fd[1], STDERR_FILENO);
      close(fd[1]);

    } else {
      int fderr = open("/tmp/ezflavor.errlog", O_WRONLY | O_CREAT, 0600);

      dup2(fderr, STDOUT_FILENO);
      dup2(fderr, STDERR_FILENO);
      close(fderr);
    }

    execvp(argv[0], argv);
    perror("execvp");

  }

  return pid;
}

pid_t exec_single_cmd_args(int *output_fd, char *cmd, char *arg, ...)
{
  char *argv[128];
  int nb = 0;

  argv[nb++] = cmd;
  argv[nb++] = arg;
  if(arg) {
    va_list vl;

    va_start(vl, arg);

    for(;;) {
      argv[nb] = va_arg(vl, char *);
      if(!argv[nb])
	break;
      nb++;
    }

    va_end(vl);
  }

  return exec_single_cmd(output_fd, argv);
}

pid_t exec_single_cmd_fmt(int *output_fd, char *fmt, ...)
{
  static char cmd[4096];
  char *argv[128];
  va_list vl;
  unsigned nb = 0, i = 0;

  va_start(vl, fmt);
  vsprintf(cmd, fmt, vl);
  va_end(vl);

  for(;;) {
    while(cmd[i] != '\0' && isspace(cmd[i]))
      i++;

    if(cmd[i] == '\0')
      break;

    argv[nb++] = &cmd[i];

    while(cmd[i] != '\0' && !isspace(cmd[i]))
      i++;

    if(cmd[i] == '\0')
      break;

    cmd[i++] = '\0';
  }

  argv[nb] = NULL;

  return exec_single_cmd(output_fd, argv);
}

int exec_wait(pid_t pid)
{
  int status;

  waitpid(pid, &status, 0);

  return WEXITSTATUS(status);
}
