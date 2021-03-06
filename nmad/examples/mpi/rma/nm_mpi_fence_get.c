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
#include <assert.h>

#define BUFF_SIDE 16
#define BUFF_SIZE (BUFF_SIDE * BUFF_SIDE)
#define LEN_MAX  (220 / 4)

void mysleep(int n);
void print_arr(int*buff, int len, int ld);

int main(int argc, char**argv){
  int rank, i, nb_tests = 9;
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
    
    //Initializations
    for(i = 0; i < BUFF_SIZE; ++i){
      buff[i] = i;
    }

    MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
  
    for(i = 0; i < nb_tests; ++i){
      MPI_Win_fence(0, win);
      mysleep(1); /* Simulate computation time */
      MPI_Win_fence(0, win);
    }
    
  } else { //source

    MPI_Win_create(NULL, 0, disp_unit, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Datatype mpi_arr_t, mpi_diag_t, mpi_block_2_unsized_t, mpi_block_2_t, mpi_diag_block_2_t;
    
    //Complete array
    MPI_Type_contiguous(BUFF_SIZE, MPI_INT, &mpi_arr_t);
    MPI_Type_commit(&mpi_arr_t);

    //Extract diag
    MPI_Type_vector(BUFF_SIDE, 1, BUFF_SIDE + 1, MPI_INT, &mpi_diag_t);
    MPI_Type_commit (&mpi_diag_t);

    //Extract a 2x2 block
    MPI_Type_vector(2, 2, BUFF_SIDE, MPI_INT, &mpi_block_2_unsized_t);
    MPI_Get_address(&buff[0], &lb);
    MPI_Get_address(&buff[2 * BUFF_SIDE + 2], &ub);
    MPI_Type_create_resized(mpi_block_2_unsized_t, 0, ub - lb, &mpi_block_2_t);
    MPI_Type_commit(&mpi_block_2_t);

    //Extract diag of 2x2 block
    MPI_Type_vector(BUFF_SIDE / 2, 1, 1, mpi_block_2_t, &mpi_diag_block_2_t);
    MPI_Type_commit(&mpi_diag_block_2_t);

    //Initializations
    memset(buff, 0, BUFF_SIZE * disp_unit);
    buff[0] = 42;

    MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
    
    { // Single int
      MPI_Win_fence(0, win);
      MPI_Get(buff, 1, MPI_INT, 0, 0, 1, MPI_INT, win);
      MPI_Win_fence(0, win);
      print_arr(buff, BUFF_SIZE, BUFF_SIDE);
    }
    buff[0] = 0;
    { // Array of ints to array of int
      MPI_Win_fence(0, win);
      MPI_Get(buff, 1, mpi_arr_t, 0, 0, BUFF_SIZE, MPI_INT, win);
      MPI_Win_fence(0, win);
      print_arr(buff, BUFF_SIZE, BUFF_SIDE);
    }    
    memset(buff, 0, BUFF_SIZE * disp_unit);
    { // Matrix diag to array of int
      MPI_Win_fence(0, win);
      MPI_Get(buff, BUFF_SIDE, MPI_INT, 0, 0, 1, mpi_diag_t, win);
      MPI_Win_fence(0, win);
      print_arr(buff, BUFF_SIZE, BUFF_SIDE);
    }
    memset(buff, 0, BUFF_SIZE * disp_unit);
    { // Matrix diag 2x2 blocks to array of int
      MPI_Win_fence(0, win);
      MPI_Get(buff, 2 * BUFF_SIDE, MPI_INT, 0, 0, BUFF_SIDE / 2, mpi_block_2_t, win);
      MPI_Win_fence(0, win);
      print_arr(buff, BUFF_SIZE, BUFF_SIDE);
    }
    memset(buff, 0, BUFF_SIZE * disp_unit);
    { // Matrix diag 2x2 blocks to array of int
      MPI_Win_fence(0, win);
      MPI_Get(buff, 2 * BUFF_SIDE, MPI_INT, 0, 0, 1, mpi_diag_block_2_t, win);
      MPI_Win_fence(0, win);
      print_arr(buff, BUFF_SIZE, BUFF_SIDE);
    }
    memset(buff, 0, BUFF_SIZE * disp_unit);
    { // Array of int to matrix diag
      MPI_Win_fence(0, win);
      MPI_Get(buff, 1, mpi_diag_t, 0, 0, BUFF_SIDE, MPI_INT, win);
      MPI_Win_fence(0, win);
      print_arr(buff, BUFF_SIZE, BUFF_SIDE);
    }
    memset(buff, 0, BUFF_SIZE * disp_unit);
    { // Array of int to matrix diag 2x2 blocks
      MPI_Win_fence(0, win);
      MPI_Get(buff, BUFF_SIDE / 2, mpi_block_2_t, 0, 0, 2 * BUFF_SIDE, MPI_INT, win);
      MPI_Win_fence(0, win);
      print_arr(buff, BUFF_SIZE, BUFF_SIDE);
    }
    memset(buff, 0, BUFF_SIZE * disp_unit);
    { // Array of int to matrix diag 2x2 blocks
      MPI_Win_fence(0, win);
      MPI_Get(buff, 1, mpi_diag_block_2_t, 0, 0, 2 * BUFF_SIDE, MPI_INT, win);
      MPI_Win_fence(0, win);
      print_arr(buff, BUFF_SIZE, BUFF_SIDE);
    }
    memset(buff, 0, BUFF_SIZE * disp_unit);
    { // Matrix diag 2x2 blocks to matrix diag 2x2 blocks
      MPI_Win_fence(0, win);
      MPI_Get(buff, 1, mpi_diag_block_2_t, 0, 0, 1, mpi_diag_block_2_t, win);
      MPI_Win_fence(0, win);
      print_arr(buff, BUFF_SIZE, BUFF_SIDE);
    }

    MPI_Type_free(&mpi_diag_block_2_t);
    MPI_Type_free(&mpi_block_2_t);
    MPI_Type_free(&mpi_block_2_unsized_t);
    MPI_Type_free(&mpi_diag_t);
    MPI_Type_free(&mpi_arr_t);

  }

  MPI_Win_fence(MPI_MODE_NOSUCCEED, win);
  
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
  printf("Grabbed :");
  for(i = 0; i < len && (ld < LEN_MAX || i < LEN_MAX); ++i){
    if(!(i%ld))
      printf("\n");
    printf("%3d ", buff[i]);
  }
  printf("\n\n");
}
