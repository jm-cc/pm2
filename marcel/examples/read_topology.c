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

#ifdef MA__NUMA
#define EPOXY_COLOR 32
#define MEMORY_COLOR 34
#define DIE_COLOR 33
#define CACHE_COLOR 7
#define CORE_COLOR 35
#define THREAD_COLOR 7

/* grid unit: 1/6 inch */
#define UNIT 200
#define FONT_SIZE 12

void fig_box(FILE *output, int color, unsigned depth, unsigned x, unsigned width, unsigned y, unsigned height) {
  marcel_fprintf(output, "2 2 0 1 0 %d %u -1 20 0.0 0 0 -1 0 0 5\n\t", color, depth);
  marcel_fprintf(output, " %u %u", x, y);
  marcel_fprintf(output, " %u %u", x + width, y);
  marcel_fprintf(output, " %u %u", x + width, y + height);
  marcel_fprintf(output, " %u %u", x, y + height);
  marcel_fprintf(output, " %u %u", x, y);
  marcel_fprintf(output, "\n");
}

void fig_text(FILE *output, int color, int size, unsigned depth, unsigned x, unsigned y, const char *text) {
  marcel_fprintf(output, "4 0 %d %u -1 0 %u 0.0 4 %u %u %u %u %s\\001\n", color, depth, size, size * 10, (unsigned) strlen(text) * size * 10, x, y + size * 10, text);
}

void get_sublevels(struct marcel_topo_level **levels, unsigned numlevels, struct marcel_topo_level ***sublevels, unsigned *numsublevels) {
  unsigned alloced = numlevels * levels[0]->arity;
  struct marcel_topo_level **l = tmalloc(alloced * sizeof(*l));
  unsigned i, j, n;

  n = 0;
  for (i = 0; i < numlevels; i++) {
    for (j = 0; j < levels[i]->arity; j++) {
      if (n >= alloced) {
	alloced *= 2;
	l = trealloc(l, alloced * sizeof(*l));
      }
      l[n++] = levels[i]->children[j];
    }
  }
  *sublevels = l;
  *numsublevels = n;
}

int get_sublevels_type(struct marcel_topo_level *level, enum marcel_topo_level_e type, struct marcel_topo_level ***sublevels, unsigned *numsublevels) {
  unsigned n = 1, n2;
  struct marcel_topo_level **l = tmalloc(sizeof(*l)), **l2;
  l[0] = level;

  while(1) {
    if (l[0]->merged_type & (1 << type)) {
      *sublevels = l;
      *numsublevels = n;
      return 1;
    }
    if (l[0]->arity <= 1) {
      tfree(l);
      return 0;
    }
    get_sublevels(l, n, &l2, &n2);
    free(l);
    l = l2;
    n = n2;
  }
}

#define IF_RECURSE(type) { \
  struct marcel_topo_level **sublevels; \
  unsigned numsublevels; \
  unsigned width, height; \
  if (get_sublevels_type(level, type, &sublevels, &numsublevels)) { \
    int i;

#define LAST_RECURSE(next, sep) \
    for (i = 0; i < numsublevels; i++) { \
      next(sublevels[i], output, depth-1, x + totwidth, &width, y + myheight, &height); \
      totwidth += width + sep; \
      if (height > maxheight) \
	maxheight = height; \
    } \
    totwidth -= sep;

#define ENDIF_RECURSE(next) \
    tfree(sublevels); \
  } else { \
    next(level, output, depth-1, x + totwidth, &width, y + myheight, &height); \
    maxheight = height; \
    totwidth += width; \
  } \
}

#define RECURSE(type, next, sep) \
  IF_RECURSE(type) \
  LAST_RECURSE(next, sep) \
  ENDIF_RECURSE(next)

void vp_fig(struct marcel_topo_level *level, FILE *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight) {
  unsigned myheight = 0;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  if (level->merged_type & (1 << MARCEL_LEVEL_VP)) {
    snprintf(text, sizeof(text), "VP%u", level->number);
    fig_text(output, 0, FONT_SIZE, depth-1, x, y + myheight, text);
    myheight += FONT_SIZE*10;
  }

  totwidth = 2*UNIT;

  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;
}

void proc_fig(struct marcel_topo_level *level, FILE *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight) {
  unsigned myheight = 0;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  myheight += UNIT;
  snprintf(text, sizeof(text), "CPU%u", level->os_cpu);
  fig_text(output, 0, FONT_SIZE, depth-1, x + UNIT, y + myheight, text);
  myheight += FONT_SIZE*10 + UNIT;

#if 0
  RECURSE(MARCEL_LEVEL_VP, vp_fig, 0);
  maxheight += UNIT;
#else
  totwidth = 3*UNIT;
#endif

  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;
  fig_box(output, THREAD_COLOR, depth, x, *retwidth, y, *retheight);
}

