#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

void print_buffer(int *buffer) {
  if (buffer[0] != 1 && buffer[1] != 2 && buffer[2] != 3 && buffer[3] != 4) {
    printf("Incorrect data\n");
  }
  printf("Received data [%d, %d, %d, %d]\n", buffer[0], buffer[1], buffer[2], buffer[3]);
}

void contig_datatype(int rank) {
  MPI_Datatype mytype;
  MPI_Datatype mytype3;
  MPI_Datatype mytype2;

  MPI_Type_contiguous(2, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_contiguous(2, mytype, &mytype2);
  MPI_Type_commit(&mytype2);

  if (rank == 0) {
    int buffer[4] = {1, 2, 3, 4};
    MPI_Send(buffer, 4, MPI_INT, 1, 10, MPI_COMM_WORLD);
    MPI_Send(buffer, 2, mytype, 1, 10, MPI_COMM_WORLD);
    MPI_Send(buffer, 1, mytype2, 1, 10, MPI_COMM_WORLD);
  }
  else {
    int buffer[4], buffer2[4], buffer3[4];
    MPI_Recv(buffer, 4, MPI_INT, 0, 10, MPI_COMM_WORLD, NULL);
    print_buffer(buffer);

    MPI_Recv(buffer2, 2, mytype, 0, 10, MPI_COMM_WORLD, NULL);
    print_buffer(buffer2);

    MPI_Recv(buffer3, 1, mytype2, 0, 10, MPI_COMM_WORLD, NULL);
    print_buffer(buffer3);
  }

  MPI_Type_contiguous(2, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_contiguous(2, MPI_INT, &mytype3);
  MPI_Type_commit(&mytype3);
  MPI_Type_contiguous(2, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_free(&mytype3);
  MPI_Type_contiguous(2, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);
}

void vector_datatype(int rank) {
  MPI_Datatype mytype;
  MPI_Datatype mytype2;

  MPI_Type_vector(10, 2, 10, MPI_INT, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_hvector(8, 3, 8*sizeof(MPI_FLOAT), MPI_FLOAT, &mytype2);
  MPI_Type_commit(&mytype2);

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
    int i;
    int buffer[20];
    float buffer2[24];

    MPI_Recv(buffer, 1, mytype, 0, 10, MPI_COMM_WORLD, NULL);
    printf("Vector: [");
    for(i=0 ; i<20 ; i++) printf("%d ", buffer[i]);
    printf("]\n");

    MPI_Recv(buffer2, 1, mytype2, 0, 10, MPI_COMM_WORLD, NULL);
    printf("Vector: [");
    for(i=0 ; i<24 ; i++) printf("%3.2f ", buffer2[i]);
    printf("]\n");
  }
}

void indexed_datatype(int rank) {
  MPI_Datatype mytype;
  MPI_Datatype mytype2;
  int blocklengths[3] = {1, 3, 2};
  int strides[3] = {0, 2, 6};
  int strides2[3] = {0*sizeof(MPI_CHAR), 2*sizeof(MPI_CHAR), 6*sizeof(MPI_CHAR)};

  MPI_Type_indexed(3, blocklengths, strides, MPI_CHAR, &mytype);
  MPI_Type_commit(&mytype);
  MPI_Type_indexed(3, blocklengths, strides2, MPI_CHAR, &mytype2);
  MPI_Type_commit(&mytype2);

  if (rank == 0) {
    char buffer[26] = {'a', 'b', 'c', 'd', 'e', 'f', 'g','h', 'i' ,'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

    MPI_Send(buffer, 3, mytype, 1, 10, MPI_COMM_WORLD);
    MPI_Send(buffer, 2, mytype2, 1, 10, MPI_COMM_WORLD);
  }
  else {
    char buffer[18];
    char buffer2[12];
    int i;

    MPI_Recv(buffer, 3, mytype, 0, 10, MPI_COMM_WORLD, NULL);
    printf("Index: [ ");
    for(i=0 ; i<18 ; i++) printf("%c(%d) ", buffer[i], ((int) buffer[i])-97);
    printf("]\n");

    MPI_Recv(buffer2, 2, mytype2, 0, 10, MPI_COMM_WORLD, NULL);
    printf("Index: [ ");
    for(i=0 ; i<12 ; i++) printf("%c(%d) ", buffer[i], ((int) buffer[i])-97);
    printf("]\n");
  }
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

  MPI_Get_address(&(particle.class), &displacements[0]);
  MPI_Get_address(&(particle.d), &displacements[1]);
  MPI_Get_address(&(particle.b), &displacements[2]);
  for(i=2 ; i>=0 ; i--) displacements[i] -= displacements[0];

  printf("sizeof struct %d, displacements[%d,%d,%d]\n", sizeof(struct part_s), displacements[0], displacements[1], displacements[2]);

  MPI_Type_struct(3, blocklens, displacements, types, &mytype);
  MPI_Type_commit(&mytype);

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
    for(i=0 ; i<10 ; i++) {
      printf("Sending Particle[%d] = {%c, {%3.2f, %3.2f} {%d, %d, %d, %d}\n", i, particles[i].class[0], particles[i].d[0], particles[i].d[1],
             particles[i].b[0], particles[i].b[1], particles[i].b[2], particles[i].b[3]);
    }

    MPI_Send(particles, 10, mytype, 1, 10, MPI_COMM_WORLD);
  }
  else {
    struct part_s particles[10];
    int i;
    MPI_Recv(particles, 10, mytype, 0, 10, MPI_COMM_WORLD, NULL);
    for(i=0 ; i<10 ; i++) {
      printf("Receiving Particle[%d] = {%c, {%3.2f, %3.2f} {%d, %d, %d, %d}\n", i, particles[i].class[0], particles[i].d[0], particles[i].d[1],
             particles[i].b[0], particles[i].b[1], particles[i].b[2], particles[i].b[3]);
    }
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

//  contig_datatype(rank);
//  vector_datatype(rank);
//  indexed_datatype(rank);

  struct_datatype(rank);

  MPI_Finalize();
  exit(0);
}
