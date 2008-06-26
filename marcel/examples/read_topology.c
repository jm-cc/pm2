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

void print_level_description(struct marcel_topo_level *l, FILE *output, int txt_mode, int verbose_mode)
{
  unsigned long type = l->merged_type;
  const char * separator = " + ";
  const char * current_separator = ""; /* not prefix for the first one */

  if (!verbose_mode) {
    /* don't print "vp" if there's something else (including "fake") */
    if (type & ~(1<<MARCEL_LEVEL_VP))
      type &= ~(1<<MARCEL_LEVEL_VP);
    /* don't print "fake" if there's something else */
    if (type & ~(1<<MARCEL_LEVEL_FAKE))
      type &= ~(1<<MARCEL_LEVEL_FAKE);
    /* don't print smtproc or caches if there's also core or die */
    if (type & ((1<<MARCEL_LEVEL_CORE) | (1<<MARCEL_LEVEL_DIE)))
      type &= ~( (1<<MARCEL_LEVEL_PROC) | (1<<MARCEL_LEVEL_L2) | (1<<MARCEL_LEVEL_L3) );
  }

  if (type & (1<<MARCEL_LEVEL_MACHINE)) {
    marcel_fprintf(output, "%sMachine", current_separator);
    current_separator = separator;
  }
#ifdef MA__LWPS
  if (type & (1<<MARCEL_LEVEL_FAKE)) {
    marcel_fprintf(output, "%sFake", current_separator);
    current_separator = separator;
  }
#  ifdef MA__NUMA
  if (type & (1<<MARCEL_LEVEL_NODE)) {
    marcel_fprintf(output, "%sNUMAnode", current_separator);
    current_separator = separator;
  }
  if (type & (1<<MARCEL_LEVEL_DIE)) {
    marcel_fprintf(output, "%sDie", current_separator);
    current_separator = separator;
  }
  if (type & (1<<MARCEL_LEVEL_L3)) {
    marcel_fprintf(output, "%sL3cache", current_separator);
    current_separator = separator;
  }
  if (type & (1<<MARCEL_LEVEL_L2)) {
    marcel_fprintf(output, "%sL2cache", current_separator);
    current_separator = separator;
  }
  if (type & (1<<MARCEL_LEVEL_CORE)) {
    marcel_fprintf(output, "%sCore", current_separator);
    current_separator = separator;
  }
  if (type & (1<<MARCEL_LEVEL_PROC)) {
    marcel_fprintf(output, "%sSMTprocessor", current_separator);
    current_separator = separator;
  }
#  endif
  if (type & (1 << MARCEL_LEVEL_VP)) {
    marcel_fprintf(output, "%sVP", current_separator);
    current_separator = separator;
  }
#endif
}

void indent(FILE *output, int i) {
  int x;
  for(x=0 ; x<i ; x++) marcel_fprintf(output, "  ");
}

void print_level(struct marcel_topo_level *l, FILE *output, int txt_mode, int verbose_mode) {
  const char * separator = txt_mode ? " " : "\\\\";
  const char * indexprefix = txt_mode ? "#" : "";
  const char * labelseparator = txt_mode ? ":" : "";

  if (!txt_mode) {
    if (l->arity) {
      marcel_fprintf(output, "\\pstree");
    }
    marcel_fprintf(output, "{\\Level{c}{");
  }
  print_level_description(l, output, txt_mode, verbose_mode);
  marcel_fprintf(output, labelseparator);
  if (l->os_node != -1) marcel_fprintf(output, "%sNode %s%u", separator, indexprefix, l->os_node);
  if (l->os_die != -1)  marcel_fprintf(output, "%sDie %s%u" , separator, indexprefix, l->os_die);
  if (l->os_l3 != -1)   marcel_fprintf(output, "%sL3 %s%u"  , separator, indexprefix, l->os_l3);
  if (l->os_l2 != -1)   marcel_fprintf(output, "%sL2 %s%u"  , separator, indexprefix, l->os_l2);
  if (l->os_core != -1) marcel_fprintf(output, "%sCore %s%u", separator, indexprefix, l->os_core);
  if (l->os_cpu != -1)  marcel_fprintf(output, "%sCPU %s%u" , separator, indexprefix, l->os_cpu);

  if (l->level == marcel_topo_nblevels-1) {
    marcel_fprintf(output, "%sVP %s%u", separator, indexprefix, l->number);
  }
  if (txt_mode) {
    marcel_fprintf(output,"\n");
  } else {
    marcel_fprintf(output,"}}\n");
  }
}

void f(struct marcel_topo_level *l, FILE *output, int i, int txt_mode, int verbose_mode) {
  int x;

  indent(output, i);
  print_level(l, output, txt_mode, verbose_mode);
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
  int txt_mode = 0;
  int verbose_mode = 0;

  marcel_init(&argc, argv);

  while (argc >= 2) {
    if (!strcmp(argv[1], "-t") || !strcmp(argv[1], "--txt")) {
      txt_mode = 1;
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

  l = &marcel_topo_levels[0][0];
  f(l, output, 0, txt_mode, verbose_mode);

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
