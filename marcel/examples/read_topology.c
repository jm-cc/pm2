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
#include "marcel.h"
#include "marcel_topology.h"
#include <stdio.h>
#include <stdlib.h>

void indent(FILE *output, int i) {
  int x;
  for(x=0 ; x<i ; x++) marcel_fprintf(output, "  ");
}

void f(struct marcel_topo_level *l, FILE *output, int i, int txt_mode, int verbose_mode) {
  int x;
  const char * separator = txt_mode ? " " : "\\\\";
  const char * indexprefix = txt_mode ? "#" : "\\#";
  const char * labelseparator = txt_mode ? ":" : "";
  const char * levelterm = txt_mode ? "" : "}}";

  indent(output, i);
  if (!txt_mode) {
    if (l->arity) {
      marcel_fprintf(output, "\\pstree");
    }
    marcel_fprintf(output, "{\\Level{c}{");
  }
  marcel_print_level(l, output, txt_mode, verbose_mode, separator, indexprefix, labelseparator, levelterm);
  if (l->arity || (!i && !l->arity)) {
    if (!txt_mode) {
      indent(output, i);
      marcel_fprintf(output, "{\n");
    }
    for(x=0; x<l->arity; x++)
      f(l->children[x], output, i+1, txt_mode, verbose_mode);
    if (!txt_mode) {
      indent(output, i);
      marcel_fprintf(output, "}\n");
    }
  }
}

int marcel_main(int argc, char **argv) {
  struct marcel_topo_level *l;
  char hostname[256], filename[256];
  FILE *output;
  int txt_mode = 1;
  int verbose_mode = 0;

  marcel_init(&argc, argv);

  while (argc >= 2) {
    if (!strcmp(argv[1], "-l") || !strcmp(argv[1], "--latex")) {
      txt_mode = 0;
    } else if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--verbose")) {
      verbose_mode = 1;
    } else {
      marcel_fprintf(stderr, "Unrecognized options: %s\n", argv[1]);
    }
    argc--;
    argv++;
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

  l = marcel_topo_level(0, 0);
  f(l, output, 0, txt_mode, verbose_mode);

  if (!txt_mode) {
    marcel_fprintf(output, "\\end{document}\n");
    marcel_fclose(output);

    marcel_fprintf(stdout, "The file %s_topology.tex has been created. Compile it as follows:\n", hostname);
    marcel_fprintf(stdout, "\tlatex %s_topology\n", hostname);
    marcel_fprintf(stdout, "\tdvips -Ppdf  -t landscape %s_topology.dvi\n", hostname);
    marcel_fprintf(stdout, "\tps2pdf %s_topology.ps\n", hostname);
  }
  

  if (verbose_mode) {
    int i;
    for (i = 0; i <= MARCEL_LEVEL_LAST; i++)
      marcel_fprintf(output, "marcel type %d on lvl %d.\n", i, ma_get_topo_type_level (i));
  }

  marcel_end();
  return 0;
}
