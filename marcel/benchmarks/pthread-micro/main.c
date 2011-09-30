/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2010, 2011 "the PM2 team" (see AUTHORS file)
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

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


#define TEST_TIME 4
#define TEST_AUTOBENCHMARK_ENV "PT_AUTOBENCH"


static unsigned int  nproc;
static unsigned int  isend;
static unsigned long count;
static char         *testname;


static void test_exec(void);
static void test_print_results(int sig);
static void print_results(void);


static void print_testname(char *name)
{
	testname = NULL;

	// need to find the real testname !!!
	name = strtok(name, "/");
	do {
		testname = name;
		name = strtok(NULL, "/");
	} while (name);

	if (getenv(TEST_AUTOBENCHMARK_ENV))
		printf("%s|%d|%d|", testname, TEST_TIME, nproc);
	else
		printf("-- %s test (duration: %ds, nbcpus: %d)\n", testname, TEST_TIME, nproc);
}

static void print_results(void)
{
	if (getenv(TEST_AUTOBENCHMARK_ENV))
		printf("%ld\n", count/TEST_TIME);
	else
		printf("%ld %s in %d seconds: [%ld/s]\n", count, strstr(testname, "_")+1, TEST_TIME, count/TEST_TIME);	
}


int main(int argc, char *argv[])
{
	struct sigaction sa;
	
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = test_print_results;
	if (0 != sigaction(SIGALRM, &sa, NULL)) {
		perror("sigaction");
		exit(1);
	}

	isend = 0;
	nproc = sysconf(_SC_NPROCESSORS_ONLN);

	print_testname(argv[0]);

	alarm(TEST_TIME);
	test_exec();
	
	return 0;
}
