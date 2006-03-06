#include "pm2_common.h" /* Here! */
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int inf, sup, res;
  marcel_sem_t sem;/* Here! */
} job;

static void job_init(job *j, int inf, int sup) {
  j->inf = inf;
  j->sup = sup;
  marcel_sem_init(&j->sem, 0);/* Here! */
}

any_t sum(any_t arg) {
  job *j = (job *)arg;
  job j1, j2;

  if(j->inf == j->sup) {
    j->res = j->inf;
    marcel_sem_V(&j->sem);/* Here! */
    return NULL;
  }

  job_init(&j1, j->inf, (j->inf+j->sup)/2 );
  job_init(&j2, (j->inf+j->sup)/2+1, j->sup);

  marcel_create(NULL, NULL, sum, (any_t)&j1);/* Here! */
  marcel_create(NULL, NULL, sum, (any_t)&j2);/* Here! */
  marcel_sem_P(&j1.sem);/* Here! */
  marcel_sem_P(&j2.sem);/* Here! */

  j->res = j1.res+j2.res;
  marcel_sem_V(&j->sem);/* Here! */
  return NULL;
}

int marcel_main(int argc, char **argv) {/* Here! */
  job j;

  common_pre_init(&argc, argv, NULL);/* Here! */
  common_post_init(&argc, argv, NULL);/* Here! */

  marcel_sem_init(&j.sem, 0);/* Here! */
  j.inf = 1;
  j.sup = 1000;

  marcel_create(NULL, NULL, sum, (any_t)&j);/* Here! */
  marcel_sem_P(&j.sem);/* Here! */
  printf("Sum from 1 to %d = %d\n", j.sup, j.res);

  common_exit(NULL);/* Here! */
  return 0;
}
