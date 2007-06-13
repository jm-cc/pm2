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

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#define NB_PARTICLES 30

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
  int sizeof_particle;
  int i;

  MPI_Get_address(&(particle.class), &displacements[0]);
  MPI_Get_address(&(particle.d), &displacements[1]);
  MPI_Get_address(&(particle.b), &displacements[2]);
  for(i=2 ; i>=0 ; i--) displacements[i] -= displacements[0];

  printf("sizeof struct %ud displacements[%d,%d,%d]\n", sizeof(struct part_s), displacements[0], displacements[1], displacements[2]);

  MPI_Type_struct(3, blocklens, displacements, types, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_size(mytype, &sizeof_particle);

  printf("size of struct type : %d\n", sizeof_particle);

  if (rank == 0) {
    struct part_s particles[NB_PARTICLES];
    MPI_Request request1, request2;
    int i;

    for(i=0 ; i<NB_PARTICLES ; i++) {
      particles[i].class[0] = (char) i+97;
      particles[i].d[0] = (i+1)*10;
      particles[i].d[1] = (i+1)*2;
      particles[i].b[0] = i;
      particles[i].b[1] = i+2;
      particles[i].b[2] = i*3;
      particles[i].b[3] = i+5;
    }
    for(i=0 ; i<NB_PARTICLES ; i++) {
      printf("Sending Particle[%d] = {%c, {%3.2f, %3.2f} {%d, %d, %d, %d}\n", i, particles[i].class[0], particles[i].d[0], particles[i].d[1],
             particles[i].b[0], particles[i].b[1], particles[i].b[2], particles[i].b[3]);
    }
    MPI_Isend(particles, NB_PARTICLES, mytype, 1, 11, MPI_COMM_WORLD, &request1);
    MPI_Isend(particles, NB_PARTICLES, mytype, 1, 10, MPI_COMM_WORLD, &request2);
    MPI_Wait(&request1, MPI_STATUS_IGNORE);
    MPI_Wait(&request2, MPI_STATUS_IGNORE);
  }
  else {
    struct part_s particles[NB_PARTICLES];
    void *buffer, *ptr;
    int i;
    char *class;
    double *d;
    int *b;

    buffer = malloc(NB_PARTICLES*sizeof_particle);
    MPI_Recv(particles, NB_PARTICLES, mytype, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(buffer, NB_PARTICLES * sizeof_particle, MPI_BYTE, 0, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    ptr = buffer;
    for(i=0 ; i<NB_PARTICLES ; i++) {
      class = (char *) ptr;
      d = (double *) (ptr + sizeof(char));
      b = (int *) (ptr + sizeof(char) + 2*sizeof(double));
      printf("Receiving Particle[%d] = {%c, {%3.2f, %3.2f} {%d, %d, %d, %d}\n", i, particles[i].class[0], particles[i].d[0], particles[i].d[1],
             particles[i].b[0], particles[i].b[1], particles[i].b[2], particles[i].b[3]);
      printf("Receiving Particle[%d] = {%c, {%3.2f, %3.2f} {%d, %d, %d, %d}\n", i, class[0], d[0], d[1], b[0], b[1], b[2], b[3]);
      ptr += sizeof(char) + 2*sizeof(double) + 4*sizeof(int);
    }
    free(buffer);
  }

  MPI_Type_free(&mytype);
}

int main(int argc, char **argv) {
  int numtasks, rank;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  printf("Rank %d Size %d\n", rank, numtasks);

  struct_datatype(rank);

  MPI_Finalize();
  exit(0);
}
