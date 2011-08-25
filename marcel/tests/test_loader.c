/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "tbx_compiler.h"

#define  TIMEOUT      250
#define  BUF_SZ       2048
#define  ITER_MAX     8

#define  AUTOTEST_SKIPPED_TEST 77

#define  ITER_MAX_ENV "MARCEL_TEST_ITER"
#define  TIMEOUT_ENV  "MARCEL_TEST_TIMEOUT"


static pid_t child_pid = 0;

static void test_cleaner(int sig TBX_UNUSED)
{
	pid_t child_gid;

	// send signal to all loader family members
	child_gid = getpgid(child_pid);
	kill(-child_gid, SIGKILL);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int   i, iter_max, timeout;
	int   child_exit_status;
	char *ld_preload;
	char *ld_library_path;
	char *test_name;
	int   status;
	struct rlimit rlim;
	struct sigaction sa;
	char  env_buf[BUF_SZ];

	timeout = iter_max = 0;
	test_name = ld_preload = ld_library_path = NULL;
	for (i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "LD_PRELOAD", strlen("LD_PRELOAD"))) {
			ld_preload = argv[i] + strlen("LD_PRELOAD") + 1;
			continue;
		}

		if (!strncmp(argv[i], "LD_LIBRARY_PATH", strlen("LD_LIBRARY_PATH"))) {
			ld_library_path = argv[i] + strlen("LD_LIBRARY_PATH") + 1;
			continue;
		}

		test_name = argv[i];
	}

	/* get user-defined iter_max value */
#ifndef PM2VALGRIND
	if (getenv(ITER_MAX_ENV))
		iter_max = strtol(getenv(ITER_MAX_ENV), NULL, 10);
	if (iter_max <= 0)
		iter_max = ITER_MAX;
#else
	iter_max = 1;
#endif

	if (getenv(TIMEOUT_ENV))
		timeout = strtol(getenv(TIMEOUT_ENV), NULL, 10);
	if (timeout <= 0)
		timeout = TIMEOUT;

	/* set SIGALARM handler */
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = test_cleaner;
	if (-1 == sigaction(SIGALRM, &sa, NULL))
		perror("sigaction");

	/* autotest unset stack user limit --> set to 10MB */
	rlim.rlim_cur = rlim.rlim_max = 10 * 1024 * 1024;
	setrlimit(RLIMIT_STACK, &rlim);

	i = 0;
	while (i++ < iter_max) {
		child_pid = fork();
		if (0 == child_pid) {
			// get a new pgid
			if (-1 == setpgid(0, 0)) {
				perror("setpgid");
				exit(EXIT_FAILURE);
			}

			if (ld_preload) {
				sprintf(env_buf, "LD_PRELOAD=%s", ld_preload);
				putenv(env_buf);
			}

			if (ld_library_path) {
				sprintf(env_buf, "LD_LIBRARY_PATH=%s", ld_library_path);
				putenv(env_buf);
			}

			/** put LD_BIND_NOW always: normally required 
			 *  only when lwps support is enabled */
			putenv("LD_BIND_NOW=y");

#ifdef __GLIBC__
			/** find bugs related to dynamically allocated area **/
			putenv("MALLOC_CHECK_=2");
			putenv("MALLOC_PERTURB_=0x77");
#endif

			if (test_name) {
#ifdef PM2VALGRIND
				execlp("valgrind", "valgrind", test_name, NULL);
#else
				execl(test_name, test_name, NULL);
#endif
			}
			exit(EXIT_FAILURE);
		}
		if (-1 == child_pid)
			exit(EXIT_FAILURE);

		alarm(timeout);
		if (child_pid == waitpid(child_pid, &child_exit_status, 0)) {
			if (WIFEXITED(child_exit_status)) {
				status = WEXITSTATUS(child_exit_status);
				if (EXIT_SUCCESS == status) {
					alarm(0);
					continue;
				} else {
					if (AUTOTEST_SKIPPED_TEST != status)
						printf("Exited with return code %d\n", status);
					return status;
				}
			} else {
				printf("Interrupted by a signal maybe\n");
				return EXIT_FAILURE;
			}
		}
	}

	return EXIT_SUCCESS;
}
