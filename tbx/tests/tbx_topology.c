/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2011 "the PM2 team" (see AUTHORS file)
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
#include "tbx.h"
#include "tbx_topology.h"


#define ARG_MAX 3
#define BUF_SZ  100


#ifdef PM2_TOPOLOGY


int main()
{
	int  test_argc = ARG_MAX;
	char *test_argv[BUF_SZ];

	test_argv[0] = "tbx_topology";
	test_argv[ARG_MAX+1] = NULL;

	// invalid destroy
	tbx_topology_destroy();
	
	// --synthetic-topology
	test_argv[1] = "--synthetic-topology";
	test_argv[2] = "2 2 4";
	tbx_topology_init(test_argc, test_argv);
	tbx_topology_init(test_argc, test_argv);
	tbx_topology_destroy();

	// invalid xml file
	test_argv[1] = "--topology-xml";
	test_argv[2] = "/tmp/null.xml";
	tbx_topology_init(test_argc, test_argv);
	tbx_topology_destroy();

	// invalid --topology-fsys-root argument
	test_argv[1] = "--topology-fsys-root";
	test_argv[2] = "/tmp/nulldir";
	tbx_topology_init(test_argc, test_argv);
	tbx_topology_destroy();

	return EXIT_SUCCESS;
}


#else


/** topology module not supported: return TEST_SKIPPED for autotests */
int main()
{
	return 77;
}


#endif
