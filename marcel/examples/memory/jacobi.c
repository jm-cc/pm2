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

#define LOOPS 2

typedef struct jacobi_s {
  int thread_id;
  int grid_size;
  int nb_workers;
  int nb_iters;
  int strip_size;
  int migration_policy;
} jacobi_t;

void compare_jacobi(int grid_size, int nb_workers, int nb_iters, int migration_policy, FILE *f);
void jacobi(int grid_size, int nb_workers, int nb_iters, int migration_policy, double *maxdiff, unsigned long *ns);
any_t worker(any_t arg);
void initialize_grids(int grid_size, int migration_policy);
void barrier(int nb_workers);

marcel_mutex_t mutex;  /* mutex semaphore for the barrier */
marcel_cond_t go;      /* condition variable for leaving */
int nb_arrived = 0;    /* count of the number who have arrived */

marcel_memory_manager_t memory_manager;

double *local_max_diff;
double **grid1, **grid2;

int marcel_main(int argc, char *argv[]) {
  int grid_size, nb_workers, nb_iters, migration_policy;
  FILE *out;

  marcel_init(&argc, argv);

  /* initialize mutex and condition variable */
  marcel_mutex_init(&mutex, NULL);
  marcel_cond_init(&go, NULL);

  out = fopen("output", "w");
  if (!out) {
    printf("Error when opening file <output>\n");
    return -1;
  }

  marcel_printf("# grid_size\tnb_workers\tnb_iters\tmax_diff\ttime_migrate_on_next_touch_userspace(ns)\ttime_migrate_on_next_touch_kernel(ns)\ttime_migrate_on_first_touch(ns)\n");
  fprintf(out, "# grid_size\tnb_workers\tnb_iters\tmax_diff\ttime_migrate_on_next_touch_userspace(ns)\ttime_migrate_on_next_touch_kernel(ns)\ttime_migrate_on_first_touch(ns)\n");

  migration_policy = -1;
  if (argc == 4 || argc == 5) {
    grid_size = atoi(argv[1]);
    nb_workers = atoi(argv[2]);
    nb_iters = atoi(argv[3]);
    if (argc == 5) migration_policy = atoi(argv[4]);

    compare_jacobi(grid_size, nb_workers, nb_iters, migration_policy, out);
  }
  else {
    nb_workers = marcel_topo_level_nbitems[MARCEL_LEVEL_CORE];
    // small matrix
    for(nb_iters=10 ; nb_iters<=100 ; nb_iters+=10) {
      //compare_jacobi(128, nb_workers, nb_iters, out);
      compare_jacobi(4000, nb_workers, nb_iters, migration_policy, out);
    }
    // medium matrix
    for(nb_iters=10 ; nb_iters<=100 ; nb_iters+=10) {
      //compare_jacobi(1024, nb_workers, nb_iters, out);
      compare_jacobi(8000, nb_workers, nb_iters, migration_policy, out);
    }
    // big matrix
    for(nb_iters=10 ; nb_iters<=100 ; nb_iters+=10) {
      //compare_jacobi(4096, nb_workers, nb_iters, out);
      compare_jacobi(16000, nb_workers, nb_iters, migration_policy, out);
    }
  }

  fclose(out);
  // Finish marcel
  marcel_end();
  return 0;
}

