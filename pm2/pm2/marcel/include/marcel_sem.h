
#ifndef MARCEL_SEM_EST_DEF
#define MARCEL_SEM_EST_DEF

_PRIVATE_ typedef struct cell_struct {
  marcel_t task;
  struct cell_struct *next;
  boolean blocked;
} cell;

_PRIVATE_ struct semaphor_struct {
  int value;
  struct cell_struct *first,*last;
  marcel_lock_t lock;  /* For preventing concurrent access from multiple LWPs */
};

typedef struct semaphor_struct marcel_sem_t;

void marcel_sem_init(marcel_sem_t *s, int initial);
void marcel_sem_P(marcel_sem_t *s);
void marcel_sem_V(marcel_sem_t *s);
void marcel_sem_timed_P(marcel_sem_t *s, unsigned long timeout);
_PRIVATE_ void marcel_sem_VP(marcel_sem_t *s1, marcel_sem_t *s2);
_PRIVATE_ void marcel_sem_unlock_all(marcel_sem_t *s);

#endif
