
#ifndef DSM_AS_POOL_IS_DEF
#define DSM_AS_POOL_IS_DEF

#include "tbx.h"

/* define as usual by default */
#define AS_STACK_SIZE SIGSTKSZ /* private : do not used directly */
#define AS_NB_AREA 64          /* private : do not used directly */

extern p_tbx_memory_t as_head; /* private : do not used directly */

static void __inline__ dsm_as_free_stack(void *ptr) __attribute__ ((unused));
static void __inline__ dsm_as_free_stack(void *ptr) {
  /* used ptr return by setup_altstack */
  tbx_free(as_head, ptr);
}

static void  __inline__ dsm_as_clean()  __attribute__ ((unused));
static void  __inline__ dsm_as_clean() {
  tbx_malloc_clean(as_head);
}

void  dsm_as_init(); /* allocate memory and setup and altstack */
void* dsm_as_setup_altstack(); /* return old *real* base address */
void  dsm_as_check_altstack();

#endif

