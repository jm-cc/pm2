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

#if defined(MARCEL_MAMI_ENABLED)

#define JACOBI_MIGRATE_ON_NEXT_TOUCH_USERSPACE  0
#define JACOBI_MIGRATE_ON_NEXT_TOUCH_KERNEL     1
#define JACOBI_MIGRATE_ON_FIRST_TOUCH           2

char *migration_policy_texts[3] = {"NEXT_TOUCH_USERSPACE",
                                   "NEXT_TOUCH_KERNEL   ",
                                   "FIRST_TOUCH         "};
typedef struct jacobi_s {
  int thread_id;
  int grid_size;
  int nb_workers;
  int nb_iters;
  int strip_size;
  int migration_policy;
} jacobi_t;

void jacobi(int grid_size, int nb_workers, int nb_iters, int migration_policy);
any_t worker(any_t arg);
void barrier(int nb_workers);

marcel_mutex_t mutex;  /* mutex semaphore for the barrier */
marcel_cond_t go;      /* condition variable for leaving */
int nb_arrived = 0;    /* count of the number who have arrived */

double *local_max_diff;
double **grid1, **grid2;

int marcel_main(int argc, char *argv[]) {
  int grid_size, nb_workers, nb_iters, migration_policy;

  marcel_init(&argc, argv);
  marcel_mutex_init(&mutex, NULL);
  marcel_cond_init(&go, NULL);

  if (argc == 5) {
    grid_size = atoi(argv[1]);
    nb_workers = atoi(argv[2]);
    nb_iters = atoi(argv[3]);
    migration_policy = atoi(argv[4]);

    jacobi(grid_size, nb_workers, nb_iters, migration_policy);
  }

  // Finish marcel
  marcel_end();
  return 0;
}

