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
#include <math.h>
#include <mpi.h>

#define BUFF_SIDE 8
#define BUFF_SIZE (BUFF_SIDE * BUFF_SIDE)
#define LEN_MAX  (220 / 8)

struct part_s{
  char c[3];
  double d[6];
  int i[4];
};

void mysleep(int n);
void print_arr(struct part_s*buff, int len, int ld);

int main(int argc, char**argv){
  int rank, i, nb_tests = 9;
  int ranks[1] = {0};
  struct part_s buff[BUFF_SIZE];
  MPI_Aint size, lb, ub;
  int disp_unit = sizeof(struct part_s);
  MPI_Win win;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  MPI_Get_address(&buff[0], &lb);
  MPI_Get_address(&buff[BUFF_SIZE], &ub);
  size = ub - lb;
    
  if(!rank){ //target

    memset(buff, 0, BUFF_SIZE * disp_unit);
    
    MPI_Win_create(buff, size, disp_unit, MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    
    for(i = 0; i < nb_tests; ++i){
      MPI_Barrier(MPI_COMM_WORLD);
      MPI_Win_sync(win);
      print_arr(buff, BUFF_SIZE, BUFF_SIDE);
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      memset(buff, 0, BUFF_SIZE * disp_unit);
      MPI_Win_unlock(0, win);
      MPI_Barrier(MPI_COMM_WORLD);
    }
    
  } else { //source

    struct part_s blank[BUFF_SIZE];
    MPI_Datatype type[2] = { MPI_CHAR, MPI_INT };
    int blocklen[2] = {1, 1};
    MPI_Aint disp[2];
    MPI_Datatype mpi_part_unsized_t, mpi_part_t, mpi_arr_t, mpi_diag_t, mpi_block_2_unsized_t, mpi_block_2_t, mpi_diag_block_2_t;

    disp[0] = (void*)&buff[0].c    - (void*)&buff[0];
    disp[1] = (void*)&buff[0].i[2] - (void*)&buff[0];
    
    //Initializations
    for(i = 0; i < BUFF_SIZE; ++i) {
      buff[i] = (struct part_s){ .c = ((i%26)+65),
				 .d = { i*M_PI, i*M_PI*2, i*M_PI*3, i*M_PI*4, i*M_PI*5, i*M_PI*6 },
				 .i = { 4 * i, 1 + 4 * i, 2 + 4 * i, 3 + 4 * i } };
    }
    memset(blank, 0, sizeof(struct part_s) * BUFF_SIZE);
    
    //Particule
    MPI_Type_create_struct(2, blocklen, disp, type, &mpi_part_unsized_t);
    MPI_Get_address(&buff[0], &lb);
    MPI_Get_address(&buff[1], &ub);
    MPI_Type_create_resized(mpi_part_unsized_t, 0, ub - lb, &mpi_part_t);
    MPI_Type_commit(&mpi_part_t);

    //Complete array
    MPI_Type_contiguous(BUFF_SIZE, mpi_part_t, &mpi_arr_t);
    MPI_Type_commit(&mpi_arr_t);

    //Extract diag
    MPI_Type_vector(BUFF_SIDE, 1, BUFF_SIDE + 1, mpi_part_t, &mpi_diag_t);
    MPI_Type_commit (&mpi_diag_t);

    //Extract a 2x2 block
    MPI_Type_vector(2, 2, BUFF_SIDE, mpi_part_t, &mpi_block_2_unsized_t);
    MPI_Get_address(&buff[0], &lb);
    MPI_Get_address(&buff[2 * BUFF_SIDE + 2], &ub);
    MPI_Type_create_resized(mpi_block_2_unsized_t, 0, ub - lb, &mpi_block_2_t);
    MPI_Type_commit(&mpi_block_2_t);

    //Extract diag of 2x2 block
    MPI_Type_vector(BUFF_SIDE / 2, 1, 1, mpi_block_2_t, &mpi_diag_block_2_t);
    MPI_Type_commit(&mpi_diag_block_2_t);

    MPI_Win_create(NULL, 0, disp_unit, MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    
    buff[0] = (struct part_s){ .c = '@',
			       .d = { M_E, M_E, M_E, M_E, M_E, M_E },
			       .i = { 42, 42, 42, 42 } };
    { // Single particule
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      MPI_Put(buff, 1, mpi_part_t, 0, 0, 1, mpi_part_t, win);
      MPI_Win_unlock(0, win);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    buff[0] = (struct part_s){ .c = 'A',
			       .d = { 0, 0, 0, 0, 0, 0 },
			       .i = { 0, 1, 2, 3 } };
    MPI_Barrier(MPI_COMM_WORLD);
    { // Array of particules to array of particules
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      MPI_Put(buff, 1, mpi_arr_t, 0, 0, BUFF_SIZE, mpi_part_t, win);
      MPI_Win_unlock(0, win);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    { // Matrix diag to array of particules
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      MPI_Put(buff, 1, mpi_diag_t, 0, 0, BUFF_SIDE, mpi_part_t, win);
      MPI_Win_unlock(0, win);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    { // Matrix diag 2x2 blocks to array of particules
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      MPI_Put(buff, BUFF_SIDE / 2, mpi_block_2_t, 0, 0, 2 * BUFF_SIDE, mpi_part_t, win);
      MPI_Win_unlock(0, win);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    { // Matrix diag 2x2 blocks to array of particules
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      MPI_Put(buff, 1, mpi_diag_block_2_t, 0, 0, 2 * BUFF_SIDE, mpi_part_t, win);
      MPI_Win_unlock(0, win);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    { // Array of particules to matrix diag
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      MPI_Put(buff, BUFF_SIDE, mpi_part_t, 0, 0, 1, mpi_diag_t, win);
      MPI_Win_unlock(0, win);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    { // Array of particules to matrix diag 2x2 blocks
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      MPI_Put(buff, 2 * BUFF_SIDE, mpi_part_t, 0, 0, BUFF_SIDE / 2, mpi_block_2_t, win);
      MPI_Win_unlock(0, win);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    { // Array of particules to matrix diag 2x2 blocks
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      MPI_Put(buff, 2 * BUFF_SIDE, mpi_part_t, 0, 0, 1, mpi_diag_block_2_t, win);
      MPI_Win_unlock(0, win);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    { // Matrix diag 2x2 blocks to matrix diag 2x2 blocks
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      MPI_Put(buff, 1, mpi_diag_block_2_t, 0, 0, 1, mpi_diag_block_2_t, win);
      MPI_Win_unlock(0, win);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Type_free(&mpi_diag_block_2_t);
    MPI_Type_free(&mpi_block_2_t);
    MPI_Type_free(&mpi_block_2_unsized_t);
    MPI_Type_free(&mpi_diag_t);
    MPI_Type_free(&mpi_arr_t);
    MPI_Type_free(&mpi_part_t);
    MPI_Type_free(&mpi_part_unsized_t);

  }
  
  MPI_Win_free(&win);  
  MPI_Finalize();

  return EXIT_SUCCESS;
}

void mysleep(int n){
  fprintf(stderr, "Sleeps %d secs...\n", n);
  sleep(n);
}

void print_arr(struct part_s*buff, int len, int ld){
  int i;
  printf("Recieved :");
  for(i = 0; i < len && (ld < LEN_MAX || i < LEN_MAX); ++i){
    if(!(i%ld))
      printf("\n");
    printf("{%c, %3d} ", *buff[i].c != 0 ? *buff[i].c : ' ', buff[i].i[2]);
  }
  printf("\n\n");
}
