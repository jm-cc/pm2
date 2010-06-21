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


#ifdef PM2_TOPOLOGY

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbx_topology.h"
#include "tbx_types.h"

hwloc_topology_t topology;
char *synthetic_topology_description = NULL;

static tbx_bool_t already_defined = tbx_false;

static void show_synthetic_topology_help(void);


void tbx_topology_init(int argc, char *argv[])
{
	int i;
	char *topology_fsys_root_path = NULL;

	if (already_defined)
		return;

	// get topology description helpers from options (arguments)
	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "--synthetic-topology")) {
			if (i < argc - 1)
				synthetic_topology_description =
				    argv[i + 1];
			else {
				fprintf(stderr,
					"Fatal error: --synthetic-topology option must be followed "
					"by <topology-description>.\n");
				show_synthetic_topology_help();
				exit(1);
			}
		}

		if (!strcmp(argv[i], "--topology-fsys-root")) {
			if (i < argc - 1)
				topology_fsys_root_path = argv[i + 1];
			else {
				fprintf(stderr,
					"Fatal error: --topology-fsys-root option must be followed "
					"by the path of a directory.\n");
				exit(1);
			}
		}
	}

  hwloc_topology_init(&topology);
  if (topology_fsys_root_path)
    hwloc_topology_set_fsroot(topology, topology_fsys_root_path);
  if (synthetic_topology_description)
    hwloc_topology_set_synthetic(topology, synthetic_topology_description);
  hwloc_topology_load(topology);

	already_defined = tbx_true;
}

void tbx_topology_destroy()
{
	if (already_defined) {
		hwloc_topology_destroy(topology);
		already_defined = tbx_false;
	}
}



static void show_synthetic_topology_help(void)
{
	fprintf(stderr,
		"<topology-description> is a space-separated list of positive integers,\n"
		"each of which denotes the number of children attached to a each node of the\n"
		"corresponding topology level.\n\n"
		"Example: \"2 4 2\" denotes a 2-node machine with 4 dual-core CPUs.\n");
}

#endif				/* PM2_TOPOLOGY */
