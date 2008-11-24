
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>

#include "marcel.h"
#include "asm/marcel_archdep.h"
#include "madeleine.h"
#include "pm2.h"
#include "sys/isomalloc_archdep.h"
#include "magic.h"
#include "slot_alloc.h"



void slot_slot_busy(void *addr)
{
  /*
    This function is called before a migration, to indicate that the slot
    may be asked for again before its getting liberated, due to a ping-pong
    effect.
  */
  ma_preempt_disable();

  isoaddr_add_busy_slot(slot_get_header_address(addr));

  ma_preempt_enable();
}


void slot_list_busy(slot_descr_t *descr)
{
  slot_header_t *slot_ptr;

  if (descr == NULL)
    return;

  ma_preempt_disable();

  slot_ptr = slot_get_first(descr);
  while (slot_ptr != NULL)
    {
      isoaddr_add_busy_slot(slot_ptr);
      slot_ptr = slot_get_next(slot_ptr);
    }

  ma_preempt_enable();
}


void slot_print_header(slot_header_t *ptr)
{
  if (ptr == NULL)return;
  fprintf(stderr,"\nslot header:\n");
#ifdef ASSERT
  assert(ptr->magic_number == SLOT_MAGIC_NUM);
  fprintf(stderr, "  start address = %p  available size = %d  magic number = %x\n  previous slot address = %p  next slot address = %p\n", ptr, ptr->size, ptr->magic_number, ptr->prev, ptr->next);
#endif
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
  LOG_IN();

  descr->slots = NULL;
  descr->last_slot = NULL;
#ifdef ASSERT
  descr->magic_number = SLOT_LIST_MAGIC_NUM;
#endif
#ifdef ISOADDR_SLOT_ALLOC_TRACE
  fprintf(stderr,"slot_list initialized: list address = %p\n", descr);
#endif
  
  LOG_OUT();
}


static void *slot_alloc(slot_descr_t *descr, size_t req_size, size_t *granted_size, void *addr, isoaddr_attr_t *attr)
{
  size_t overall_size;
  slot_header_t *header_ptr;

  static isoaddr_attr_t default_isoaddr_attr = {ISO_PRIVATE, 1, 32, 0};

  LOG_IN();

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
#ifdef ASSERT
  header_ptr->magic_number = SLOT_MAGIC_NUM;
#endif
  header_ptr->atomic = attr->atomic;
  header_ptr->special = attr->special; 
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

  LOG_OUT();

  return (void *)((char *)header_ptr + SLOT_HEADER_SIZE);
}


void *slot_general_alloc(slot_descr_t *descr, size_t req_size, size_t *granted_size, void *addr, isoaddr_attr_t *attr)
{
  void *ptr;

  LOG_IN();

#ifdef ISOADDR_SLOT_ALLOC_TRACE
  fprintf(stderr,"slot_general_alloc is starting, req_size = %d, task = %p\n", req_size, marcel_self());
#endif
// slot_lock();
 ptr =  slot_alloc(descr, req_size, granted_size, addr, attr);
// slot_unlock();

  LOG_OUT();
  return ptr;
}


void slot_free(slot_descr_t *descr, void * addr)
{
  slot_header_t * header_ptr; 

 LOG_IN();

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

  LOG_OUT();
}


void slot_flush_list(slot_descr_t *descr)
{
  slot_header_t *slot_ptr, *current_slot;

  LOG_IN();

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

  LOG_OUT();
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

slot_header_t *slot_detach(void *addr)
{
  int master = isoaddr_page_get_master(isoaddr_page_index(addr));
  slot_header_t *header_ptr = (slot_header_t *)isoaddr_page_addr(master);

#ifdef ISOADDR_INFO_TRACE
   fprintf(stderr,"slot_detach: unchaining... descr = %p\n",header_ptr->thread_slot_descr);
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
   return header_ptr;
}


void slot_attach(slot_descr_t *descr, slot_header_t *header_ptr)
{
  /* 
     fill in the header 
  */
#ifdef ASSERT
  header_ptr->magic_number = SLOT_MAGIC_NUM;
#endif
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
    MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
  header_ptr->prev = NULL;
}


