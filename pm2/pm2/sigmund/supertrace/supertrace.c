
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
#include "../../../linux/include/linux/fkt.h"

void print_trace(trace tr, FILE *supertrace)
{
  int i;
  fwrite(&(tr.clock), sizeof(u_64), 1, supertrace);
  fwrite(&(tr.thread), sizeof(int), 1, supertrace);
  fwrite(&(tr.pid), sizeof(int), 1, supertrace);
  fwrite(&(tr.proc), sizeof(short int), 1, supertrace);
  fwrite(&(tr.type), sizeof(mode), 1, supertrace);
  fwrite(&(tr.number), sizeof(unsigned int), 1, supertrace);
  fwrite(&(tr.code), sizeof(int), 1, supertrace);
  for(i = 0; i < ((tr.code & 0xff) - 12) / 4; i++) {
    fwrite(&(tr.args[i]), sizeof(int), 1, supertrace);
  }
}

int main(int args, char *argv[])
{
  trace tr;
  int i = 0;
  FILE *supertrace;
  if ((supertrace = fopen("prof_file","w")) == NULL) {
    fprintf(stderr,"Unable to open prof_file");
    exit(1);
  }
  init_trace_buffer("prof_file_single", "trace_file");
  while(i == 0) {
    i = get_next_trace(&tr);
    print_trace(tr, supertrace);
  }
  fclose(supertrace);
  return 0;
}
