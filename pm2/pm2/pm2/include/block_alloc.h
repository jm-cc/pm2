
#ifndef BLOCK_ALLOC_IS_DEF
#define BLOCK_ALLOC_IS_DEF

#include <sys/types.h>

#include "isomalloc.h"

typedef struct _block_header_t {
  size_t size;
  int magic_number;
  struct _block_header_t *next;
  struct _block_header_t *prev;
} block_header_t;


typedef struct _block_descr_t {
  slot_descr_t slot_descr;
  int magic_number;
  block_header_t *last_block_set_free;
  slot_header_t *last_slot_used;
} block_descr_t;


void block_init(unsigned myrank, unsigned confsize);

void block_init_list(block_descr_t *descr);

void *block_alloc(block_descr_t *descr, size_t size, isoaddr_attr_t *attr);

void block_free(block_descr_t *descr, void * addr);

void block_flush_list(block_descr_t *descr);

void block_exit();

void block_print_list(block_descr_t *descr);

void block_pack_all(block_descr_t *descr, int dest);

void block_unpack_all();

block_descr_t *block_merge_lists(block_descr_t *src, block_descr_t *dest);

#endif

