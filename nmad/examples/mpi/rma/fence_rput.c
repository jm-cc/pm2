/*
 * NewMadeleine
 * Copyright (C) 2016 (see AUTHORS file)
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>

#define BUFF_SIDE 16
#define BUFF_SIZE (BUFF_SIDE * BUFF_SIDE)
#define LEN_MAX  (220 / 4)

void mysleep(int n);
void print_arr(int*buff, int len, int ld);

int main(int argc, char**argv){
  int rank, i;
  int ranks[1] = {0};
  int buff[BUFF_SIZE];
  MPI_Aint size, lb, ub;
  int disp_unit = sizeof(int);
  MPI_Win win;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  MPI_Get_address(&buff[0], &lb);
  MPI_Get_address(&buff[BUFF_SIZE], &ub);
  size = ub - lb;
  
  if(!rank){ //target

    MPI_Win_create(buff, size, disp_unit, MPI_INFO_NULL, MPI_COMM_WORLD, &win);
      
    memset(buff, 0, BUFF_SIZE * sizeof(int));
    MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
    MPI_Win_fence(MPI_MODE_NOSUCCEED, win);
    print_arr(buff, BUFF_SIZE, BUFF_SIDE);
    
  } else { //source
    
    MPI_Win_create(NULL, 0, disp_unit, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Datatype mpi_arr_t;
    MPI_Request request;
    
    //Initializations
    for(i = 0; i < BUFF_SIZE; ++i)
      buff[i] = i;

    //Complete array
    MPI_Type_contiguous(BUFF_SIZE, MPI_INT, &mpi_arr_t);
    MPI_Type_commit(&mpi_arr_t);

    MPI_Win_fence(MPI_MODE_NOPRECEDE, win);    

    MPI_Rput(buff, 1, mpi_arr_t, 0, 0, BUFF_SIZE, MPI_INT, win, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
    
    MPI_Win_fence(MPI_MODE_NOSUCCEED, win);
    
    MPI_Type_free(&mpi_arr_t);
    
  }
  
  MPI_Win_free(&win);  
  MPI_Finalize();

  return EXIT_SUCCESS;
}

void mysleep(int n){
  fprintf(stderr, "Sleeps %d secs...\n", n);
  sleep(n);
}

void print_arr(int*buff, int len, int ld){
  int i;
  printf("Recieved :");
  for(i = 0; i < len && (ld < LEN_MAX || i < LEN_MAX); ++i){
    if(!(i%ld))
      printf("\n");
    printf("%3d ", buff[i]);
  }
  printf("\n\n");
}
