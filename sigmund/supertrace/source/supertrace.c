
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
#include <string.h>
#include "supertracebuffer.h"
#include "fkt.h"

static int dec;
static int relative;

/* write one event into the supertrace. Its format is this:
      u_64           time in ticks of the event
      int            number of thread (-1 if none [Kernel])
      int            number of pid which runs this thread
      short int      number of cpu on which the cpu turns at this date
      mode           type of the event
                     (Kernel or User: usefull for the decoding of codes'events)
      unsigned int   number of this event (first one gets number 0)
      int            code of the event (same as in FUT and FKT)
      int[]          arguments as in FUT and FKT
*/
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
    for(i = 0; i < tr.nbargs; i++) {
      fwrite(&(tr.args[i]), sizeof(int), 1, supertrace);
    }
  }
}

/* message if a wrong command is given */
void error_usage()
{
  fprintf(stderr,"Usage: supertrace [options]\n");
  fprintf(stderr,"Traite et fusionne les traces utilisateur et noyau.\n");
  fprintf(stderr,"-u, --user-trace    fut_nom\n");
  fprintf(stderr,"-k, --kernel-trace  fkt_nom\n");
  fprintf(stderr,"-o supertrace_nom   par défault prof_file\n");
  fprintf(stderr,"--no-rel-time       empêche la renormalisation des dates\n");
  fprintf(stderr,"--dec-time          prend en considération le temps mis par l'écriture d'une trace\n");
  fprintf(stderr,"--all               considère toute la trace noyau\n");
  exit(1);
}

/* initialisation, generation of the supertrace */
int main(int argc, char **argv)
{
  trace tr;
  int i = 0;
  int all = 0;                   // 1 if all kernel information is to be writen
  char *supertrace_name = NULL;  /* supertrace file name, 
				    NULL -> default = prof_file */
  char *fut_name = NULL;         /* name of fut trace file, 
				    NULL -> no fut given */
  char *fkt_name = NULL;         /* name if fkt trace file, 
				    NULL -> no fkt given */
  int argCount;
  FILE *supertrace;              // file where to record the supertrace
  dec = 0;                       // 0 if no shifting of date is asked
  relative = 1;                  // If 0 don't put the first event to time 0
  
  // Parse the arguments
  
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
    } else if (!strcmp(*argv, "--no-rel-time")) {
      relative = 0;
    } else if (!strcmp(*argv, "--dec-time")) {
      dec = 1;
    } else if (!strcmp(*argv, "--all")) {
      all = 1;
    } else error_usage();
  }
  // Default for supertrace is prof_file
  if (supertrace_name == NULL) supertrace_name = "prof_file";   
  if ((supertrace = fopen(supertrace_name, "w")) == NULL) {
    fprintf(stderr,"Unable to open %s", supertrace_name);
    exit(1);
  }    

  // Leave room for the header
  fseek(supertrace, 500, SEEK_SET);

  // Initialise the buffer for the events
  init_trace_buffer(fut_name, fkt_name, relative, dec);          

  while(i == 0) {                         // While there are still events
    i = get_next_trace(&tr);              // Get the next one
    if ((tr.relevant != 0) || (all))      // reduces if needed the kernel trace
      print_trace(tr, supertrace);
  }
  rewind(supertrace);                     //
  supertrace_end(supertrace);             //  Writes the supertrace header
  fclose(supertrace);
  return 0;
}
