
#include <sys/types.h>
#include "sys/isoaddr_const.h"

void * dsm_slot_alloc(size_t size, size_t *granted_size, void *addr, isoaddr_attr_t *attr);

void dsm_slot_free(void *addr);

