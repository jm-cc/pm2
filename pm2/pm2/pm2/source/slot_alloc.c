/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: slot_alloc.c,v $
Revision 1.4  2000/09/12 15:45:53  gantoniu
Made all necessary modifications to allow flavors without dsm be created and compiled.

Revision 1.3  2000/09/04 07:42:47  rnamyst
Minor modifs (removed some debug messages)

Revision 1.2  2000/07/14 16:17:15  gantoniu
Merged with branch dsm3

Revision 1.1.6.1  2000/06/13 16:44:15  gantoniu
New dsm branch.

Revision 1.1.4.1  2000/06/07 09:19:41  gantoniu
Merging new dsm with current PM2 : first try.

Revision 1.1.2.6  2000/05/10 19:38:07  gantoniu
- Implemented pm2_malloc option "ISO_DEFAULT" (= private which may become
shared); ISO_PRIVATE data can no longare be shared;

- Added an attribute to all allocation functions, to pass status, protocol
and atomic info;

- Attributes are stores in the slot header (exception: status);

- Allocation of non shared data is always atomic; when it becomes shared,
  atomicity is set according to the init attribute;

- Added TRACE options.

- Modified block_merge_lists to update the ptr to the thread slot descr.

Revision 1.1.2.5  2000/05/09 17:59:44  gantoniu
Added some optimizations in the management of the page info table, by
using master pages for non atomic slots, in order to minimize the number
of writes to the table. Tests ok with 2 protocols, single-slot and multi-slot
allocation, with and without negociation.

Revision 1.1.2.4  2000/05/07 17:26:22  gantoniu
Fixed some bugs in the new code for PRIVATE to SHARED transform.
Atomicity and protocol can be specified for private allocated
data that may become shared. Tests with and without negociation ok.

Revision 1.1.2.3  2000/05/05 14:06:24  gantoniu
Added support for status change (PRIVATE -> SHARED) and for the option
ATOMIC_SLOT:
- added a 'master' field in the page info table, set when allocating
  pages (including on negociation);
- new attribute 'atomic' for pm2_malloc; always true for private data;
  for shared data it is user-specified; multi-page objects are processed as
  as single page in consistency actions when this option is used;
- when changing a private slot to shared, all pages in the possibly
  multi-page slot change their status;
- on migration, slots change owner;
- tests without migration, with single-slot allocation data ok;

Revision 1.1.2.2  2000/04/30 18:54:07  gantoniu
Extended negociation procedure to allow multi-slot data allocation.
Negociation ok (old RPCs).

Revision 1.1.2.1  2000/04/10 18:53:56  gantoniu
Ajout modules isoaddr + slot_alloc

______________________________________________________________________________

Options: ASSERT, ISOADDR_SLOT_ALLOC_TRACE, ISOADDR_INFO_TRACE

*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>

#include "marcel.h"
#include "sys/archdep.h"
#include "madeleine.h"
#include "pm2.h"
#include "sys/isomalloc_archdep.h"
#include "magic.h"
#include "slot_alloc.h"

#define ASSERT 


void slot_slot_busy(void *addr)
{
  /*
    This function is called before a migration, to indicate that the slot
    may be asked for again before its getting liberated, due to a ping-pong
    effect.
  */
  lock_task();

  isoaddr_add_busy_slot(slot_get_header_address(addr));

  unlock_task();
}


void slot_list_busy(slot_descr_t *descr)
{
  slot_header_t *slot_ptr;

  if (descr == NULL)
    return;

  lock_task();

  slot_ptr = slot_get_first(descr);
  while (slot_ptr != NULL)
    {
      isoaddr_add_busy_slot(slot_ptr);
      slot_ptr = slot_get_next(slot_ptr);
    }

  unlock_task();
}


void slot_print_header(slot_header_t *ptr)
{
  if (ptr == NULL)return;
  fprintf(stderr,"\nslot header:\n");
  assert(ptr->magic_number == SLOT_MAGIC_NUM);
  fprintf(stderr, "  start address = %p  available size = %d  magic number = %x\n  previous slot address = %p  next slot address = %p\n", ptr, ptr->size, ptr->magic_number, ptr->prev, ptr->next);
}

