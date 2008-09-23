/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

#include "marcel.h"

void jacobi(int grid_size, int nb_workers, int nb_iters);
any_t worker(any_t arg);
void initialize_grids(double** grid1, double** grid2);
void barrier();

marcel_mutex_t mutex;  /* mutex semaphore for the barrier */
marcel_cond_t go;      /* condition variable for leaving */
int nb_arrived = 0;    /* count of the number who have arrived */

int grid_size, nb_workers, nb_iters, strip_size;
double *local_max_diff;

int marcel_main(int argc, char *argv[]) {
  marcel_init(&argc, argv);

  /* initialize mutex and condition variable */
  marcel_mutex_init(&mutex, NULL);
  marcel_cond_init(&go, NULL);

  /* read command line and initialize grids */
  if (argc < 4) {
    printf("jacobi size n_thread n_iter\n");
    exit(0);
  }
  grid_size = atoi(argv[1]);
  nb_workers = atoi(argv[2]);
  nb_iters = atoi(argv[3]);

  marcel_printf("# grid_size\tnb_workers\tnb_iters\tmax_diff\n");
  jacobi(grid_size, nb_workers, nb_iters);
}

void jacobi(int grid_size, int nb_workers, int nb_iters) {
  marcel_t *workerid;
  marcel_attr_t attr;
  int i, j;
  double maxdiff = 0.0;

  marcel_attr_init(&attr);
  strip_size = grid_size/nb_workers;
  local_max_diff = (double *) malloc(nb_workers * sizeof(double));
  workerid = (marcel_t *) malloc(nb_workers * sizeof(marcel_t));

  /* create the workers, then wait for them to finish */
  for (i = 0; i < nb_workers; i++) {
    marcel_attr_settopo_level(&attr, &marcel_topo_node_level[(i+1)%marcel_nbnodes]);
    marcel_create(&workerid[i], &attr, worker, (any_t) (intptr_t) i);
  }
  for (i = 0; i < nb_workers; i++) {
    marcel_join(workerid[i], NULL);
  }

  for (i = 0; i < nb_workers; i++) {
    if (maxdiff < local_max_diff[i]) maxdiff = local_max_diff[i];
  }
  /* print the results */
  marcel_printf("%11d %15d %13d\t%e\n", grid_size, nb_workers, nb_iters, maxdiff);
}

/*
 * Each Worker computes values in one strip of the grids.
 * The main worker loop does two computations to avoid copying from
 * one grid to the other.
 */
any_t worker(any_t arg) {
  int myid = (intptr_t) arg;
  double maxdiff, temp;
  int i, j, iters;
  int first, last;
  double **grid1;
  double **grid2;

  //marcel_printf("Thread #%d located on node #%d\n", myid, marcel_current_node());

  barrier();

  grid1 = (double**)malloc((strip_size+3)*sizeof(double*));
  grid2 = (double**)malloc((strip_size+3)*sizeof(double*));
  for(i = 0; i <= strip_size; i++) {
    grid1[i] = (double*)malloc((grid_size+3)*sizeof(double));
    grid2[i] = (double*)malloc((grid_size+3)*sizeof(double));
  }

  initialize_grids(grid1, grid2);

  /* determine first and last rows of my strip of the grids */
  for (iters = 1; iters <= nb_iters; iters++) {
    /* update my points */
    for (i = 1; i < strip_size; i++) {
      for (j = 1; j <= grid_size; j++) {
        grid2[i][j] = (grid1[i-1][j] + grid1[i+1][j] + grid1[i][j-1] + grid1[i][j+1]) * 0.25;
      }
    }
    barrier();
    /* update my points again */
    for (i = 1; i < strip_size; i++) {
      for (j = 1; j <= grid_size; j++) {
        grid1[i][j] = (grid2[i-1][j] + grid2[i+1][j] + grid2[i][j-1] + grid2[i][j+1]) * 0.25;
      }
    }
    barrier();
  }
  /* compute the maximum difference in my strip and set global variable */
  maxdiff = 0.0;
  for (i = 1; i <= strip_size; i++) {
    for (j = 1; j <= grid_size; j++) {
      temp = grid1[i][j]-grid2[i][j];
      if (temp < 0)
        temp = -temp;
      if (maxdiff < temp)
        maxdiff = temp;
    }
  }
  local_max_diff[myid] = maxdiff;
}

/*
 * Initialize the grids (grid1 and grid2)
 * set boundaries to 1.0 and interior points to 0.0
 */
void initialize_grids(double **grid1, double **grid2) {
  int i, j;

  for (i = 0; i <= strip_size; i++) {
    for (j = 0; j <= grid_size+1; j++) {
      grid1[i][j] = 0.0;
      grid2[i][j] = 0.0;
    }
  }
  for (i = 0; i <= strip_size; i++) {
    grid1[i][0] = 1.0;
    grid1[i][grid_size+1] = 1.0;
    grid2[i][0] = 1.0;
    grid2[i][grid_size+1] = 1.0;
  }
  for (j = 0; j <= grid_size+1; j++) {
    grid1[0][j] = 1.0;
    grid2[0][j] = 1.0;
    grid1[strip_size][j] = 1.0;
    grid2[strip_size][j] = 1.0;
  }
}

void barrier() {
  marcel_mutex_lock(&mutex);
  nb_arrived++;
  if (nb_arrived == nb_workers) {
    nb_arrived = 0;
    marcel_cond_broadcast(&go);
  }
  else {
    marcel_cond_wait(&go, &mutex);
  }
  marcel_mutex_unlock(&mutex);
}
