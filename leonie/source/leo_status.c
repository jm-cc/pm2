/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
 * leo_status.c
 * ============
 *
 * - user level messages
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "leonie.h"

/*
 * Static variables
 * ================
 */

/*
 * Usage
 * -----
 */
static const char *usage_str = "leonie [OPTION]... CONFIG_FILE [APP_OPTION]...";

void
leo_usage(void)
{
  fprintf(stderr, "Usage: %s\n", usage_str);
  exit(EXIT_FAILURE);
}

static
void
__leo_option_help(char *s_opt, char *l_opt, char *txt)
{
  if (strlen(l_opt) > 20)
    {
      char *comma = NULL;
      char buf_txt[41];
      char buf_opt[61];

      strncpy(buf_opt, l_opt, 60);
      buf_opt[60] = '\0';

      if (*s_opt && *l_opt)
	{
	  comma = ",";
	}
      else
	{
	  comma = " ";
	}

      fprintf(stderr, "%5s%s %s\n", s_opt, comma, buf_opt);

      if (strlen(l_opt) > 60)
	{
	  l_opt += 60;

	  while (strlen(l_opt) > 60)
	    {
	      strncpy(buf_opt, l_opt, 60);
	      buf_opt[60] = '\0';
	      fprintf(stderr, "      %s\n", buf_opt);
	      l_opt += 60;
	    }

	  fprintf(stderr, "      %s\n", buf_opt);
	}

      while (strlen(txt) > 40)
	{
	  strncpy(buf_txt, txt, 40);
	  buf_txt[40] = '\0';
	  fprintf(stderr, "                            %s\n", buf_txt);
	  txt += 40;
	}

      fprintf(stderr, "                            %s\n", txt);
    }
  else
    {
      char *comma = NULL;
      char buf_txt[41];

      strncpy(buf_txt, txt, 40);
      buf_txt[40] = '\0';
      if (*s_opt && *l_opt)
	{
	  comma = ",";
	}
      else
	{
	  comma = " ";
	}

      fprintf(stderr, "%5s%s %-20s %s\n", s_opt, comma, l_opt, buf_txt);

      if (strlen(txt) > 40)
	{
	  txt += 40;

	  while (strlen(txt) > 40)
	    {
	      strncpy(buf_txt, txt, 40);
	      buf_txt[40] = '\0';
	      fprintf(stderr, "                            %s\n", buf_txt);
	      txt += 40;
	    }

	  fprintf(stderr, "                            %s\n", txt);
	}
    }
}

void
leo_help(void)
{
  fprintf(stderr, "Usage: %s\n", usage_str);
  fprintf(stderr, "Launch the PM2 session described in CONFIG_FILE\n\n");

  __leo_option_help("", "--appli=APPLICATION",
		    "Select APPLICATION as executable");
  __leo_option_help("-d", "",
		    "Open session processes in gdb           (implies  -x)");
  __leo_option_help("[--d]", "",
		    "Do not open session processes in gdb");
  __leo_option_help("", "--numactl=VALUE",
		    "Open session processes with numactl");
  __leo_option_help("-vg", "",
		    "Open session processes in valgrind      (implies  -x and -p)");
  __leo_option_help("[--vg]", "",
		    "Do not open session processes in valgrind");
  __leo_option_help("-e", "",
		    "Export PATH, PM2_ROOT, PM2_SRCROOT, PM2_OBJROOT and LEO_XTERM     to client nodes");
  __leo_option_help("[--e]", "",
		    "Do not export local PATH, PM2_ROOT, PM2_SRCROOT, PM2_OBJROOT and  LEO_XTERM to client nodes");

  __leo_option_help("", "--flavor=FLAVOR",
		    "Select FLAVOR as the application flavor");
  __leo_option_help("", "--leonie-host=HOSTNAME",
		    "Force HOSTNAME to be the name of the    machine which leonie is started on      (default: gethostname/gethostbyname)");
  __leo_option_help("", "--wd=WORKING_DIR",
		    "Change working directory to WORKING     _DIR at the beginning of session");
  __leo_option_help("[-cd]", "",
		    "Change working directory at the         beginning of session");
  __leo_option_help("--cd", "",
		    "Do not change working directory at      the beginning of session");
  __leo_option_help("-l", "",
		    "Log session processes output to files");
  __leo_option_help("[--l]", "",
		    "Do not log session processes output");
  __leo_option_help("", "--net=NETWORK_FILE[,NETWORK_FILE]...",
		    "Use NETWORK_FILE for network definitions");
  __leo_option_help("[-p]", "",
		    "Pause after session processes           termination");
  __leo_option_help("--p", "",
		    "Do not pause after session processes    termination");

  __leo_option_help("", "-smp",
		    "Preload required libraries for Marcel   SMP support");
  __leo_option_help("", "--smp",
		    "Do not preload Marcel SMP support       libraries");

  __leo_option_help("[-x]", "", "Open an Xterm for each session process");
  __leo_option_help("--x", "", "Do not open Xterms for session processes");

  __leo_option_help("-w", "", "Wait for the loader to fork");
  __leo_option_help("[--w]", "", "Do not wait for the loader to fork");

  exit(EXIT_SUCCESS);
}

void
leonie_failure_cleanup(p_leo_settings_t settings)
{
  pid_t pid = 0;

  if (settings && settings->pause_mode) {
    fprintf(stderr, "A failure has been detected, hit return to close the session\n");
    getchar();
  }
  else {
    fprintf(stderr, "A failure has been detected, killing process ...\n");
  }
  pid = getpid();
  kill(-pid, SIGTERM);
}

void
leo_terminate(const char *msg, p_leo_settings_t settings)
{
  fprintf(stderr, "error: %s\n", msg);
  leonie_failure_cleanup(settings);
  exit(EXIT_FAILURE);
}

void
leo_error(const char *command, p_leo_settings_t settings)
{
  perror(command);
  leonie_failure_cleanup(settings);
  exit(EXIT_FAILURE);
}

void
leonie_processes_cleanup(void)
{
  pid_t pid = 0;

  pid = getpid();
  kill(-pid, SIGHUP);
}

