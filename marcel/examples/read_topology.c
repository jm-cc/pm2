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
#define MARCEL_KERNEL
#include "marcel.h"
#include "marcel_topology.h"
#include <stdio.h>
#include <stdlib.h>

char* level_descriptions[MARCEL_LEVEL_LAST+1] = 
  {
    "Whole machine",
#ifdef MA__LWPS
    "Fake", // level for meeting the marcel_topo_max_arity constraint
#  ifdef MA__NUMA
    "NUMA node",
    "Chip", // Physical chip
    "L3 cache",
    "L2 cache",
    "Core",
    "SMT Processor in a core",
#  endif
    "VP"
#endif
  };

FILE *output;

void indent(int i) {
  int x;
  for(x=0 ; x<i ; x++) marcel_fprintf(output, "  ");
}

void print_level(struct marcel_topo_level *l, int i) {
  indent(i);
  if (l->arity || (!i && !l->arity)) {
    marcel_fprintf(output, "\\pstree");
  }
  marcel_fprintf(output, "{\\TR{%s}}\n", level_descriptions[l->type]);
}

void f(struct marcel_topo_level *l, int i) {
  int x;

  print_level(l, i);
  if (l->arity || (!i && !l->arity)) {
    indent(i);
    marcel_fprintf(output, "{\n");
    for(x=0; x<l->arity; x++)
      f(l->children[x], i+1);
    indent(i);
    marcel_fprintf(output, "}\n");
  }
}

int main(int argc, char **argv) {
  struct marcel_topo_level *l;
  char hostname[256];

  marcel_init(&argc, argv);
  gethostname(hostname, 256);

  output = marcel_fopen("topology.tex", "w");
  marcel_fprintf(output, "\\documentclass[landscape,a4paper,10pt]{article}\n");
  marcel_fprintf(output, "\\usepackage{fullpage}\n");
  marcel_fprintf(output, "\\usepackage{pstricks,pst-node,pst-tree}\n");
  marcel_fprintf(output, "\\begin{document}\n");
  marcel_fprintf(output, "\\title{%s: Topology}\n", hostname);
  marcel_fprintf(output, "\\author{Marcel}\n");
  marcel_fprintf(output, "\\maketitle\n");
  marcel_fprintf(output, "\\tiny\n");

  l = &marcel_topo_levels[0][0];
  f(l, 0);

  marcel_fprintf(output, "\\end{document}\n");
  marcel_fclose(output);

  marcel_fprintf(stdout, "The file topology.tex has been created. Compile it as follows:\n");
  marcel_fprintf(stdout, "\tlatex topology\n");
  marcel_fprintf(stdout, "\tdvips -Ppdf  -t landscape topology.dvi\n");
  marcel_fprintf(stdout, "\tps2pdf topology.ps\n");
  marcel_end();
  return 0;
}



