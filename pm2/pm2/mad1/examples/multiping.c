
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

#include "madeleine.h"

#include <stdio.h>
#include <stdlib.h>

#define ESSAIS          3
#define MODE            MAD_IN_PLACE
#define MAX_VECS        8
#define MAX_MESSAGE     (64*1024)

#define min(a, b) ((a) < (b) ? (a) : (b))


static int les_modules[2], nb_modules, moi, autre;
static unsigned nb;
static unsigned nb_vecs;
static FILE *file;
static char static_buffer[MAX_VECS][MAX_MESSAGE] __MAD_ALIGNED__;
static Tick t1, t2;

void f(int bytes)
{
  unsigned long temps;
  unsigned n, i, j;

  if(bytes % 1024 == 0)
    printf("%d Koctets transférés :\n", bytes/1024);
  else
    printf("%d octets transférés :\n", bytes);

  temps = ~0L;
  for(n=0; n<ESSAIS; n++) {
    GET_TICK(t1);
    for(i=nb/2; i; i--) {
      mad_sendbuf_init(autre);
      for(j=0; j<nb_vecs; j++)
	old_mad_pack_byte(MODE, static_buffer[j], bytes / nb_vecs);
      mad_sendbuf_send();
      
      mad_receive();
      for(j=0; j<nb_vecs; j++)
	old_mad_unpack_byte(MODE, static_buffer[j], bytes / nb_vecs);
      mad_recvbuf_receive();
    }
    GET_TICK(t2);
    temps = min(timing_tick2usec(TICK_DIFF(t1, t2)), temps);
  }

  fprintf(stderr, "temps transfert = %ld.%03ldms\n", temps/1000, temps%1000);
  fprintf(file, "%d %ld\n", bytes, temps/1000);
}

void g(int bytes)
{
  unsigned n, i, j;

  for(n=0; n<ESSAIS; n++) {
    for(i=nb/2; i; i--) {
      mad_receive();
      for(j=0; j<nb_vecs; j++)
	old_mad_unpack_byte(MODE, static_buffer[j], bytes / nb_vecs);
      mad_recvbuf_receive();

      mad_sendbuf_init(autre);
      for(j=0; j<nb_vecs; j++)
	old_mad_pack_byte(MODE, (char *)static_buffer[j], bytes / nb_vecs);
      mad_sendbuf_send();
    }
  }
}

static void set_data_dir(char *buf, char *suffix)
{
  char *s = getenv("MAD1_ROOT");

  if(s)
    sprintf(buf, "%s/examples/data/%s_%s_%d", s, mad_arch_name(), suffix, nb_vecs);
  else
    sprintf(buf, "./%s_%s_%d", mad_arch_name(), suffix, nb_vecs);
}

int main(int argc, char **argv)
{
  int i;
  char name[1024];

  mad_init(&argc, argv, 2, les_modules, &nb_modules, &moi);
  mad_buffers_init();

  if(moi == les_modules[0]) {

    autre = les_modules[1];

    printf("*** Performances de madeleine au-dessus de %s ***\n", mad_arch_name());

    if(argc > 1) {
      if (argc != 3) {
	fprintf(stderr, "Usage : %s <taille_msg> <nb_morceaux>\n", argv[0]);
	exit(1);
      } else {
	nb = atoi(argv[1]);
	nb_vecs = atoi(argv[2]);
      }
    } else {
      printf("Combien de messages ? ");
      scanf("%d", &nb);
      printf("En combien de morceaux ? ");
      scanf("%d", &nb_vecs);
    }

    set_data_dir(name, "madping");
    file = fopen(name, "w");

    mad_sendbuf_init(autre);
    old_mad_pack_int(MAD_IN_HEADER, &nb, 1);
    old_mad_pack_int(MAD_IN_HEADER, &nb_vecs, 1);
    mad_sendbuf_send();

    for(i=0; i<30; i++)
      f(i*100);

    for(i=4; i<=16; i++)
      f(i*1024);

    fclose(file);

  } else {

    autre = les_modules[0];

    mad_receive();
    old_mad_unpack_int(MAD_IN_HEADER, &nb, 1);
    old_mad_unpack_int(MAD_IN_HEADER, &nb_vecs, 1);
    mad_recvbuf_receive();

    mdebug("nb de messages = %d\n", nb);

    for(i=0; i<30; i++)
      g(i*100);

    for(i=4; i<=16; i++)
      g(i*1024);

  }

  mad_exit();

  return 0;
}