void l1_fig(struct marcel_topo_level *level, FILE *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight) {
  unsigned myheight = 0;
  unsigned totwidth = 0, maxheight = 0;
  char text[64];

  if (level->merged_type & (1 << MARCEL_LEVEL_L1)) {
    myheight += UNIT;
    snprintf(text, sizeof(text), "L1 %u - %ldKB", level->os_l1, level->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L1]);
    fig_text(output, 0, FONT_SIZE, depth-1, x + UNIT, y + myheight, text);
    myheight += FONT_SIZE*10 + UNIT + UNIT;
  }

  RECURSE(MARCEL_LEVEL_PROC, proc_fig, 0);

  if (level->merged_type & (1 << MARCEL_LEVEL_L1)) {
    if (totwidth < 7*UNIT)
      totwidth = 7*UNIT;
  }
  *retwidth = totwidth;
  *retheight = myheight + maxheight;
  if (level->merged_type & (1 << MARCEL_LEVEL_L1)) {
    if (!maxheight)
      *retheight -= UNIT;
    fig_box(output, CACHE_COLOR, depth, x, *retwidth, y, myheight - UNIT);
  }
}

void core_fig(struct marcel_topo_level *level, FILE *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight) {
  unsigned myheight = UNIT;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  if (level->merged_type & (1 << MARCEL_LEVEL_CORE)) {
    snprintf(text, sizeof(text), "Core %u", level->os_core);
    fig_text(output, 0, FONT_SIZE, depth-1, x + UNIT, y + myheight, text);
    myheight += FONT_SIZE*10 + UNIT;
  }

  RECURSE(MARCEL_LEVEL_L1, l1_fig, 0);

  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight + (maxheight?UNIT:0);
  fig_box(output, CORE_COLOR, depth, x, *retwidth, y, *retheight);
}

void l2_fig(struct marcel_topo_level *level, FILE *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight) {
  unsigned myheight = 0;
  unsigned totwidth = 0, maxheight = 0;
  char text[64];

  if (level->merged_type & (1 << MARCEL_LEVEL_L2)) {
    myheight += UNIT;
    snprintf(text, sizeof(text), "L2 %u - %ldMB", level->os_l2, level->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L2] >> 10);
    fig_text(output, 0, FONT_SIZE, depth-1, x + UNIT, y + myheight, text);
    myheight += FONT_SIZE*10 + UNIT + UNIT;
  }

  RECURSE(MARCEL_LEVEL_CORE, core_fig, 0);

  *retwidth = totwidth;
  *retheight = myheight + maxheight;
  if (level->merged_type & (1 << MARCEL_LEVEL_L2)) {
    fig_box(output, CACHE_COLOR, depth, x, *retwidth, y, myheight - UNIT);
  }
}

void l3_fig(struct marcel_topo_level *level, FILE *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight) {
  unsigned myheight = 0;
  unsigned totwidth = 0, maxheight = 0;
  char text[64];

  if (level->merged_type & (1 << MARCEL_LEVEL_L3)) {
    myheight += UNIT;
    snprintf(text, sizeof(text), "L3 %u - %ldMB", level->os_l3, level->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_L3] >> 10);
    fig_text(output, 0, FONT_SIZE, depth-1, x + UNIT, y + myheight, text);
    myheight += FONT_SIZE*10 + UNIT + UNIT;
  }

  RECURSE(MARCEL_LEVEL_L2, l2_fig, UNIT);

  *retwidth = totwidth;
  *retheight = myheight + maxheight;
  if (level->merged_type & (1 << MARCEL_LEVEL_L3)) {
    fig_box(output, CACHE_COLOR, depth, x, *retwidth, y, myheight - UNIT);
  }
}

void die_fig(struct marcel_topo_level *level, FILE *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight) {
  unsigned myheight = UNIT;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  if (level->merged_type & (1 << MARCEL_LEVEL_DIE)) {
    snprintf(text, sizeof(text), "Die %u", level->os_die);
    fig_text(output, 0, FONT_SIZE, depth-1, x + UNIT, y + myheight, text);
    myheight += FONT_SIZE*10 + UNIT;
  }

  RECURSE(MARCEL_LEVEL_L3, l3_fig, UNIT);
  maxheight += UNIT;

  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;
  fig_box(output, DIE_COLOR, depth, x, *retwidth, y, *retheight);
}

void node_fig(struct marcel_topo_level *level, FILE *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight) {
  unsigned myheight = UNIT;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  if (level->merged_type & (1 << MARCEL_LEVEL_NODE)) {
    myheight += UNIT;
    snprintf(text, sizeof(text), "Node %u - %luGB", level->os_node, level->memory_kB[MARCEL_TOPO_LEVEL_MEMORY_NODE] >> 20);
    fig_text(output, 0, FONT_SIZE, depth-2, x + 2 * UNIT, y + myheight, text);
    myheight += FONT_SIZE*10 + 2*UNIT;
  }

  RECURSE(MARCEL_LEVEL_DIE, die_fig, UNIT);
  maxheight += UNIT;

  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;
  if (level->merged_type & (1 << MARCEL_LEVEL_NODE)) {
    fig_box(output, MEMORY_COLOR, depth-1, x + UNIT, *retwidth - 2 * UNIT, y + UNIT, myheight - 2 * UNIT);
  }
  fig_box(output, EPOXY_COLOR, depth, x, *retwidth, y, *retheight);
}