void compare_jacobi(int grid_size, int nb_workers, int nb_iters, int migration_policy, FILE *f) {
  double maxdiff_migrate_on_next_touch_userspace=0.0, maxdiff_migrate_on_next_touch_kernel=0.0, maxdiff_migrate_on_first_touch=0.0;
  unsigned long time_migrate_on_next_touch_userspace[LOOPS], time_migrate_on_next_touch_kernel[LOOPS], time_migrate_on_first_touch[LOOPS];
  unsigned long mean_migrate_on_next_touch_userspace=0, mean_migrate_on_next_touch_kernel=0, mean_migrate_on_first_touch=0;
  int i;

  for(i=0 ; i<LOOPS ; i++) {
    if (migration_policy == -1 || migration_policy == JACOBI_MIGRATE_ON_NEXT_TOUCH_USERSPACE) {
      jacobi(grid_size, nb_workers, nb_iters, JACOBI_MIGRATE_ON_NEXT_TOUCH_USERSPACE,
             &maxdiff_migrate_on_next_touch_userspace, &time_migrate_on_next_touch_userspace[i]);
      mean_migrate_on_next_touch_userspace += time_migrate_on_next_touch_userspace[i];
    }
    if (migration_policy == -1 || migration_policy == JACOBI_MIGRATE_ON_NEXT_TOUCH_KERNEL) {
      jacobi(grid_size, nb_workers, nb_iters, JACOBI_MIGRATE_ON_NEXT_TOUCH_KERNEL,
             &maxdiff_migrate_on_next_touch_kernel, &time_migrate_on_next_touch_kernel[i]);
      mean_migrate_on_next_touch_kernel += time_migrate_on_next_touch_kernel[i];
    }
    if (migration_policy == -1 || migration_policy == JACOBI_MIGRATE_ON_FIRST_TOUCH) {
      jacobi(grid_size, nb_workers, nb_iters, JACOBI_MIGRATE_ON_FIRST_TOUCH,
             &maxdiff_migrate_on_first_touch, &time_migrate_on_first_touch[i]);
      mean_migrate_on_first_touch += time_migrate_on_first_touch[i];
    }
  }

  mean_migrate_on_next_touch_userspace /= LOOPS;
  mean_migrate_on_next_touch_kernel /= LOOPS;
  mean_migrate_on_first_touch /= LOOPS;

  /* print the results */
  marcel_printf("%11d %14d %13d\t%e\t%ld\t\t\t%ld\t\t\t\t%ld\n", grid_size, nb_workers, nb_iters,
                maxdiff_migrate_on_next_touch_userspace, mean_migrate_on_next_touch_userspace,
                mean_migrate_on_next_touch_kernel, mean_migrate_on_first_touch);
  fprintf(f, "%11d %14d %13d\t%e\t%ld\t\t\t%ld\t\t\t\t%ld\n", grid_size, nb_workers, nb_iters,
          maxdiff_migrate_on_next_touch_userspace, mean_migrate_on_next_touch_userspace,
          mean_migrate_on_next_touch_kernel, mean_migrate_on_first_touch);
  fflush(f);
}

void jacobi(int grid_size, int nb_workers, int nb_iters, int migration_policy, double *maxdiff, unsigned long *ns) {
  marcel_t *workerid;
  marcel_attr_t attr;
  int i, nb_cores;
  jacobi_t *args;
  struct timeval tv1, tv2;
  unsigned long us;

  marcel_memory_init(&memory_manager);
  marcel_attr_init(&attr);
  local_max_diff = (double *) malloc(nb_workers * sizeof(double));
  workerid = (marcel_t *) malloc(nb_workers * sizeof(marcel_t));
  args = (jacobi_t *) malloc(nb_workers * sizeof(jacobi_t));
  initialize_grids(grid_size, migration_policy);
  nb_cores=marcel_topo_level_nbitems[MARCEL_LEVEL_CORE];

  /* create the workers, then wait for them to finish */
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
    if (*maxdiff < local_max_diff[i]) *maxdiff = local_max_diff[i];
  }

  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  *ns = us * 1000;
  *ns /= nb_iters;

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

#if 0
  if (mydata->migration_policy == JACOBI_MIGRATE_ON_NEXT_TOUCH) {
    int err;
    for (i = first; i <= last; i++) {
      err = marcel_memory_check_pages_location(&memory_manager, grid1[i], (mydata->grid_size+2) * sizeof(double), marcel_current_node());
      if (err < 0) perror("marcel_memory_check_pages_location");
      err = marcel_memory_check_pages_location(&memory_manager, grid2[i], (mydata->grid_size+2) * sizeof(double), marcel_current_node());
      if (err < 0) perror("marcel_memory_check_pages_location");
    }
  }
#endif

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

/*
 * Initialize the grids (grid1 and grid2)
 * set boundaries to 1.0 and interior points to 0.0
 */
void initialize_grids(int grid_size, int migration_policy) {
  int i, j, err;

  //  if (migration_policy == JACOBI_MIGRATE_ON_FIRST_TOUCH) {
    err = marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_FIRST_TOUCH, 0);
    if (err < 0) perror("marcel_memory_membind");
    //  }

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
    //  marcel_memory_migrate_on_next_touch(&memory_manager, grid1);
    //marcel_memory_migrate_on_next_touch(&memory_manager, grid2);
  }
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
