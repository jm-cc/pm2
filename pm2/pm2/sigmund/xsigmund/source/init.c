#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "sigmund.h"
#include "parser.h"
#include "init.h"


/* Initialize the event list... 
   read fkt.h, fut.h ...
 */

static void code_copy(int code, char *name)
{
  code_table[nb_code].code = code;
  code_table[nb_code].name = name;
  nb_code++;
}


static char *pm2_root(void)
{
  char *ptr= getenv("PM2_ROOT");

  if(!ptr) {
    fprintf(stderr, "Error: undefined PM2_ROOT variable\n");
    exit(1);
  }
  
  return ptr;
}


static void get_builddir(char *builddir)
{
  int Fd[2];
  int pid;
  char pm2_config[500];
  if (pipe(Fd)) {
    fprintf(stderr,"Unable to pipe\n");
    exit(1);
  }
  if ((pid = fork()) == 0) {
    char *arg[3];
    close(Fd[0]);
    dup2(Fd[1], STDOUT_FILENO);
    strcpy(pm2_config, pm2_root());
    strcat(pm2_config, "/bin/pm2-config");
    arg[0] = pm2_config;
    arg[1] = "--builddir";
    arg[2] = NULL;
    execv(pm2_config, arg);
    fprintf(stderr,"Unable to run %s\n", pm2_config);
    exit(1);
  } else {
    int l;
    close(Fd[1]);
    l = read(Fd[0], builddir, 500);
    if (l == 0) {
      fprintf(stderr,"Could not get builddir information\n");
      exit(1);
    }
    close(Fd[0]);
    waitpid(pid, NULL, 0);
    if (builddir[l-1] == '\n') builddir[l-1] = '\0';
  }

}


void init_names()
{ 
  int fd; 
  char fut_print[550];
  nb_code = 0;
  get_builddir(fut_print);
  strcat(fut_print, "/profile/include/fut_print.h");
  fd = open(fut_print, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "%s could not be opened\n", fut_print);
    exit(1);
  }
  parser_start(fd);
  parser_run();
  parser_stop();
  close(fd);
  code_copy(0, NULL);
}
