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
static const char *usage_str = "leonie <app_conf_file>";

void
leo_usage(void)
{
  fprintf(stderr, "Usage: %s\n", usage_str);
  exit(EXIT_FAILURE);
}

void
leonie_failure_cleanup(void)
{
  pid_t pid = 0;
  
  LOG_IN();
  DISP("A failure has been detected, hit return to close the session");
  getchar();
  pid = getpid();
  kill(-pid, SIGTERM);
  LOG_OUT();
}

void
leo_terminate(const char *msg)
{
  fprintf(stderr, "error: %s\n", msg);
  leonie_failure_cleanup();
  exit(EXIT_FAILURE);
}

void
leo_error(const char *command) 
{
  perror(command);
  leonie_failure_cleanup();
  exit(EXIT_FAILURE);
}

void
leonie_processes_cleanup(void)
{
  pid_t pid = 0;
  
  LOG_IN();
  pid = getpid();
  kill(-pid, SIGHUP);
  LOG_OUT();
}