void slot_print_list(slot_descr_t *descr)
{
  slot_header_t *ptr;

  if (descr == NULL) return;
  ptr = descr->slots;
  fprintf(stderr,"\nHere is the slot list:\n");
  while (ptr != NULL)
    { 
     slot_print_header(ptr);
     ptr = ptr->next;
    }
  fprintf(stderr,"last slot: %p\n", descr->last_slot);
}

void slot_init_list(slot_descr_t *descr)
{
  descr->slots = NULL;
  descr->last_slot = NULL;
#ifdef ASSERT
  descr->magic_number = SLOT_LIST_MAGIC_NUM;
#endif
#ifdef ISOADDR_SLOT_ALLOC_TRACE
  fprintf(stderr,"slot_list initialized: list address = %p\n", descr);
#endif
}


static void *slot_alloc(slot_descr_t *descr, size_t req_size, size_t *granted_size, void *addr, isoaddr_attr_t *attr)
{
  size_t overall_size;
  slot_header_t *header_ptr;

#ifdef DSM
  static isoaddr_attr_t default_isoaddr_attr = {ISO_PRIVATE, DEFAULT_DSM_PROTOCOL, 1, 32};
#else
  static isoaddr_attr_t default_isoaddr_attr = {ISO_PRIVATE, 0, 1, 32};
#endif

#ifdef ISOADDR_SLOT_ALLOC_TRACE
  fprintf(stderr,"Slot_alloc is starting, req_size = %d\n", req_size);
#endif
  if (!attr)
    attr = &default_isoaddr_attr;
  /*
    allocate a slot 
  */

  TIMING_EVENT("slot_alloc is starting: isomalloc..."); 

  header_ptr = (slot_header_t *)isoaddr_malloc(((req_size == THREAD_STACK_SIZE)?THREAD_STACK_SIZE:(req_size + SLOT_HEADER_SIZE)), &overall_size, (void *)(addr ? (char *)addr - SLOT_HEADER_SIZE : NULL), attr);
  /* 
     fill in the header 
  */
  header_ptr->size = overall_size - SLOT_HEADER_SIZE;
  if (granted_size != NULL) *granted_size = header_ptr->size;
  header_ptr->magic_number = SLOT_MAGIC_NUM;
  header_ptr->prot = attr->protocol;
  header_ptr->atomic = attr->atomic;
  /* 
     chain the slot if requested 
  */
  if(descr != NULL) {
#ifdef ASSERT
    assert(descr->magic_number == SLOT_LIST_MAGIC_NUM);
#endif
    header_ptr->next = descr->slots; 
    if(descr->slots != NULL)
      descr->slots->prev = header_ptr;
    else{
#ifdef ISOADDR_SLOT_ALLOC_TRACE
      fprintf(stderr,"First slot chained\n");
#endif
      descr->last_slot = header_ptr;
    }

    /*
      update the list head 
    */
    descr->slots = header_ptr;
    /*
      record a ptr to the thread slot descr
    */
    header_ptr->thread_slot_descr = descr;

  }
  else
    header_ptr->next = NULL;
  header_ptr->prev = NULL;
  /*
    return a pointer to the available zone 
  */

  TIMING_EVENT("Slot_alloc is ending");

#ifdef ISOADDR_SLOT_ALLOC_TRACE
  fprintf(stderr,"Slot_alloc is ending\n");
#endif

  return (void *)((char *)header_ptr + SLOT_HEADER_SIZE);
}


void *slot_general_alloc(slot_descr_t *descr, size_t req_size, size_t *granted_size, void *addr, isoaddr_attr_t *attr)
{
  void *ptr;

#ifdef ISOADDR_SLOT_ALLOC_TRACE
  fprintf(stderr,"slot_general_alloc is starting, req_size = %d, task = %p\n", req_size, marcel_self());
#endif
// slot_lock();
 ptr =  slot_alloc(descr, req_size, granted_size, addr, attr);
// slot_unlock();
 return ptr;
}


