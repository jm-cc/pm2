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
#include <unistd.h>

void my_operation(void *a, void *b, int *r, MPI_Datatype *type);

int main(int argc, char **argv) {
  int numtasks, rank, provided;

  // Initialise MPI
  MPI_Init_thread(&argc,&argv, MPI_THREAD_SINGLE, &provided);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  {
    int global_rank;
    if (rank == 0) {
      global_rank = 49;
    }
    MPI_Bcast(&global_rank, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (global_rank == 49) {
      fprintf(stdout, "[%d] Success for broadcasted message\n", rank);
    }
    else {
      fprintf(stdout, "[%d] Error! Broadcasted message [%d]\n", rank, global_rank);
    }
  }

  {
    float buffer[2];
    if (rank == 0) {
      buffer[0] = 12.45;
      buffer[1] = 3.14;
    }
    MPI_Bcast(buffer, 2, MPI_FLOAT, 0, MPI_COMM_WORLD);
    if ((buffer[0] == (float)12.45) && (buffer[1] == (float)3.14)) {
      fprintf(stdout, "[%d] Success for broadcasted message\n", rank);
    }
    else {
      fprintf(stdout, "[%d] Error! Broadcasted message [%f,%f]\n", rank, buffer[0], buffer[1]);
    }
  }

  {
    int reduce[2];
    int global_sum[2];
    reduce[0] = rank*10;
    reduce[1] = 15+rank;
    MPI_Reduce(reduce, global_sum, 2, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
      int i, sum1 = 0, sum2 = 0;
      for(i=0 ; i<numtasks ; i++) {
        sum1 += i*10;
        sum2 += 15 + i;
      }
      if (sum1 == global_sum[0] && sum2 == global_sum[1]) {
        fprintf(stdout, "[%d] Success for global sum\n", rank);
      }
      else {
        fprintf(stdout, "[%d] Error! Global sum [%d,%d]\n", rank, global_sum[0], global_sum[1]);
      }
    }
  }

  {
    long lreduce[2];
    long global_product[2];
    int i;
    long product0=1, product1=1;

    lreduce[0] = (rank+1)*2;
    lreduce[1] = rank+1;
    MPI_Allreduce(lreduce, global_product, 2, MPI_LONG, MPI_PROD, MPI_COMM_WORLD);

    for(i=0 ; i<numtasks ; i++) {
      product0 *= (i+1)*2;
      product1 *= (i+1);
    }
    if (product0 == global_product[0] && product1 == global_product[1]) {
      fprintf(stdout, "[%d] Success for global product\n", rank);
    }
    else {
      fprintf(stdout, "[%d] Error! Global product [%li,%li]\n", rank, global_product[0], global_product[1]);
    }
  }

  {
    int reduce[3] = {rank+1, rank+2, rank+3};
    int global_reduce[3];
    MPI_Op operator;
    int i, reduce0=1, reduce1=2, reduce2=3;

    MPI_Op_create(my_operation, 1, &operator);
    MPI_Allreduce(reduce, global_reduce, 3, MPI_INT, operator, MPI_COMM_WORLD);
    MPI_Op_free(&operator);

    for(i=1 ; i<numtasks ; i++) {
      reduce0 += i+1; reduce0 *= 2;
      reduce1 += i+2; reduce1 *= 2;
      reduce2 += i+3; reduce2 *= 2;
    }
    if (reduce0 == global_reduce[0] && reduce1 == global_reduce[1] && reduce2 == global_reduce[2]) {
      fprintf(stdout, "[%d] Success for global reduction\n", rank);
    }
    else {
      fprintf(stdout, "[%d] Error! Global reduction [%d,%d,%d]\n", rank, global_reduce[0], global_reduce[1], global_reduce[2]);
    }
  }

  MPI_Finalize();
  exit(0);
}

void my_operation(void *a, void *b, int *r, MPI_Datatype *type) {
  int *in, *inout, i;

  if (*type != MPI_INT) {
    fprintf(stdout, "Erreur. Argument type not recognized\n");
    exit(-1);
  }
  in = (int *) a;
  inout = (int *) b;
  for(i=0 ; i<*r ; i++) {
    //printf("Inout[%d] = %d\n", i, inout[i]);
    inout[i] += in[i];
    inout[i] *= 2;
    //printf("Inout[%d] = %d\n", i, inout[i]);
  }
}
