/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#include <madeleine.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define ESSAIS  2

#define MODE            MAD_IN_HEADER
#define MAX_MESSAGE     (4*1024*1024)

#define min(a, b) ((a) < (b) ? (a) : (b))

static Tick t1, t2;
static int les_modules[2], nb_modules, moi, autre;
static unsigned nb;
static FILE *file, *file_deb;
static char static_buffer[MAX_MESSAGE] __MAD_ALIGNED__;

void f(int bytes)
{
  unsigned long temps;
  unsigned n, i;

  if(bytes % 1024 == 0)
    printf("%d Koctets transférés :\n", bytes/1024);
  else
    printf("%d octets transférés :\n", bytes);

    temps = ~0L;
    for(n=0; n<ESSAIS; n++) {

      GET_TICK(t1);
      for(i=nb/2; i; i--) {
	mad_sendbuf_init(autre);
	if(bytes)
	  old_mad_pack_byte(MODE, static_buffer, bytes);
	mad_sendbuf_send();

	mad_receive();
	if(bytes)
	  old_mad_unpack_byte(MODE, static_buffer, bytes);
	mad_recvbuf_receive();
      }
      GET_TICK(t2);
      temps = min(timing_tick2usec(TICK_DIFF(t1, t2)), temps);
    }

    fprintf(stderr, "temps transfert = %ld.%03ldms\n", temps/1000, temps%1000);
    fprintf(file, "%d %ld\n", bytes, temps/1000);
    fprintf(stderr, "debit = %f Mo/s\n",
	    (double)bytes/((double)temps*1e-3));
    fprintf(file_deb, "%d %f\n", bytes, (double)bytes/((double)temps*1e-3));
}

void g(int bytes)
{
  unsigned n, i;

  for(n=0; n<ESSAIS; n++) {
    for(i=nb/2; i; i--) {

      mad_receive();
      if(bytes)
	old_mad_unpack_byte(MODE, static_buffer, bytes);
      mad_recvbuf_receive();

      mad_sendbuf_init(autre);
      if(bytes)
	old_mad_pack_byte(MODE, (char *)static_buffer, bytes);
      mad_sendbuf_send();
    }
  }
}

static void set_data_dir(char *buf, char *suffix)
{
  char *s = getenv("MADELEINE_ROOT");

  if(s)
    sprintf(buf, "%s/examples/data/%s_%s", s, mad_arch_name(), suffix);
  else
    sprintf(buf, "./%s_%s", mad_arch_name(), suffix);
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

     set_data_dir(name, "madeleine");
     file = fopen(name, "w");

     set_data_dir(name, "madeleine_bandwidth");
     file_deb = fopen(name, "w");

     if(argc > 1)
       nb = atoi(argv[1]);
     else {
       printf("Combien de messages ? ");
       scanf("%d", &nb);
     }

     mad_sendbuf_init(autre);
     old_mad_pack_int(MAD_IN_HEADER, &nb, 1);
     mad_sendbuf_send();

     f(16);
     /*
     for(i=0; i<4096; i+=16)
       f(i);
     for(i=4096; i<128*1024; i+=1024)
       f(i);
     for(i=128*1024; i<4*1024*1024; i+=128*1024)
       f(i);
     */
     fclose(file);
     fclose(file_deb);

   } else {
     autre = les_modules[0];

     mad_receive();
     old_mad_unpack_int(MAD_IN_HEADER, &nb, 1);
     mad_recvbuf_receive();

     fprintf(stderr, "nb de messages = %d\n", nb);

     g(16);
     /*
     for(i=0; i<4096; i+=16)
       g(i);
     for(i=4096; i<128*1024; i+=1024)
       g(i);
     for(i=128*1024; i<4*1024*1024; i+=128*1024)
       g(i);
     */
   }

   mad_exit();

   return 0;
}
