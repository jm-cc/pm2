#include "marcel.h"

#if defined(MARCEL_MAMI_ENABLED)

#include <cblas.h>
#include <stdio.h>
#include <stdlib.h>

#define SAXPY_MIGRATE_ON_NEXT_TOUCH_USERSPACE  0
#define SAXPY_MIGRATE_ON_NEXT_TOUCH_KERNEL     1
#define SAXPY_MIGRATE_ON_FIRST_TOUCH           2

#define SAXPY_INIT_GLOBALE 0
#define SAXPY_INIT_LOCALE  1

float **vectorA, **vectorB;
void test_saxpy(int vector_size, int migration_policy, int initialisation_policy, unsigned long *ns);
any_t saxpy(any_t arg);

typedef struct saxpy_s {
  int thread_id;
  int vector_size;
  int initialisation_policy;
} saxpy_t;

int marcel_main(int argc, char **argv) {
  int vector_size, migration_policy, initialisation_policy;
  unsigned long ns;
  char *migration_policy_texts[3] = {"NEXT_TOUCH_USERSPACE",
                                     "NEXT_TOUCH_KERNEL   ",
                                     "FIRST_TOUCH         "};
  char *initialisation_policy_texts[3] = {"GLOBAL_INITIALISATION",
                                          "LOCAL_INITIALISATION "};

  marcel_init(&argc, argv);

  if (argc != 4) {
    marcel_printf("Error. Syntax: pm2-load saxpy <vector_size> <migration_policy> <initialisation_policy>\n");
    marcel_end();
    return -1;
  }
  vector_size = atoi(argv[1]);
  migration_policy = atoi(argv[2]);
  initialisation_policy = atoi(argv[3]);

  test_saxpy(vector_size, migration_policy, initialisation_policy, &ns);
  marcel_printf("%10d\t%s\t%s\t%20ld\n", vector_size, migration_policy_texts[migration_policy], initialisation_policy_texts[initialisation_policy], ns);

  // Finish marcel
  marcel_end();
  return 0;
}

void test_saxpy(int vector_size, int migration_policy, int initialisation_policy, unsigned long *ns) {
  marcel_t *worker_id;
  int nb_workers, nb_cores, err;
  marcel_attr_t attr;
  marcel_memory_manager_t memory_manager;
  unsigned k;
  saxpy_t *args;
  struct timeval tv1, tv2;
  unsigned long us;
  unsigned i;

  marcel_memory_init(&memory_manager);
  nb_workers = marcel_topo_level_nbitems[MARCEL_LEVEL_CORE];

  err = marcel_memory_membind(&memory_manager, MARCEL_MEMORY_MEMBIND_POLICY_FIRST_TOUCH, 0);
  if (err < 0) perror("marcel_memory_membind");

  gettimeofday(&tv1, NULL);
  /* Create and initialise the vectorrices */
  vectorA = malloc(nb_workers * sizeof(float *));
  vectorB = malloc(nb_workers * sizeof(float *));
  for(k=0 ; k<nb_workers ; k++) {
    vectorA[k] = marcel_memory_malloc(&memory_manager, vector_size*sizeof(float), MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);
    vectorB[k] = marcel_memory_malloc(&memory_manager, vector_size*sizeof(float), MARCEL_MEMORY_MEMBIND_POLICY_DEFAULT, 0);

    if (initialisation_policy == SAXPY_INIT_GLOBALE) {
      for (i = 0; i < vector_size; i++) {
        vectorA[k][i] = 1.0f;
        vectorB[k][i] = 1.0f;
      }
    }

    if (migration_policy == SAXPY_MIGRATE_ON_NEXT_TOUCH_USERSPACE || migration_policy == SAXPY_MIGRATE_ON_NEXT_TOUCH_KERNEL) {
      memory_manager.kernel_nexttouch_migration = (migration_policy == SAXPY_MIGRATE_ON_NEXT_TOUCH_KERNEL);
      marcel_memory_migrate_on_next_touch(&memory_manager, vectorA[k]);
      marcel_memory_migrate_on_next_touch(&memory_manager, vectorB[k]);
    }
  }

  worker_id = (marcel_t *) malloc(nb_workers * sizeof(marcel_t));
  marcel_attr_init(&attr);

  nb_cores=marcel_topo_level_nbitems[MARCEL_LEVEL_CORE];
  args = (saxpy_t *) malloc(nb_workers * sizeof(saxpy_t));
  for (k=0; k<nb_workers; k++) {
    args[k].thread_id = k;
    args[k].vector_size = vector_size;
    args[k].initialisation_policy = initialisation_policy;
    marcel_attr_settopo_level(&attr, &marcel_topo_core_level[k%nb_cores]);
    marcel_create(&worker_id[k], &attr, saxpy, (any_t) &args[k]);
  }
  for (k=0; k<nb_workers; k++) {
    marcel_join(worker_id[k], NULL);
  }
  gettimeofday(&tv2, NULL);
  for(k=0 ; k<nb_workers ; k++) {
    marcel_memory_free(&memory_manager, vectorA[k]);
    marcel_memory_free(&memory_manager, vectorB[k]);
  }
  marcel_memory_exit(&memory_manager);
  us = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  *ns = us * 1000;
}

any_t saxpy(any_t arg) {
  saxpy_t *mydata = (saxpy_t *) arg;
  unsigned i;

  if (mydata->initialisation_policy == SAXPY_INIT_LOCALE) {
    for (i = 0; i < mydata->vector_size; i++) {
      vectorA[mydata->thread_id][i] = 1.0f;
      vectorB[mydata->thread_id][i] = 1.0f;
    }
  }

  cblas_saxpy(mydata->vector_size, 1.0f,
              vectorA[mydata->thread_id], 1,
              vectorB[mydata->thread_id], 1);

  //  marcel_printf("vectorA[%d][0] = %f\n", mydata->thread_id, vectorA[mydata->thread_id][0+0]);
  //  marcel_printf("vectorB[%d][0] = %f\n", mydata->thread_id, vectorB[mydata->thread_id][0+0]);
}

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
  return 0;
}
#endif