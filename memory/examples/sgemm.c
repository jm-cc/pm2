#include <cblas.h>
#include <stdio.h>
#include <stdlib.h>
#include "mm_mami.h"

#if defined(MM_MAMI_ENABLED)

#define SGEMM_MIGRATE_ON_NEXT_TOUCH_USERSPACE  0
#define SGEMM_MIGRATE_ON_NEXT_TOUCH_KERNEL     1
#define SGEMM_MIGRATE_ON_FIRST_TOUCH           2

#define SGEMM_INIT_GLOBALE 0
#define SGEMM_INIT_LOCALE  1

float **matA, **matB, **matC;
void test_sgemm(int matrix_size, int migration_policy, int initialisation_policy, unsigned long *ns);
any_t sgemm(any_t arg);

typedef struct sgemm_s {
  int thread_id;
  int matrix_size;
  int initialisation_policy;
} sgemm_t;

int marcel_main(int argc, char **argv) {
  int matrix_size, migration_policy, initialisation_policy;
  unsigned long ns;
  char *migration_policy_texts[3] = {"NEXT_TOUCH_USERSPACE",
                                     "NEXT_TOUCH_KERNEL   ",
                                     "FIRST_TOUCH         "};
  char *initialisation_policy_texts[3] = {"GLOBAL_INITIALISATION",
                                          "LOCAL_INITIALISATION "};

  marcel_init(&argc, argv);

  if (argc != 4) {
    marcel_printf("Error. Syntax: pm2-load sgemm <matrix_size> <migration_policy> <initialisation_policy>\n");
    marcel_end();
    return -1;
  }
  matrix_size = atoi(argv[1]);
  migration_policy = atoi(argv[2]);
  initialisation_policy = atoi(argv[3]);

  test_sgemm(matrix_size, migration_policy, initialisation_policy, &ns);
  marcel_printf("%10d\t%s\t%s\t%20ld\n", matrix_size, migration_policy_texts[migration_policy], initialisation_policy_texts[initialisation_policy], ns);

  // Finish marcel
  marcel_end();
  return 0;
}

void test_sgemm(int matrix_size, int migration_policy, int initialisation_policy, unsigned long *ns) {
  marcel_t *worker_id;
  int nb_workers, nb_cores, err;
  marcel_attr_t attr;
  marcel_memory_manager_t memory_manager;
  unsigned k;
  sgemm_t *args;
  struct timeval tv1, tv2;
  unsigned long us;
  unsigned i,j;

  marcel_memory_init(&memory_manager);
  nb_workers = marcel_topo_level_nbitems[MARCEL_LEVEL_CORE];

  err = marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_FIRST_TOUCH, 0);
  if (err < 0) perror("marcel_memory_membind");

  gettimeofday(&tv1, NULL);
  /* Create and initialise the matrices */
  matA = malloc(nb_workers * sizeof(float *));
  matB = malloc(nb_workers * sizeof(float *));
  matC = malloc(nb_workers * sizeof(float *));
  for(k=0 ; k<nb_workers ; k++) {
    matA[k] = marcel_memory_malloc(&memory_manager, matrix_size*matrix_size*sizeof(float), MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
    matB[k] = marcel_memory_malloc(&memory_manager, matrix_size*matrix_size*sizeof(float), MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
    matC[k] = marcel_memory_malloc(&memory_manager, matrix_size*matrix_size*sizeof(float), MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);

    if (initialisation_policy == SGEMM_INIT_GLOBALE) {
      for (j = 0; j < matrix_size; j++) {
        for (i = 0; i < matrix_size; i++) {
          matA[k][i+matrix_size*j] = 1.0f;
          matB[k][i+matrix_size*j] = 1.0f;
          matC[k][i+matrix_size*j] = 1.0;
        }
      }
    }

    if (migration_policy == SGEMM_MIGRATE_ON_NEXT_TOUCH_USERSPACE || migration_policy == SGEMM_MIGRATE_ON_NEXT_TOUCH_KERNEL) {
      memory_manager.kernel_nexttouch_migration = (migration_policy == SGEMM_MIGRATE_ON_NEXT_TOUCH_KERNEL);
      marcel_memory_migrate_on_next_touch(&memory_manager, matA[k]);
      marcel_memory_migrate_on_next_touch(&memory_manager, matB[k]);
      marcel_memory_migrate_on_next_touch(&memory_manager, matC[k]);
    }
  }

  worker_id = (marcel_t *) malloc(nb_workers * sizeof(marcel_t));
  marcel_attr_init(&attr);

  nb_cores=marcel_topo_level_nbitems[MARCEL_LEVEL_CORE];
  args = (sgemm_t *) malloc(nb_workers * sizeof(sgemm_t));
  for (k=0; k<nb_workers; k++) {
    args[k].thread_id = k;
    args[k].matrix_size = matrix_size;
    args[k].initialisation_policy = initialisation_policy;
    marcel_attr_settopo_level(&attr, &marcel_topo_core_level[k%nb_cores]);
    marcel_create(&worker_id[k], &attr, sgemm, (any_t) &args[k]);
  }
  for (k=0; k<nb_workers; k++) {
    marcel_join(worker_id[k], NULL);
  }
  gettimeofday(&tv2, NULL);
  for(k=0 ; k<nb_workers ; k++) {
    marcel_memory_free(&memory_manager, matA[k]);
    marcel_memory_free(&memory_manager, matB[k]);
    marcel_memory_free(&memory_manager, matC[k]);
  }
  marcel_memory_exit(&memory_manager);
  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  *ns = us * 1000;
}

any_t sgemm(any_t arg) {
  sgemm_t *mydata = (sgemm_t *) arg;
  unsigned i, j;

  if (mydata->initialisation_policy == SGEMM_INIT_LOCALE) {
    for (j = 0; j < mydata->matrix_size; j++) {
      for (i = 0; i < mydata->matrix_size; i++) {
        matA[mydata->thread_id][i+mydata->matrix_size*j] = 1.0f;
        matB[mydata->thread_id][i+mydata->matrix_size*j] = 1.0f;
        matC[mydata->thread_id][i+mydata->matrix_size*j] = 1.0;
      }
    }
  }

  cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
              mydata->matrix_size, mydata->matrix_size, mydata->matrix_size,
              1.0f,
              matA[mydata->thread_id], mydata->matrix_size,
              matB[mydata->thread_id], mydata->matrix_size,
              0.0f,
              matC[mydata->thread_id], mydata->matrix_size);

  //  marcel_printf("matA[%d][0] = %f\n", mydata->thread_id, matA[mydata->thread_id][0+0]);
  //  marcel_printf("matB[%d][0] = %f\n", mydata->thread_id, matB[mydata->thread_id][0+0]);
  //  marcel_printf("matC[%d][0] = %f\n", mydata->thread_id, matC[mydata->thread_id][0+0]);
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
  return 0;
}
#endif