void slot_free(slot_descr_t *descr, void * addr)
{
  slot_header_t * header_ptr; 

#ifdef ISOADDR_SLOT_ALLOC_TRACE
  fprintf(stderr,"Slot_free is starting\n");
#endif
  /* 
     get the address of the slot header 
  */
  header_ptr = (slot_header_t *)((char *)addr - SLOT_HEADER_SIZE);
#ifdef ASSERT
  assert(header_ptr->magic_number == SLOT_MAGIC_NUM);
  assert(header_ptr->thread_slot_descr == descr);
#endif

  /* 
     suppress the slot from the list if necessary 
  */
  if(descr != NULL){

    if (header_ptr->prev != NULL)
      header_ptr->prev->next = header_ptr->next;
    else 
      /* 
	 the slot to suppress is the head of the list 
      */
      descr->slots = header_ptr->next;
    if (header_ptr->next != NULL)
      header_ptr->next->prev = header_ptr->prev;
    else
      { 
      /*
	the slot to suppress is the tail of the list 
      */
      if (header_ptr->prev != NULL)
	/* 
	   the slot to suppress is not the only one in the list 
	*/
	header_ptr->prev->next = NULL;

      /*
	update the address of the last slot in the list
      */
      descr->last_slot = header_ptr->prev;
      }
 }
  /* 
     set the slot free 
  */
  isoaddr_free(header_ptr, slot_get_overall_size(header_ptr));

#ifdef ISOADDR_SLOT_ALLOC_TRACE
  fprintf(stderr,"slot_free is ending\n");
  /*  
      slot_print_list(descr);
  */
#endif
// slot_unlock();

}


void slot_flush_list(slot_descr_t *descr)
{
  slot_header_t *slot_ptr, *current_slot;

#ifdef ISOADDR_SLOT_ALLOC_TRACE
  fprintf(stderr, "slot_flush_list is starting...\n");
#endif

 if (descr == NULL) return; 
 /* 
    nothing to do: the slot list is empty 
  */
#ifdef ASSERT
 assert(descr->magic_number == SLOT_LIST_MAGIC_NUM);
#endif
// slot_lock();

 slot_ptr = descr->slots;

  while (slot_ptr != NULL) 
    /* 
       for every slot in the list 
    */
    {
#ifdef ASSERT
      assert(slot_ptr->magic_number == SLOT_MAGIC_NUM);
#endif
      current_slot = slot_ptr;
      slot_ptr = slot_ptr->next;
      isoaddr_free(current_slot, slot_get_overall_size(current_slot));
    }
  descr->slots = NULL;
#ifdef ISOADDR_SLOT_ALLOC_TRACE
  fprintf(stderr, "Slot_flush_list is ending...\n");
#endif
//  slot_unlock();
}

#ifndef ISOMALLOC_USE_MACROS

slot_header_t *slot_get_first(slot_descr_t *descr)
{
  if (descr != NULL)
    {
#ifdef ASSERT
      assert(descr->magic_number == SLOT_LIST_MAGIC_NUM);
#endif
      return descr->slots;
    }
  else
    return (slot_header_t *)NULL;
}


slot_header_t *slot_get_next(slot_header_t *slot_ptr)
{
  if (slot_ptr != NULL)
    {
#ifdef ASSERT
      assert(slot_ptr->magic_number == SLOT_MAGIC_NUM);
#endif
      return slot_ptr->next;
    }
  else
    return (slot_header_t *)NULL;
}


slot_header_t *slot_get_prev(slot_header_t *slot_ptr)
{
  if (slot_ptr != NULL)
    {
#ifdef ASSERT
      assert(slot_ptr->magic_number == SLOT_MAGIC_NUM);
#endif
      return slot_ptr->prev;
    }
  else
    return (slot_header_t *)NULL;
}


