
#include "marcel.h"
#include "dsm_slot_alloc.h"
#include "isoaddr.h"
#include "dsm_page_manager.h"
#include "dsm_page_size.h"

#define DEBUG_SLOT

void *dsm_slot_alloc(size_t size, size_t *granted_size, void *addr, isoaddr_attr_t *attr)
{
  int i, base;
  size_t gsize;
  void *ptr;

  LOG_IN();

#ifdef DEBUG_SLOT
  fprintf(stderr, "-----> dsm_slot_alloc called: size = %d\n", size);
#endif
  if (attr->align == 0)
    attr->align = 1;
  ptr = isoaddr_malloc(size, &gsize, addr, attr);
#ifdef DEBUG_SLOT
  fprintf(stderr, "-----> isomalloc_addr returned: ptr = %p, gsize = %d\n", ptr, gsize);
#endif
  if (!ptr)
    RAISE(STORAGE_ERROR);
  
  base = dsm_isoaddr_page_index(isoaddr_page_index(ptr));

  if (attr->atomic)
    dsm_enable_page_entry(base, dsm_self(), attr->protocol, ptr, gsize, TRUE);
  else
    /* Add an entry in the page table for each alloacted page. */
    for (i = 0; i < gsize/DSM_PAGE_SIZE; i++)
      dsm_enable_page_entry(base - i, dsm_self(), attr->protocol, (char *)ptr + i*DSM_PAGE_SIZE, DSM_PAGE_SIZE, TRUE);
  
  if (granted_size != NULL)
    *granted_size = gsize;

  LOG_OUT();
  
  return ptr;
}

void dsm_slot_free(void *addr)
{
  LOG_IN();
  LOG_OUT();
}
