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
#define MARCEL_INTERNAL_INCLUDE
#include "marcel.h"
#include "marcel_topology.h"
#include <stdio.h>
#include <stdlib.h>

#define indent(output, i) \
  marcel_fprintf(output, "%*s", 2*i, "");

static void output_topology(struct marcel_topo_level *l, FILE *output, int i, int verbose_mode) {
  int x;
  const char * separator = " ";
  const char * indexprefix = "#";
  const char * labelseparator = ":";
  const char * levelterm = "";

  indent(output, i);
  marcel_print_level(l, output, verbose_mode, separator, indexprefix, labelseparator, levelterm);
  if (l->arity || (!i && !l->arity)) {
    for(x=0; x<l->arity; x++)
      output_topology(l->children[x], output, i+1, verbose_mode);
  }
}


int marcel_main(int argc, char **argv) {
  FILE *output = stdout;
  int verbose_mode = 0;

  marcel_init(&argc, argv);

  while (argc >= 2) {
    if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--verbose")) {
      verbose_mode = 1;
    } else {
      marcel_fprintf(stderr, "Unrecognized options: %s\n", argv[1]);
    }
    argc--;
    argv++;
  }

  output_topology(marcel_topo_level(0, 0), output, 0, verbose_mode);

#ifdef MA__NUMA
  if (verbose_mode) {
    int l,i;
    for (l = 0; l <= MARCEL_LEVEL_LAST; l++) {
      int depth = ma_get_topo_type_depth (l);
      for (i = 0; i<depth; i++)
	marcel_fprintf(output, " ");
      marcel_fprintf(output, "depth %d:\tmarcel type #%d (%s)\n", depth, l, marcel_topo_level_string(l));
    }
  }
#endif

  marcel_end();
  return 0;
}
