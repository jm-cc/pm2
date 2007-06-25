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

void print_buffer(int rank, int *buffer) {
  if (buffer[0] != 1 && buffer[1] != 2 && buffer[2] != 3 && buffer[3] != 4) {
    printf("[%d] Incorrect data\n", rank);
    printf("Received data [%d, %d, %d, %d]\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  }
  else {
    printf("[%d] Contig data successfully received\n", rank);
  }
}

void contig_datatype(int rank, int optimized) {
  MPI_Datatype mytype;
  MPI_Datatype mytype1;
  MPI_Datatype mytype2;
  MPI_Datatype mytype3;

  MPI_Type_contiguous(2, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_contiguous(2, mytype, &mytype2);
  MPI_Type_commit(&mytype2);

#if defined(MAD_MPI)
  if (optimized) {
    MPI_Type_optimized(&mytype, 1);
    MPI_Type_optimized(&mytype2, 1);
  }
#endif /* MAD_MPI */

  if (rank == 0) {
    int buffer[4] = {1, 2, 3, 4};
    MPI_Send(buffer, 4, MPI_INT, 1, 10, MPI_COMM_WORLD);
    MPI_Send(buffer, 2, mytype, 1, 10, MPI_COMM_WORLD);
    MPI_Send(buffer, 1, mytype2, 1, 10, MPI_COMM_WORLD);
  }
  else {
    int buffer[4], buffer2[4], buffer3[4];
    MPI_Recv(buffer, 4, MPI_INT, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    print_buffer(rank, buffer);

    MPI_Recv(buffer2, 2, mytype, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    print_buffer(rank, buffer2);

    MPI_Recv(buffer3, 1, mytype2, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    print_buffer(rank, buffer3);
  }

  MPI_Type_free(&mytype);
  MPI_Type_free(&mytype2);

  MPI_Type_contiguous(2, MPI_INT, &mytype1);
  MPI_Type_commit(&mytype1);
  MPI_Type_contiguous(2, MPI_INT, &mytype2);
  MPI_Type_commit(&mytype2);
  MPI_Type_contiguous(2, MPI_INT, &mytype3);
  MPI_Type_commit(&mytype3);
  MPI_Type_free(&mytype3);
  MPI_Type_contiguous(2, MPI_INT, &mytype3);
  MPI_Type_commit(&mytype3);
  MPI_Type_free(&mytype1);
  MPI_Type_free(&mytype2);
  MPI_Type_free(&mytype3);
}

void vector_datatype(int rank, int optimized) {
  MPI_Datatype mytype;
  MPI_Datatype mytype2;

  MPI_Type_vector(10, 2, 10, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_hvector(8, 3, 8*sizeof(float), MPI_FLOAT, &mytype2);
  MPI_Type_commit(&mytype2);

#if defined(MAD_MPI)
  if (optimized) {
    MPI_Type_optimized(&mytype, 1);
    MPI_Type_optimized(&mytype2, 1);
  }
#endif /* MAD_MPI */

  if (rank == 0) {
    int i;
    int buffer[100];
    float buffer2[64];

    for(i=0 ; i<100 ; i++) buffer[i] = i;
    for(i=0 ; i<64 ; i++) buffer2[i] = i;

    MPI_Send(buffer, 1, mytype, 1, 10, MPI_COMM_WORLD);
    MPI_Send(buffer2, 1, mytype2, 1, 10, MPI_COMM_WORLD);
  }
  else {
    int buffer[20];
    float buffer2[24];

    MPI_Recv(buffer, 1, mytype, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    {
      int i=0, success=1;
      int value=0;
      int count, blength, stride=10;

      for(count=0 ; count<10 ; count++) {
        for(blength=0 ; blength<2 ; blength++) {
          if (buffer[i] != value) {
            success=0;
            break;
          }
          i++;
          value++;
        }
        value += stride - 2;
      }

      if (success) {
        printf("[%d] Vector successfully received\n", rank);
      }
      else {
        printf("[%d] Incorrect vector: [", rank);
        for(i=0 ; i<20 ; i++) printf("%d ", buffer[i]);
        printf("]\n");
      }
    }

    MPI_Recv(buffer2, 1, mytype2, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    {
      int i=0, success=1;
      float value=0;
      int count, blength, stride=8;

      for(count=0 ; count<8 ; count++) {
        for(blength=0 ; blength<3 ; blength++) {
          if (buffer2[i] != value) {
            success=0;
            break;
          }
          i++;
          value++;
        }
        value += stride - 3;
      }

      if (success) {
        printf("[%d] Vector successfully received\n", rank);
      }
      else {
        printf("[%d] Incorrect vector: [", rank);
        for(i=0 ; i<24 ; i++) printf("%3.2f ", buffer2[i]);
        printf("]\n");
      }
    }
  }
  MPI_Type_free(&mytype);
  MPI_Type_free(&mytype2);
}

void indexed_datatype(int rank, int optimized) {
  MPI_Datatype mytype;
  MPI_Datatype mytype2;
  int blocklengths[3] = {1, 3, 2};
  int strides[3] = {0, 2, 6};
  MPI_Aint strides2[3] = {0*sizeof(char), 2*sizeof(char), 6*sizeof(char)};

  MPI_Type_indexed(3, blocklengths, strides, MPI_CHAR, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_hindexed(3, blocklengths, strides2, MPI_CHAR, &mytype2);
  MPI_Type_commit(&mytype2);

#if defined(MAD_MPI)
  if (optimized) {
    MPI_Type_optimized(&mytype, 1);
    MPI_Type_optimized(&mytype2, 1);
  }
#endif /* MAD_MPI */

  if (rank == 0) {
    char buffer[26] = {'a', 'b', 'c', 'd', 'e', 'f', 'g','h', 'i' ,'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

    MPI_Send(buffer, 3, mytype, 1, 10, MPI_COMM_WORLD);
    MPI_Send(buffer, 2, mytype2, 1, 10, MPI_COMM_WORLD);
  }
  else {
    char buffer[18];
    char buffer2[12];

    MPI_Recv(buffer, 3, mytype, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    {
      int i=0, j=0, success=1;
      char value;
      int nb, count, blength;

      for(nb=0 ; nb<3 ; nb++) {
        j=0;
        for(count=0 ; count<3 ; count++) {
          value = 'a' + (nb * (strides[2] + blocklengths[2]));
          value += strides[j];
          for(blength=0 ; blength<blocklengths[j] ; blength++) {
            if (buffer[i] != value) {
              success=0;
              break;
            }
            i++;
            value++;
          }
          j++;
        }
      }

      if (success) {
        printf("[%d] Index successfully received\n", rank);
      }
      else {
        printf("Incorrect index: [ ");
        for(i=0 ; i<18 ; i++) printf("%c(%d) ", buffer[i], ((int) buffer[i])-97);
        printf("]\n");
      }
    }

    MPI_Recv(buffer2, 2, mytype2, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    {
      int i=0, j=0, success=1;
      char value='a';
      int nb, count, blength;

      for(nb=0 ; nb<2 ; nb++) {
        j=0;
        for(count=0 ; count<3 ; count++) {
          value = 'a' + (nb * (strides[2] + blocklengths[2]));
          value += strides[j];
          for(blength=0 ; blength<blocklengths[j] ; blength++) {
            if (buffer[i] != value) {
              success=0;
              break;
            }
            i++;
            value++;
          }
          j++;
        }
      }

      if (success) {
        printf("[%d] Index successfully received\n", rank);
      }
      else {
        printf("Incorrect index: [ ");
        for(i=0 ; i<12 ; i++) printf("%c(%d) ", buffer[i], ((int) buffer[i])-97);
        printf("]\n");
      }
    }
  }

  MPI_Type_free(&mytype);
  MPI_Type_free(&mytype2);
}

void struct_datatype(int rank, int optimized) {
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

  MPI_Address(&(particle.class), &displacements[0]);
  MPI_Address(&(particle.d), &displacements[1]);
  MPI_Address(&(particle.b), &displacements[2]);
  for(i=2 ; i>=0 ; i--) displacements[i] -= displacements[0];

  //  printf("sizeof struct %lud, displacements[%d,%d,%d]\n", sizeof(struct part_s), displacements[0], displacements[1], displacements[2]);

  MPI_Type_struct(3, blocklens, displacements, types, &mytype);
  MPI_Type_commit(&mytype);

#if defined(MAD_MPI)
  if (optimized) {
    MPI_Type_optimized(&mytype, 1);
  }
#endif /* MAD_MPI */

  if (rank == 0) {
    struct part_s particles[10];
    int i;
    for(i=0 ; i<10 ; i++) {
      particles[i].class[0] = (char) i+97;
      particles[i].d[0] = (i+1)*10;
      particles[i].d[1] = (i+1)*2;
      particles[i].b[0] = i;
      particles[i].b[1] = i+2;
      particles[i].b[2] = i*3;
      particles[i].b[3] = i+5;
    }
    MPI_Send(particles, 10, mytype, 1, 10, MPI_COMM_WORLD);
  }
  else {
    struct part_s particles[10];
    MPI_Recv(particles, 10, mytype, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    {
      int i, success=1;
      for(i=0 ; i<10 ; i++) {
        if (particles[i].class[0] != (char) i+97 ||
            particles[i].d[0] != (i+1)*10 || particles[i].d[1] != (i+1)*2 ||
            particles[i].b[0] != i || particles[i].b[1] != i+2 ||
            particles[i].b[2] != i*3 || particles[i].b[3] != i+5) {
          success=0;
          break;
        }
      }

      if (success) {
        printf("[%d] Struct successfully received\n", rank);
      }
      else {
        for(i=0 ; i<10 ; i++) {
          printf("Receiving incorrect particle[%d] = {%c, {%3.2f, %3.2f} {%d, %d, %d, %d}\n",
                 i, particles[i].class[0], particles[i].d[0], particles[i].d[1],
                 particles[i].b[0], particles[i].b[1], particles[i].b[2], particles[i].b[3]);
        }
      }
    }
  }
  MPI_Type_free(&mytype);
}

void struct_and_indexed(int rank, int optimized) {
  MPI_Datatype struct_type, struct_type_ext;
  struct part_s {
    double d;
    int b;
  };
  MPI_Datatype types[2] = { MPI_DOUBLE, MPI_INT };
  int struct_blocklens[2] = { 1, 1 };
  MPI_Aint struct_displacements[2];
  struct part_s particle;
  MPI_Aint lb, extent;

  MPI_Datatype indexed_type;
  int indexed_blocklens[2] = { 3, 1 };
  int indexed_displacements[2] = { 0, 4*sizeof(struct part_s) };
  int indexed_displacements_aux[2] = { 0, 4 };

  MPI_Address(&(particle.d), &struct_displacements[0]);
  MPI_Address(&(particle.b), &struct_displacements[1]);
  struct_displacements[1] -= struct_displacements[0];
  struct_displacements[0] = 0;

  MPI_Type_struct(2, struct_blocklens, struct_displacements, types, &struct_type);
  MPI_Type_commit(&struct_type);

#if defined(MAD_MPI)
  if (optimized) {
    MPI_Type_optimized(&struct_type, 1);
  }
#endif /* MAD_MPI */

  MPI_Type_lb(struct_type, &lb);
  MPI_Type_extent(struct_type, &extent);
  if (extent != sizeof(struct part_s)) {
    MPI_Type_create_resized(struct_type, lb, sizeof(struct part_s), &struct_type_ext);
    MPI_Type_commit(&struct_type_ext);
    MPI_Type_indexed(2, indexed_blocklens, indexed_displacements, struct_type_ext, &indexed_type);
  }
  else {
    MPI_Type_indexed(2, indexed_blocklens, indexed_displacements, struct_type, &indexed_type);
  }

  MPI_Type_commit(&indexed_type);
#if defined(MAD_MPI)
  if (optimized) {
    MPI_Type_optimized(&indexed_type, 1);
  }
#endif /* MAD_MPI */

  //printf("Sizeof particle is %lud\n", sizeof(struct part_s));
  if (rank == 0) {
    struct part_s particles[20];
    int i;
    for(i=0 ; i<20 ; i++) {
      particles[i].d = (i+1)*10;
      particles[i].b = i+1;
      //printf("Sending Particle[%d] = {%3.2f, %d}\n", i, particles[i].d, particles[i].b);
    }
    MPI_Send(particles, 3, indexed_type, 1, 10, MPI_COMM_WORLD);
  }
  else {
    struct part_s particles[12];
    MPI_Recv(particles, 3, indexed_type, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    {
      int i=0, j, success=1;
      int nb, count, blength;
      int value;

      for(nb=0 ; nb<3 ; nb++) {
        j=0;
        for(count=0 ; count<2 ; count++) {
          value = nb * (indexed_displacements_aux[1] + indexed_blocklens[1]);
          value += indexed_displacements_aux[j];
          for(blength=0 ; blength<indexed_blocklens[j] ; blength++) {
            if (particles[i].d != (value+1)*10 || particles[i].b != value+1) {
              success=0;
              break;
            }
            i++;
            value++;
          }
          j++;
        }
      }

      if (success) {
        printf("[%d] Struct indexed successfully received\n", rank);
      }
      else {
        for(i=0 ; i<12 ; i++) {
          printf("Receiving incorrect particle[%d] = {%3.2f, %d}\n", i, particles[i].d, particles[i].b);
        }
      }
    }
  }
  if (extent != sizeof(struct part_s)) {
    MPI_Type_free(&struct_type_ext);
  }
  MPI_Type_free(&indexed_type);
  MPI_Type_free(&struct_type);
}

int main(int argc, char **argv) {
  int numtasks, rank;

  // Initialise MPI
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  contig_datatype(rank, 0);
  vector_datatype(rank, 0);
  indexed_datatype(rank, 0);
  struct_datatype(rank, 0);

  struct_and_indexed(rank, 0);

  contig_datatype(rank, 1);
  vector_datatype(rank, 1);
  indexed_datatype(rank, 1);
  struct_datatype(rank, 1);

  struct_and_indexed(rank, 1);

  MPI_Finalize();
  exit(0);
}
