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

//#define DATATYPE_DEBUG

void struct_datatype(int rank);

int main(int argc, char **argv) {
  int rank;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  struct_datatype(rank);

  MPI_Finalize();
  exit(0);
}

void struct_datatype(int rank) {
  struct part_s {
    char class[1];
    double d[2];
    int b[4];
  };
  MPI_Datatype mytype;
  MPI_Datatype types[3] = { MPI_CHAR, MPI_DOUBLE, MPI_INT };
  int blocklens[3] = { 1, 2, 4 };
  MPI_Aint displacements[3];
  struct part_s particle;
  int i;
  int ping_side, rank_dst;

  ping_side = !(rank & 1);
  rank_dst = ping_side?(rank | 1) : (rank & ~1);

  MPI_Address(&(particle.class), &displacements[0]);
  MPI_Address(&(particle.d), &displacements[1]);
  MPI_Address(&(particle.b), &displacements[2]);
  for(i=2 ; i>=0 ; i--) displacements[i] -= displacements[0];

#ifdef DATATYPE_DEBUG
  printf("sizeof struct %ld, displacements[%ld,%ld,%ld]\n", sizeof(struct part_s), displacements[0], displacements[1], displacements[2]);
#endif

  MPI_Type_struct(3, blocklens, displacements, types, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_optimized(&mytype, 1);

  if (ping_side) {
    struct part_s particles[10];

    for(i=0 ; i<10 ; i++) {
      particles[i].class[0] = (char) i+97;
      particles[i].d[0] = (i+1)*10;
      particles[i].d[1] = (i+1)*2;
      particles[i].b[0] = i;
      particles[i].b[1] = i+2;
      particles[i].b[2] = i*3;
      particles[i].b[3] = i+5;
    }

#ifdef DATATYPE_DEBUG
    for(i=0 ; i<10 ; i++) {
      printf("Sending Particle[%d] = {%c, {%3.2f, %3.2f} {%d, %d, %d, %d}\n", i, particles[i].class[0], particles[i].d[0], particles[i].d[1],
             particles[i].b[0], particles[i].b[1], particles[i].b[2], particles[i].b[3]);
    }
#endif

    MPI_Send(particles, 10, mytype, rank_dst, 10, MPI_COMM_WORLD);
  }
  else {
    struct part_s particles[10];
    int success=1;
    MPI_Request requests[30];

    for(i=0 ; i<10 ; i++) {
      MPI_Irecv(&(particles[i].class), 1, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, &requests[i*3]);
      MPI_Irecv(&(particles[i].d[0]), 2, MPI_DOUBLE, rank_dst, 10, MPI_COMM_WORLD, &requests[i*3+1]);
      MPI_Irecv(&(particles[i].b[0]), 4, MPI_INT, rank_dst, 10, MPI_COMM_WORLD, &requests[i*3+2]);
    }
    MPI_Wait(&requests[29], MPI_STATUS_IGNORE);

    for(i=0 ; i<10 ; i++) {
      if ((particles[i].class[0] != (char) i+97) ||
	  (particles[i].d[0] != (i+1)*10) ||
	  (particles[i].d[1] != (i+1)*2) ||
	  (particles[i].b[0] != i) ||
	  (particles[i].b[1] != i+2) ||
	  (particles[i].b[2] != i*3) ||
	  (particles[i].b[3] != i+5))
	success=0;
    }

    if (success) {
      printf("Success\n");
    }
    else {
      for(i=0 ; i<10 ; i++) {
	printf("Error receiving Particle[%d] = {%c, {%3.2f, %3.2f} {%d, %d, %d, %d}\n", i, particles[i].class[0], particles[i].d[0], particles[i].d[1],
	       particles[i].b[0], particles[i].b[1], particles[i].b[2], particles[i].b[3]);
      }
    }
  }

  MPI_Type_free(&mytype);
}
