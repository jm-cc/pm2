
#ifndef ISOADDR_IS_DEF
#define ISOADDR_IS_DEF

#include <sys/types.h>
#include "sys/isoaddr_const.h"

#define ALIGN_UNIT 32
#define ALIGN(x, unit) ((((unsigned long)x) + ((unit) - 1)) & ~((unit) - 1))
#define ISOMALLOC_USE_MACROS

#define THREAD_STACK_SIZE 0

/*#define DEBUG*/

void isoaddr_init(unsigned myrank, unsigned confsize);

void isoaddr_init_rpc(void);

void isoaddr_exit();

int isoaddr_addr(void *addr);

int isoaddr_page_index(void *addr);

void *isoaddr_page_addr(int index);

void *isoaddr_malloc(size_t size, size_t *granted_size, void *addr, isoaddr_attr_t *attr);

void isoaddr_free(void *addr, size_t size);

void isoaddr_add_busy_slot(void *addr);

void isoaddr_set_area_size(int pages);

void isoaddr_set_distribution(int mode, ...);

/********************************************************************************/

/* page info:  */

void isoaddr_page_set_owner(int index, node_t owner);

node_t isoaddr_page_get_owner(int index);

#define ISO_UNUSED 0
#define ISO_SHARED 1
#define ISO_PRIVATE 2
#define ISO_DEFAULT 3
#define ISO_AVAILABLE 4

void isoaddr_page_set_status(int index, int status);

int isoaddr_page_get_status(int index);

void isoaddr_page_set_master(int index, int master);

int isoaddr_page_get_master(int index);

void isoaddr_page_info_unlock(int index);

void isoaddr_page_info_unlock(int index);

#define CENTRALIZED 0
#define BLOCK       1
#define CYCLIC      2
#define CUSTOM      3

void isoaddr_page_set_distribution(int mode, ...);

void isoaddr_page_display_info();

#endif
