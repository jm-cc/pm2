
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

#include <stdio.h>
#include <stdlib.h>
#include "tracebuffer.h"
#include "fkt.h"

static int dec;
static int relative;

void print_trace(trace tr, FILE *supertrace)
{
  int i;
  fwrite(&(tr.clock), sizeof(u_64), 1, supertrace);
  fwrite(&(tr.thread), sizeof(int), 1, supertrace);
  fwrite(&(tr.pid), sizeof(int), 1, supertrace);
  fwrite(&(tr.cpu), sizeof(short int), 1, supertrace);
  fwrite(&(tr.type), sizeof(mode), 1, supertrace);
  fwrite(&(tr.number), sizeof(unsigned int), 1, supertrace);
  fwrite(&(tr.code), sizeof(int), 1, supertrace);
  if ((tr.type == USER) || (tr.code > FKT_UNSHIFTED_LIMIT_CODE)) {
    for(i = 0; i < ((tr.code & 0xff) - 12) / 4; i++) {
      fwrite(&(tr.args[i]), sizeof(int), 1, supertrace);
    }
  }
}

void error_usage()
{
  fprintf(stderr,"Usage: supertrace [options]\n");
  fprintf(stderr,"Traite et fusionne les traces utilisateur et noyau.\n");
  fprintf(stderr,"-u, --user-trace    fut_nom\n");
  fprintf(stderr,"-k, --kernel-trace  fkt_nom\n");
  fprintf(stderr,"-o supertrace_nom   par défault prof_file\n");
  fprintf(stderr,"--no-rel-time       Empêche la renormalisation des dates\n");
  fprintf(stderr,"--dec-time          Prend en considération le temps mis par l'écriture d'une trace\n");
  exit(1);
}

int main(int argc, char **argv)
{
  trace tr;
  int i = 0;
  char *supertrace_name = NULL;
  char *fut_name = NULL;
  char *fkt_name = NULL;
  int argCount;
  FILE *supertrace;
  dec = 0;
  relative = 1;
  for(argc--, argv++; argc > 0; argc -= argCount, argv +=argCount) {
    argCount = 1;
    if ((!strcmp(*argv, "--user-trace")) || (!strcmp(*argv, "-u"))) {
      if (argc <= 1) error_usage();
      if (fut_name != NULL) error_usage();
      argCount = 2;
      fut_name = *(argv + 1);
    } else if ((!strcmp(*argv, "--kernel-trace")) || (!strcmp(*argv, "-k"))) {
      if (argc <= 1) error_usage();
      if (fkt_name != NULL) error_usage();
      argCount = 2;
      fkt_name = *(argv + 1);
    } else if (!strcmp(*argv, "-o")) {
      if (argc <= 1) error_usage();
      if (supertrace_name != NULL) error_usage();
      argCount = 2;
      supertrace_name = *(argv + 1);
    } else if (!strcmp(*argv, "--no-relative-time")) {
      relative = 0;
    } else if (!strcmp(*argv, "--dec-time")) {
      dec = 1;
    } else error_usage();
  }
  if (supertrace_name == NULL) supertrace_name = "prof_file";
  if ((supertrace = fopen(supertrace_name, "w")) == NULL) {
    fprintf(stderr,"Unable to open %s", supertrace_name);
    exit(1);
  }
  init_trace_buffer(fut_name, fkt_name, relative, dec);
  while(i == 0) {
    i = get_next_trace(&tr);
    print_trace(tr, supertrace);
  }
  fclose(supertrace);
  return 0;
}