void jacobi(int grid_size, int nb_workers, int nb_iters, int migration_policy) {
  marcel_t *workerid;
  marcel_attr_t attr;
  int i, err;
  int nb_cores;
  jacobi_t *args;
  struct timeval tv1, tv2;
  unsigned long us, ns;
  double maxdiff;
  marcel_memory_manager_t memory_manager;

  marcel_memory_init(&memory_manager);
  marcel_attr_init(&attr);
  local_max_diff = (double *) malloc(nb_workers * sizeof(double));
  workerid = (marcel_t *) malloc(nb_workers * sizeof(marcel_t));
  args = (jacobi_t *) malloc(nb_workers * sizeof(jacobi_t));
  nb_cores=marcel_topo_level_nbitems[MARCEL_LEVEL_CORE];

  // Initialise the grids
  err = marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_FIRST_TOUCH, 0);
  if (err < 0) perror("marcel_memory_membind");

  grid1 = (double **) marcel_memory_malloc(&memory_manager, (grid_size+2) * sizeof(double *), MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  grid2 = (double **) marcel_memory_malloc(&memory_manager, (grid_size+2) * sizeof(double *), MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  for (i = 0; i <= grid_size+1; i++) {
    grid1[i] = (double *) marcel_memory_malloc(&memory_manager, (grid_size+2) * sizeof(double), MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
    grid2[i] = (double *) marcel_memory_malloc(&memory_manager, (grid_size+2) * sizeof(double), MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
  }

  if (migration_policy == JACOBI_MIGRATE_ON_NEXT_TOUCH_USERSPACE || migration_policy == JACOBI_MIGRATE_ON_NEXT_TOUCH_KERNEL) {
    for (i = 0; i <= grid_size+1; i++) {
      marcel_memory_migrate_on_next_touch(&memory_manager, grid1[i], migration_policy);
      marcel_memory_migrate_on_next_touch(&memory_manager, grid2[i], migration_policy);
    }
  }

  // create the workers, then wait for them to finish
  gettimeofday(&tv1, NULL);
  for (i = 0; i < nb_workers; i++) {
    args[i].grid_size = grid_size;
    args[i].nb_workers = nb_workers;
    args[i].nb_iters = nb_iters;
    args[i].thread_id = i;
    args[i].strip_size = grid_size/nb_workers;
    args[i].migration_policy = migration_policy;
    marcel_attr_settopo_level(&attr, &marcel_topo_core_level[i%nb_cores]);
    marcel_create(&workerid[i], &attr, worker, (any_t) &args[i]);
  }
  for (i = 0; i < nb_workers; i++) {
    marcel_join(workerid[i], NULL);
  }
  gettimeofday(&tv2, NULL);

  for (i = 0; i < nb_workers; i++) {
    if (maxdiff < local_max_diff[i]) maxdiff = local_max_diff[i];
  }

  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  ns = us * 1000;
  ns /= nb_iters;

  // Free the memory
  for (i = 0; i <= grid_size+1; i++) {
    marcel_memory_free(&memory_manager, grid1[i]);
    marcel_memory_free(&memory_manager, grid2[i]);
  }
  marcel_memory_free(&memory_manager, grid1);
  marcel_memory_free(&memory_manager, grid2);
  free(workerid);
  free(local_max_diff);
  marcel_memory_exit(&memory_manager);

  // print the results
  marcel_printf("# grid_size\tnb_workers\tnb_iters\tmigration_policy\tmax_diff\ttime(ns)\n");
  marcel_printf("%11d %14d %13d\t%s\t%10e %10ld\n", grid_size, nb_workers, nb_iters, migration_policy_texts[migration_policy], maxdiff, ns);
}

/*
 * Each Worker computes values in one strip of the grids.
 * The main worker loop does two computations to avoid copying from
 * one grid to the other.
 */
any_t worker(any_t arg) {
  jacobi_t *mydata = (jacobi_t *) arg;
  double maxdiff, temp;
  int i, j, iters;
  int first, last;

  //  marcel_printf("Thread #%d located on node #%d\n", mydata->thread_id, marcel_current_node());

  /* determine first and last rows of my strip of the grids */
  first = mydata->thread_id*mydata->strip_size + 1;
  last = first + mydata->strip_size - 1;

  for (i = first; i <= last; i++)
    for (j = 0; j <= mydata->grid_size+1; j++) {
      grid1[i][j] = 0.0;
      grid2[i][j] = 0.0;
    }
  for (i = first; i <= last; i++) {
    grid1[i][0] = 1.0;
    grid1[i][mydata->grid_size+1] = 1.0;
    grid2[i][0] = 1.0;
    grid2[i][mydata->grid_size+1] = 1.0;
  }
  for (j = 0; j <= mydata->grid_size+1; j++) {
    grid1[0][j] = 1.0;
    grid2[0][j] = 1.0;
    grid1[mydata->grid_size+1][j] = 1.0;
    grid2[mydata->grid_size+1][j] = 1.0;
  }

  for (iters = 1; iters <= mydata->nb_iters; iters++) {
    /* update my points */
    for (i = first; i <= last; i++) {
      for (j = 1; j <= mydata->grid_size; j++) {
        grid2[i][j] = (grid1[i-1][j] + grid1[i+1][j] + grid1[i][j-1] + grid1[i][j+1]) * 0.25;
      }
    }
    barrier(mydata->nb_workers);
    /* update my points again */
    for (i = first; i <= last; i++) {
      for (j = 1; j <= mydata->grid_size; j++) {
        grid1[i][j] = (grid2[i-1][j] + grid2[i+1][j] + grid2[i][j-1] + grid2[i][j+1]) * 0.25;
      }
    }
    barrier(mydata->nb_workers);
  }
  /* compute the maximum difference in my strip and set global variable */
  maxdiff = 0.0;
  for (i = first; i <= last; i++) {
    for (j = 1; j <= mydata->grid_size; j++) {
      temp = grid1[i][j]-grid2[i][j];
      if (temp < 0)
        temp = -temp;
      if (maxdiff < temp)
        maxdiff = temp;
    }
  }
  local_max_diff[mydata->thread_id] = maxdiff;
  return 0;
}

void barrier(int nb_workers) {
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

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