void fig(struct marcel_topo_level *level, FILE *output, unsigned depth, unsigned x, unsigned y) {
  unsigned myheight = 0;
  unsigned totwidth = 0, maxheight = 0;

  IF_RECURSE(MARCEL_LEVEL_NODE)
    myheight = 2*UNIT + FONT_SIZE*10 + UNIT;
  LAST_RECURSE(node_fig, UNIT)
    fig_text(output, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, "Interconnect");
    fig_box(output, EPOXY_COLOR, depth, x, totwidth, y, 2*UNIT + FONT_SIZE*10);
  ENDIF_RECURSE(node_fig);
}
#endif

#define indent(output, i) \
  marcel_fprintf(output, "%*s", 2*i, "");

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
  char hostname[256], filename[256];
  FILE *output;
  int txt_mode = 1, fig_mode = 0;
  int verbose_mode = 0;
  const char *suffix = "tex";

  marcel_init(&argc, argv);

  while (argc >= 2) {
    if (!strcmp(argv[1], "-l") || !strcmp(argv[1], "--latex")) {
      txt_mode = 0;
#ifdef MA__NUMA
    } else if (!strcmp(argv[1], "-f") || !strcmp(argv[1], "--fig")) {
      txt_mode = 0;
      fig_mode = 1;
      suffix = "fig";
#endif
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
    gethostname(hostname, sizeof(hostname)-1);
    hostname[sizeof(hostname)-1] = 0;
    marcel_snprintf(filename, sizeof(filename), "%s_topology.%s", hostname, suffix);
    output = marcel_fopen(filename, "w");

    if (fig_mode) {
      marcel_fprintf(output, "#FIG 3.2  Produced by PM2's read_topology\n");
      marcel_fprintf(output, "Landscape\n");
      marcel_fprintf(output, "Center\n");
      marcel_fprintf(output, "Inches\n");
      marcel_fprintf(output, "letter\n");
      marcel_fprintf(output, "100.00\n");	/* magnification */
      marcel_fprintf(output, "Single\n");	/* single page */
      marcel_fprintf(output, "-2\n");		/* no transparent color */
      marcel_fprintf(output, "1200 2\n");	/* 1200 ppi resolution, upper left origin */
      marcel_fprintf(output, "0 32 #e7ffb5\n");
      marcel_fprintf(output, "0 33 #dedede\n");
      marcel_fprintf(output, "0 34 #efdfde\n");
      marcel_fprintf(output, "0 35 #bebebe\n");
    } else {
      marcel_fprintf(output, "\\documentclass[landscape,a4paper,10pt]{article}\n");
      marcel_fprintf(output, "\\usepackage{fullpage}\n");
      marcel_fprintf(output, "\\usepackage{pst-tree}\n");
      marcel_fprintf(output, "%% Macro to define one level\n");
      marcel_fprintf(output, "\\newcommand{\\Level}[3][]{\\TR[#1]{\\setlength{\\tabcolsep}{0mm}\\begin{tabular}[t]{#2}#3\\end{tabular}}}\n\n");
      marcel_fprintf(output, "\\begin{document}\n");
      marcel_fprintf(output, "\\title{%s: Topology}\n", hostname);
      marcel_fprintf(output, "\\author{Marcel}\n");
      marcel_fprintf(output, "\\maketitle\n");
      marcel_fprintf(output, "\\tiny\n");
    }
  }

#ifdef MA__NUMA
  if (fig_mode)
    fig(marcel_topo_level(0,0), output, 100, 0, 0);
  else
#endif
    f(marcel_topo_level(0, 0), output, 0, txt_mode, verbose_mode);

  if (!txt_mode) {
    if (!fig_mode)
      marcel_fprintf(output, "\\end{document}\n");
    marcel_fclose(output);

    marcel_fprintf(stdout, "The file %s has been created.\n", filename);
    if (!fig_mode) {
      marcel_fprintf(stdout, "Compile it as follows:\n");
      marcel_fprintf(stdout, "\tlatex %s_topology\n", hostname);
      marcel_fprintf(stdout, "\tdvips -Ppdf  -t landscape %s_topology.dvi\n", hostname);
      marcel_fprintf(stdout, "\tps2pdf %s_topology.ps\n", hostname);
    }
  }

#ifdef MA__NUMA
  if (verbose_mode && txt_mode) {
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
