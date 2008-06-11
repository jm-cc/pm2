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

void indent(FILE *output, int i) {
  int x;
  for(x=0 ; x<i ; x++) marcel_fprintf(output, "  ");
}

void print_level(struct marcel_topo_level *l, FILE *output, int i, int txt_mode) {
  indent(output, i);
  if (!txt_mode && (l->arity || (!i && !l->arity))) {
    marcel_fprintf(output, "\\pstree");
  }
  if (!txt_mode) {
    marcel_fprintf(output, "{\\Level{c}{");
  }
  marcel_fprintf(output, "%s", level_descriptions[l->type]);
  if (l->os_node != -1) marcel_fprintf(output, "\\\\Node %u", l->os_node);
  if (l->os_die != -1)  marcel_fprintf(output, "\\\\Die %u" , l->os_die);
  if (l->os_l3 != -1)   marcel_fprintf(output, "\\\\L3 %u"  , l->os_l3);
  if (l->os_l2 != -1)   marcel_fprintf(output, "\\\\L2 %u"  , l->os_l2);
  if (l->os_core != -1) marcel_fprintf(output, "\\\\Core %u", l->os_core);
  if (l->os_cpu != -1)  marcel_fprintf(output, "\\\\CPU %u" , l->os_cpu);

  if (l->level == marcel_topo_nblevels-1) {
    marcel_fprintf(output, "\\\\VP %u", l->number);
  }
  if (txt_mode) {
    marcel_fprintf(output,"\n");
  } else {
    marcel_fprintf(output,"}}\n");
  }
}

void f(struct marcel_topo_level *l, FILE *output, int i, int txt_mode) {
  int x;

  print_level(l, output, i, txt_mode);
  if (l->arity || (!i && !l->arity)) {
    indent(output, i);
    marcel_fprintf(output, "{\n");
    for(x=0; x<l->arity; x++)
      f(l->children[x], output, i+1, txt_mode);
    indent(output, i);
    marcel_fprintf(output, "}\n");
  }
}

int marcel_main(int argc, char **argv) {
  struct marcel_topo_level *l;
  char hostname[256], filename[256];
  FILE *output;
  int txt_mode = 0;

  marcel_init(&argc, argv);
  if (argc >= 2 && !strcmp(argv[1], "--txt")) {
    txt_mode = 1;
  }

  if (txt_mode) {
    output = stdout;
  }
  else {
    gethostname(hostname, 256);
    marcel_sprintf(filename, "%s_topology.tex", hostname);
    output = marcel_fopen(filename, "w");

    marcel_fprintf(output, "\\documentclass[landscape,a4paper,10pt]{article}\n");
    marcel_fprintf(output, "\\usepackage{fullpage}\n");
    marcel_fprintf(output, "\\usepackage{pst-tree}\n");
    marcel_fprintf(output, "% Macro to define one level\n");
    marcel_fprintf(output, "\\newcommand{\\Level}[3][]{\\TR[#1]{\\setlength{\\tabcolsep}{0mm}\\begin{tabular}[t]{#2}#3\\end{tabular}}}\n\n");
    marcel_fprintf(output, "\\begin{document}\n");
    marcel_fprintf(output, "\\title{%s: Topology}\n", hostname);
    marcel_fprintf(output, "\\author{Marcel}\n");
    marcel_fprintf(output, "\\maketitle\n");
    marcel_fprintf(output, "\\tiny\n");
  }

  l = &marcel_topo_levels[0][0];
  f(l, output, 0, txt_mode);

  if (!txt_mode) {
    marcel_fprintf(output, "\\end{document}\n");
    marcel_fclose(output);

    marcel_fprintf(stdout, "The file %s_topology.tex has been created. Compile it as follows:\n", hostname);
    marcel_fprintf(stdout, "\tlatex %s_topology\n", hostname);
    marcel_fprintf(stdout, "\tdvips -Ppdf  -t landscape %s_topology.dvi\n", hostname);
    marcel_fprintf(stdout, "\tps2pdf %s_topology.ps\n", hostname);
  }

  marcel_end();
  return 0;
}
