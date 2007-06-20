/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(MAD_MPI)
#  define MPI_Esend(a, b, c, d, e, f, g, h) MPI_Isend(a, b, c, d, e, g, h)
#endif

#define LOOPS 1000

void send_array(int comm_rank, int *sizes) {
  char *buffer = NULL;
  int   total_size = 0;
  int   nb_messages = 0;
  int   i, j, dest;
  int   max_size=0;
  MPI_Request *requests;

  {
    int *size = sizes;
    while (*size != -1) {
      total_size += *size;
      if (*size > max_size) max_size = *size;
      nb_messages ++;
      size++;
    }
  }

  buffer = malloc(max_size);
  memset(buffer, 0, max_size);
  requests = malloc(nb_messages * sizeof(MPI_Request));

  if (comm_rank == 0) {
    double start, stop;
    dest = 1;
    start = MPI_Wtime();
    for(i=0;i<LOOPS;i++) {
      // ping
      for(j=0 ; j<nb_messages ; j++) {
        MPI_Isend(buffer, sizes[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, &requests[j]);
      }
      for(j=0 ; j<nb_messages ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
      // pong
      for(j=0 ; j<nb_messages ; j++) {
        MPI_Irecv(buffer, sizes[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, &requests[j]);
      }
      for(j=0 ; j<nb_messages ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
    }
    stop = MPI_Wtime();
    fprintf(stdout, "Temps = %f, Taille totale = %d, Nb de messages = %d\n", (stop-start)/LOOPS/2, total_size, nb_messages);
  }
  else {
    dest = 0;
    for(i=0;i<LOOPS;i++) {
      // pong
      for(j=0 ; j<nb_messages ; j++) {
        MPI_Irecv(buffer, sizes[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, &requests[j]);
      }
      for(j=0 ; j<nb_messages ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
      // ping
      for(j=0 ; j<nb_messages ; j++) {
        MPI_Isend(buffer, sizes[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, &requests[j]);
      }
      for(j=0 ; j<nb_messages ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
    }
  }
  free(requests);
  free(buffer);
}

int sizes_pastix[]  = {112440, 144528, 29000, 398056, 638264, 384168, -1};
int sizes_nmad[]    = {
  20040,  19640,  4040,   1240,   440,   1640,   1240,  3640,  2040,   440,   // 54400
  440,    840,    1640,   840,    440,   1240,   440,   440,   440,    440,   //  7200
  440,    440,    1240,   840,    840,   440,    440,   440,   1240,   1240,  //  7600
  440,    840,    1240,   440,    1240,  440,    440,   440,   840,    440,   //  6800
  840,    440,    440,    440,    440,   840,    440,   440,   440,    440,   //  5200
  440,    440,    1240,   440,    840,   1240,   840,   440,   840,    840,   //  7600
  840,    440,    440,    440,    48,    64,     440,   440,   80,     440,   //  3672
  440,    440,    1240,   1240,   1240,  2440,   840,   840,   440,    1240,  // 10400
  48,     440,    440,    440,    1240,  840,    440,   440,   440,    440,   //  5208
  440,    840,    440,    440,    440,   440,    440,   440,   440,           //  4360

  19248,  3960,   17680,  29048,  18464,  432,   432,   432,  2392,   1608,  //  93696
  432,    432,    1216,   216,    1216,   432,   824,   1216, 432,    1216,  //   7632
  432,    432,    432,    2000,   824,    824,   432,   2000, 2000,   432,   //   9808
  432,    432,    432,    432,    1216,   432,   824,   1216, 824,    824,   //   7064
  824,    824,    824,    432,    432,    432,   432,   432,  432,    432,   //   5496
  432,    1216,   1216,   2392,   56,     2392,  824,   824,  432,    1216,  //  11000
  432,    1216,   48,     1216,   824,    432,   432,   432,  432,    432,   //   5896
  88,     824,    432,    432,    432,    432,   432,   432,  432,           //   3936

  2088,   9720,   3800,   120,    120,    120,   520,   360,   120,   120,   //  17088
  280,    280,    120,    200,    280,    120,   280,   120,   120,   120,   //   1920
  440,    200,    120,    120,    440,    440,   120,   48,    120,   48,    //   2096
  120,    120,    120,    280,    120,    200,   280,   200,   120,   200,   //   1760
  200,    200,    120,    120,    120,    120,   120,   120,   120,   120,   //   1360
  280,    280,    280,    520,    200,    200,   120,   280,   168,   48,    //   2376
  280,    280,    200,    120,    120,    120,   120,   120,   200,   120,   //   1680
  120,    120,    120,    120,    120,    120,                               //    720

  31792,  48424,  8104,   33808,  20704,  63544, 63544, 5080,  58000, 65056, // 398056

  127048, 127048, 127048, 127048, 130072,                                    // 638264

  127048, 127048, 130072,                                                    // 384168

  -1};

int nb_messages[] = {0,      0,      0,     0,      0,      0,      -1};
int bornes_inf[]  = {0,      0,      0,     0,      0,      0,      -1};
int bornes_sup[]  = {0,      0,      0,     0,      0,      0,      -1};

int set_bornes_pastix() {
  int *nmad = sizes_nmad;
  int total = 0;
  int *pastix = sizes_pastix;
  int *sup = bornes_sup;
  int *inf = bornes_inf;
  int *nb = nb_messages;
  int chunks = 0;

  // calcule comment pastix aggrege les messages
  while (*pastix != -1) {
    total =  0;
    *nb = 0;
    while (total != *pastix) {
      total += *nmad;
      nmad++;
      *sup += 1;
      *nb += 1;
    }
    chunks++;
    pastix ++;
    int *ptr = sup+1;
    *ptr = *sup;
    sup++;
    nb++;
  }
  *sup = -1;
  *nb = -1;

  inf += 1;
  sup = bornes_sup;
  while (*sup != -1) {
    *inf = *sup;
    sup ++;
    inf ++;
  }

  return chunks;
}

void process(int comm_rank, int borne_inf, int borne_sup, int size_pastix, int nb_message);

int main(int argc, char	**argv) {
  int comm_rank;
  int comm_size;
  int i, chunks;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  //send_array(comm_rank, sizes_pastix);
  //send_array(comm_rank, sizes_nmad);

  chunks = set_bornes_pastix();

  if (comm_rank == 0) {
    fprintf(stdout, "Start ! Stop ! Total size ! Nb messages ! Default (ms) ! Aggregation (ms) ! Memcopy (ms) ! Extended (ms) ! Benefit aggreg vs memcpy (%%)\n");
  }

  for(i=0 ; i<chunks ; i++) {
    process(comm_rank, bornes_inf[i], bornes_sup[i], sizes_pastix[i], nb_messages[i]);
  }

  MPI_Finalize();
  return 0;
}

void process(int comm_rank, int borne_inf, int borne_sup, int size_pastix, int nb_message) {
  MPI_Request *requests;
  int i, j, k;
  double stop_memcopy=0, start_memcopy=0;
  double stop_aggreg=0, start_aggreg=0;
  double stop_default=0, start_default=0;
  double stop_extended=0, start_extended=0;
  char **buffer;
  int dest;

  requests = malloc(borne_sup * sizeof(MPI_Request));
  buffer = malloc(borne_sup * sizeof(char *));
  for(i=borne_inf ; i<borne_sup ; i++) {
    buffer[i] = malloc(sizes_nmad[i]);
    memset(buffer[i], 0, sizes_nmad[i]);
  }

#define TEST_DEFAULT
#ifdef TEST_DEFAULT
  if (comm_rank == 0) {
    dest = 1;
    start_default = MPI_Wtime();
    for(i=0 ; i<LOOPS ; i++) {
      // ping
      for(j=borne_inf ; j<borne_sup ; j++) {
        // Envoie un message de taille sizes_nmad[j]
        MPI_Send(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD);
      }
      // pong
      for(j=borne_inf ; j<borne_sup ; j++) {
        // Reception pour un message de taille sizes_nmad[j]
        MPI_Recv(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      // Process data
      for(j=borne_inf ; j<borne_sup ; j++) {
        for(k=0 ; k<sizes_nmad[j] ; k++) {
          buffer[j][k] ++;
        }
      }
    }
    stop_default = MPI_Wtime();
  }
  else if (comm_rank == 1) {
    dest = 0;
    for(i=0 ; i<LOOPS ; i++) {
      // pong
      for(j=borne_inf ; j<borne_sup ; j++) {
        // Reception pour un message de taille sizes_nmad[j]
        MPI_Recv(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      // Process data
      for(j=borne_inf ; j<borne_sup ; j++) {
        for(k=0 ; k<sizes_nmad[j] ; k++) {
          buffer[j][k] ++;
        }
      }
      // ping
      for(j=borne_inf ; j<borne_sup ; j++) {
        // Envoie un message de taille sizes_nmad[j]
        MPI_Send(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD);
      }
    }
  }
#endif /* TEST_DEFAULT */

#define TEST_AGGREG
#ifdef TEST_AGGREG
  if (comm_rank == 0) {
    dest = 1;
    start_aggreg = MPI_Wtime();
    for(i=0 ; i<LOOPS ; i++) {
      // ping
      for(j=borne_inf ; j<borne_sup ; j++) {
        // Envoie un message de taille sizes_nmad[j]
        MPI_Isend(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, &requests[j]);
      }
      for(j=borne_inf ; j<borne_sup ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
      // pong
      for(j=borne_inf ; j<borne_sup ; j++) {
        // Reception pour un message de taille sizes_nmad[j]
        MPI_Irecv(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, &requests[j]);
      }
      for(j=borne_inf ; j<borne_sup ; j++) {
	MPI_Status status;
        MPI_Wait(&requests[j], &status);
      }
      // Process data
      for(j=borne_inf ; j<borne_sup ; j++) {
        for(k=0 ; k<sizes_nmad[j] ; k++) {
          buffer[j][k] ++;
        }
      }
    }
    stop_aggreg = MPI_Wtime();
  }
  else if (comm_rank == 1) {
    dest = 0;
    for(i=0 ; i<LOOPS ; i++) {
      // pong
      for(j=borne_inf ; j<borne_sup ; j++) {
        // Reception pour un message de taille sizes_nmad[j]
        MPI_Irecv(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, &requests[j]);
      }
      for(j=borne_inf ; j<borne_sup ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
      // Process data
      for(j=borne_inf ; j<borne_sup ; j++) {
        for(k=0 ; k<sizes_nmad[j] ; k++) {
          buffer[j][k] ++;
        }
      }
      // ping
      for(j=borne_inf ; j<borne_sup ; j++) {
        // Envoie un message de taille sizes_nmad[j]
        MPI_Isend(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, &requests[j]);
      }
      for(j=borne_inf ; j<borne_sup ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
    }
  }
#endif /* TEST_AGGREG */

#define TEST_EXTENDED
#ifdef TEST_EXTENDED
  if (comm_rank == 0) {
    dest = 1;
    start_extended = MPI_Wtime();
    for(i=0 ; i<LOOPS ; i++) {
      // ping
      for(j=borne_inf ; j<borne_sup-1 ; j++) {
        // Envoie un message de taille sizes_nmad[j]
        MPI_Esend(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_IS_NOT_COMPLETED, MPI_COMM_WORLD, &requests[j]);
      }
      MPI_Esend(buffer[borne_sup-1], sizes_nmad[borne_sup-1], MPI_CHAR, dest, 0, MPI_IS_COMPLETED, MPI_COMM_WORLD, &requests[borne_sup-1]);
      for(j=borne_inf ; j<borne_sup ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
      // pong
      for(j=borne_inf ; j<borne_sup ; j++) {
        // Reception pour un message de taille sizes_nmad[j]
        MPI_Irecv(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, &requests[j]);
      }
      for(j=borne_inf ; j<borne_sup ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
      // Process data
      for(j=borne_inf ; j<borne_sup ; j++) {
        for(k=0 ; k<sizes_nmad[j] ; k++) {
          buffer[j][k] ++;
        }
      }
    }
    stop_extended = MPI_Wtime();
  }
  else if (comm_rank == 1) {
    dest = 0;
    for(i=0 ; i<LOOPS ; i++) {
      // pong
      for(j=borne_inf ; j<borne_sup ; j++) {
        // Reception pour un message de taille sizes_nmad[j]
        MPI_Irecv(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_COMM_WORLD, &requests[j]);
      }
      for(j=borne_inf ; j<borne_sup ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
      // Process data
      for(j=borne_inf ; j<borne_sup ; j++) {
        for(k=0 ; k<sizes_nmad[j] ; k++) {
          buffer[j][k] ++;
        }
      }
      // ping
      for(j=borne_inf ; j<borne_sup-1 ; j++) {
        // Envoie un message de taille sizes_nmad[j]
        //        if (j % NM_SO_PENDING_PACKS_WINDOW-1)
        MPI_Esend(buffer[j], sizes_nmad[j], MPI_CHAR, dest, 0, MPI_IS_NOT_COMPLETED, MPI_COMM_WORLD, &requests[j]);
      }
      MPI_Esend(buffer[borne_sup-1], sizes_nmad[borne_sup-1], MPI_CHAR, dest, 0, MPI_IS_COMPLETED, MPI_COMM_WORLD, &requests[borne_sup-1]);
      for(j=borne_inf ; j<borne_sup ; j++) {
        MPI_Wait(&requests[j], MPI_STATUS_IGNORE);
      }
    }
  }
#endif /* TEST_EXTENDED */

#define TEST_MEMCPY
#ifdef TEST_MEMCPY
  {
    char *bigbuffer, *ptr;
    // cree buffer de taille sizes_pastix[x]
    bigbuffer = malloc(size_pastix);
    if (comm_rank == 0) {
      start_memcopy = MPI_Wtime();
      for(i=0 ; i<LOOPS ; i++) {
        memset(bigbuffer, 0, size_pastix);
        ptr = bigbuffer;
        for(j=borne_inf ; j<borne_sup ; j++) {
          // copie dans buffer un message de taille sizes_nmad[j]
          memcpy(ptr, buffer[j], sizes_nmad[j]);
          ptr += sizes_nmad[j];
        }
        MPI_Send(bigbuffer, size_pastix, MPI_CHAR, dest, 0, MPI_COMM_WORLD);
        MPI_Recv(bigbuffer, size_pastix, MPI_CHAR, dest, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // Process data
        for(j=borne_inf ; j<borne_sup ; j++) {
          for(k=0 ; k<sizes_nmad[j] ; k++) {
            buffer[j][k] ++;
          }
        }
      }
      stop_memcopy = MPI_Wtime();
    }
    else if (comm_rank == 1) {
      for(i=0 ; i<LOOPS ; i++) {
        MPI_Recv(bigbuffer, size_pastix, MPI_CHAR, dest, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // Process data
        for(j=borne_inf ; j<borne_sup ; j++) {
          for(k=0 ; k<sizes_nmad[j] ; k++) {
            buffer[j][k] ++;
          }
        }
        MPI_Send(bigbuffer, size_pastix, MPI_CHAR, dest, 0, MPI_COMM_WORLD);
      }
    }
    free(bigbuffer);
  }
#endif /* TEST_MEMCPY */

  if (comm_rank == 0) {
    double defaut;
    double aggreg;
    double memcopy;
    double extended;
    double gain;

    defaut = (stop_default-start_default)/LOOPS/2 * 1000;
    aggreg = (stop_aggreg-start_aggreg)/LOOPS/2 * 1000;
    memcopy = (stop_memcopy-start_memcopy)/LOOPS/2 * 1000;
    extended = (stop_extended-start_extended)/LOOPS/2 * 1000;
    gain = 100 - (aggreg/memcopy * 100);

    fprintf(stdout, "%d\t%d\t%d\t\t%d\t%f\t%f\t\t%f\t%f\t\t%3.2f\n", borne_inf, borne_sup, size_pastix, nb_message, defaut, aggreg, memcopy, extended, gain);
  }

  for(i=borne_inf ; i<borne_sup ; i++) {
    free(buffer[i]);
  }
  free(buffer);
  free(requests);
}