void * slot_get_usable_address(slot_header_t *slot_ptr)
{
  if (slot_ptr != NULL)
    {
      return (void *)((char *)slot_ptr + SLOT_HEADER_SIZE);
    }
  else
    return (void *)NULL;
}


slot_header_t *slot_get_header_address(void *usable_address)
{
  if (usable_address != NULL)
    {
      slot_header_t * slot_ptr = (slot_header_t *)((char *)usable_address - SLOT_HEADER_SIZE);
      return slot_ptr;
    }
  else
    return (slot_header_t *)NULL;
}


size_t slot_get_usable_size(slot_header_t *slot_ptr)
{
  if (slot_ptr != NULL)
    {
#ifdef ASSERT
      assert(slot_ptr->magic_number == SLOT_MAGIC_NUM);
#endif
      return slot_ptr->size;
    }
  else
    return -1;
}


size_t slot_get_overall_size(slot_header_t *slot_ptr)
{
  if (slot_ptr != NULL)
    {
#ifdef ASSERT
      assert(slot_ptr->magic_number == SLOT_MAGIC_NUM);
#endif
      return slot_ptr->size + SLOT_HEADER_SIZE;
    }
  else
    return -1;
}


size_t slot_get_header_size()
{
  return SLOT_HEADER_SIZE;
}

#endif // ifndef ISOMALLOC_USE_MACROS

#ifdef DSM

#include "dsm_page_manager.h"

void slot_set_shared(void *addr)
{
  int i, master = isoaddr_page_get_master(isoaddr_page_index(addr));
  slot_header_t *header_ptr = (slot_header_t *)isoaddr_page_addr(master);

#ifdef ISOADDR_INFO_TRACE
   fprintf(stderr,"Slot_set_shared: unchaining... descr = %p\n",header_ptr->thread_slot_descr);
   slot_print_list(header_ptr->thread_slot_descr);
#endif
  /* 
     get the slot out of the thread list if linked 
  */
  if(header_ptr->thread_slot_descr != NULL){
    
    if (header_ptr->prev != NULL)
      header_ptr->prev->next = header_ptr->next;
    else 
      /* 
	 the slot to suppress is the head of the list 
      */
      header_ptr->thread_slot_descr->slots = header_ptr->next;
    if (header_ptr->next != NULL)
      header_ptr->next->prev = header_ptr->prev;
    else
      { 
      /*
	the slot to suppress is the tail of the list 
      */
	if (header_ptr->prev != NULL)
	/* 
	   the slot to suppress is not the only one in the list 
	   */
	  header_ptr->prev->next = NULL;
	
	/*
	  update the address of the last slot in the list
	  */
	header_ptr->thread_slot_descr->last_slot = header_ptr->prev;
      }
  }
#ifdef ISOADDR_INFO_TRACE
   fprintf(stderr,"Slot_set_shared: unchained... descr = %p\n", 
	   header_ptr->thread_slot_descr);
   slot_print_list(header_ptr->thread_slot_descr);
#endif

#ifdef ISOADDR_INFO_TRACE
   fprintf(stderr,"Slot_set_shared: master = %d\n", master);
#endif

   isoaddr_page_set_status(master, ISO_SHARED);
   i= master;

   if (header_ptr->atomic)
     {
       while (isoaddr_page_get_master(i) == master) 
	 i--;
       
       dsm_enable_page_entry(dsm_isoaddr_page_index(master), pm2_self(), 
			     header_ptr->prot, isoaddr_page_addr(master), 
			     (master - i)*DSM_PAGE_SIZE, FALSE);
     }
   else
     {
     while (isoaddr_page_get_master(i) == master) 
       {
	 dsm_enable_page_entry(dsm_isoaddr_page_index(i), pm2_self(), 
			       header_ptr->prot, isoaddr_page_addr(i), 
			       DSM_PAGE_SIZE, FALSE);
	 i--;
       }
     }


}
#endif //DSM


