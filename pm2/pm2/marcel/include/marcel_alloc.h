
#ifndef MARCEL_ALLOC_EST_DEF
#define MARCEL_ALLOC_EST_DEF

#include "sys/isomalloc_archdep.h"

void marcel_slot_init(void);

void *marcel_slot_alloc(void);

void marcel_slot_free(void *addr);

void marcel_slot_exit(void);

#endif
