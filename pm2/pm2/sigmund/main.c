
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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
#include "sigmund.h"
#include "parser.h"
#include <stdlib.h>
#include "time.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define FUT_SWITCH_TO_CODE			0x230

extern void init(void);

char *code2name(int code)
{
  int i = 0;
  while(1) {
    if (code_table[i].code == 0) return NULL;
    if (code_table[i].code == code) return code_table[i].name;
    i++;
  }
}

int main()
{
  FILE *fin;
  u_64 a;
  u_64 olda;
  int c;
  int thread = 0;
  char header[200];
  init();
  if ((fin = fopen("prof_file_single","r")) == NULL) {
    fprintf(stderr,"Erreur dans l'ouverture du fichier\n");
    exit(1);
  } 
  fread(header, sizeof(unsigned long) + sizeof(double) + 2*sizeof(time_t) + sizeof(unsigned int), 1, fin);
  fread(&a, sizeof(u_64), 1, fin);
  olda = a;
  while(!feof(fin)) {
    int i;
    u_64 r;
    r = a - olda;
    printf("%u\t%u ", thread, (unsigned) r);
    fread(&c, sizeof(int), 1, fin);
    i = c & 0xff;
    printf("\t0x%x",c >> 8);
    i -= 12;
    i = i / 4;
    printf("\t%d ",i);
    printf("\t%s", code2name(c >> 8)); 
    if ((c >> 8) == FUT_SWITCH_TO_CODE) {
      fread(&c, sizeof(int), 1, fin);
      printf("\t%d",c);
      i--;
      thread = c;
    }
    while (i != 0) {
      fread(&c, sizeof(int), 1, fin);
      printf("\t%x",c);
      i--;
    }
    printf("\n");
    olda = a;
    fread(&a, sizeof(u_64), 1, fin);
  }
  fclose(fin);
  return 0;
}

