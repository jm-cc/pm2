
#ifndef SHELL_IS_DEF
#define SHELL_IS_DEF

#include <unistd.h>
#include <stdlib.h>


// These three functions return the pid of the child process
pid_t exec_single_cmd(int *output_fd, char *argv[]);

pid_t exec_single_cmd_args(int *output_fd, char *cmd, char *arg, ...);

pid_t exec_single_cmd_fmt(int *output_fd, char *fmt, ...);

int exec_wait(pid_t pid);

#endif
