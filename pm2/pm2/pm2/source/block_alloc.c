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
*/

#ifndef DEBUG
#define NDEBUG 1
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>

#include <marcel.h>
#include <sys/archdep.h>
#include  <madeleine.h>

#include <isomalloc_timing.h>

#include "block_alloc.h"
#include "magic.h"

/*marcel_mutex_t block_alloc_mutex;

#define block_lock  marcel_mutex_lock(&block_alloc_mutex)
#define block_unlock marcel_mutex_unlock(&block_alloc_mutex)
*/
#define BLOCK_ALIGN_UNIT 32
#define BLOCK_HEADER_SIZE (ALIGN(sizeof(block_header_t), BLOCK_ALIGN_UNIT))
#define BLOCK_ALIGN(s) (ALIGN(s,BLOCK_ALIGN_UNIT))
#define MIN_BLOCK_SIZE 32

/*#define DEBUG*/
void block_init(unsigned myrank, unsigned confsize)
{
  slot_init(myrank,confsize);
  /*  marcel_mutex_init(&block_alloc_mutex, NULL);*/
}


void block_init_list(block_descr_t *descr)
{
  slot_init_list(&descr->slot_descr);
#ifdef DEBUG
  descr->magic_number = BLOCK_LIST_MAGIC_NUM;
#endif
  descr->last_block_set_free = (block_header_t *)NULL;
  descr->last_slot_used = (slot_header_t *)NULL;
}

#define BLOCK_ALLOC_USE_MACROS

#ifdef BLOCK_ALLOC_USE_MACROS

#define block_get_next(b) ((b)->next)

#define block_get_prev(b) ((b)->prev)

#define block_get_usable_address(b) ((void *)((char *)(b) + BLOCK_HEADER_SIZE))

#define block_get_header_address(usable_address) ((block_header_t *)((char *)(usable_address) - BLOCK_HEADER_SIZE))

#define block_get_usable_size(b) ((b)->size)

#define block_get_overall_size(b) ((b)->size + BLOCK_HEADER_SIZE)

#define block_get_header_size() BLOCK_HEADER_SIZE

#else

static block_header_t *block_get_next(block_header_t *block_ptr)
{
  if (block_ptr != NULL)
    {
#ifdef DEBUG
      assert(block_ptr->magic_number == BLOCK_MAGIC_NUM);
#endif
      return block_ptr->next;
    }
  else
    return (block_header_t *)NULL;
}


static block_header_t *block_get_prev(block_header_t *block_ptr)
{
  if (block_ptr != NULL)
    {
#ifdef DEBUG
      assert(block_ptr->magic_number == BLOCK_MAGIC_NUM);
#endif
      return block_ptr->prev;
    }
  else
    return (block_header_t *)NULL;
}


static void * block_get_usable_address(block_header_t *block_ptr)
{
  if (block_ptr != NULL)
    {
      return (void *)((char *)block_ptr + BLOCK_HEADER_SIZE);
    }
  else
    return (void *)NULL;
}


static block_header_t *block_get_header_address(void *usable_address)
{
  if (usable_address != NULL)
    {
      block_header_t * block_ptr = (block_header_t *)((char *)usable_address - BLOCK_HEADER_SIZE);
      return block_ptr;
    }
  else
    return (block_header_t *)NULL;
}


static size_t block_get_usable_size(block_header_t *block_ptr)
{
  if (block_ptr != NULL)
    {
#ifdef DEBUG
      assert(block_ptr->magic_number == BLOCK_MAGIC_NUM);
#endif
      return block_ptr->size;
    }
  else
    return -1;
}


static size_t block_get_overall_size(block_header_t *block_ptr)
{
  if (block_ptr != NULL)
    {
#ifdef DEBUG
      assert(block_ptr->magic_number == BLOCK_MAGIC_NUM);
#endif
      return block_ptr->size + BLOCK_HEADER_SIZE;
    }
  else
    return -1;
}

#endif

static void block_print_header(block_header_t *ptr)
{
  if (ptr == NULL)return;
  fprintf(stderr,"\nblock header:\n");
  assert(ptr->magic_number == BLOCK_MAGIC_NUM);
  fprintf(stderr, "  start address = %p  available size = %d  magic number = %x\n  previous block address = %p  next block address = %p\n", ptr, ptr->size, ptr->magic_number, ptr->prev, ptr->next);
}


static void block_print_slot(slot_header_t *slot_ptr)
{
  block_header_t *block_ptr = (block_header_t *)slot_get_usable_address(slot_ptr);
  
  fprintf(stderr, "\nBlock list for slot %p: \n", slot_ptr);
  while(block_ptr != NULL)
    {
      block_print_header(block_ptr);
      block_ptr = block_get_next(block_ptr);
    }
}


void block_print_list(block_descr_t *descr)
{
  slot_header_t *slot_ptr = slot_get_first(&descr->slot_descr);
  fprintf(stderr,"\nHere is the whole block list:\n");
  while(slot_ptr != NULL)
    {
      block_print_slot(slot_ptr);
      slot_ptr = slot_get_next(slot_ptr);
    }
}


static void *block_create(block_header_t *block_ptr, size_t size)
{
  void * addr;
  size_t available_size;
  block_header_t *new_block_ptr;

TIMING_EVENT("block_create:start");
  size = BLOCK_ALIGN(size);
  if (block_ptr == NULL)
    {
/*       block_unlock */; 
      return (void *)NULL;
    }
#ifdef DEBUG
  assert(block_ptr->magic_number == BLOCK_MAGIC_NUM);
#endif
  available_size = block_get_usable_size(block_ptr);
  addr = block_get_usable_address(block_ptr);
  if (available_size > BLOCK_ALIGN(size) + MIN_BLOCK_SIZE)
    {
      /*
	Create a new block.
      */
      new_block_ptr = (block_header_t *)((char *)addr + size);
TIMING_EVENT("0");
      new_block_ptr ->size = available_size - BLOCK_HEADER_SIZE - size;
TIMING_EVENT("1");
#ifdef DEBUG
      new_block_ptr-> magic_number = BLOCK_MAGIC_NUM;
#endif
      new_block_ptr-> next = block_get_next(block_ptr);
      new_block_ptr-> prev = block_ptr;
      if (block_get_next(block_ptr) != NULL)
	new_block_ptr->next->prev = new_block_ptr;
      block_ptr->next = new_block_ptr;
    }
  /*
    Else, return the whole block.
  */
  block_ptr->size = 0;

/*   block_unlock; */
TIMING_EVENT("block_create:end");
  return addr;
}
    

static void *block_alloc_in_new_slot(block_descr_t *descr, size_t req_size) 
{
  size_t available_size;
  block_header_t *block_ptr, *last_block_ptr;
  
#ifdef DEBUG
  fprintf(stderr, "Allocating new slot...\n");
#endif
  /*
    Allocate a new slot.
  */
  block_ptr = (block_header_t *)slot_general_alloc(&descr->slot_descr, BLOCK_ALIGN(req_size) + 2 * BLOCK_HEADER_SIZE, &available_size, NULL);
  /*
    Initialize the slot by creating an empty block at the end of the slot
    and another block containing the remaining space in the slot.
  */
  block_ptr->size = available_size - 2 * BLOCK_HEADER_SIZE;
#ifdef DEBUG
  block_ptr->magic_number= BLOCK_MAGIC_NUM;
#endif
  last_block_ptr = (block_header_t *)((char *)block_ptr + block_get_overall_size(block_ptr));
  last_block_ptr->size = 0;
  block_ptr->next = last_block_ptr;
  block_ptr->prev = NULL;
#ifdef DEBUG
  last_block_ptr->magic_number= BLOCK_MAGIC_NUM;
#endif
  last_block_ptr->next = NULL;
  last_block_ptr->prev = block_ptr;
  descr->last_slot_used = slot_get_header_address((void *)block_ptr);
TIMING_EVENT("block_alloc_in_new_slot:before block_create");  
  return block_create(block_ptr, req_size);
}


void *block_search_hole_in_slot_sublist(block_descr_t *descr, slot_header_t *first, slot_header_t *last, size_t size)
{
slot_header_t *slot_ptr;
block_header_t *block_ptr;
TIMING_EVENT("search_hole:start");
  slot_ptr = first; 
  while (slot_ptr != last)
    {
      block_ptr = (block_header_t *)slot_get_usable_address(slot_ptr);
      /*
	Look for an appropriate hole in the current slot.
      */
      while ((block_ptr != NULL) && (block_get_usable_size(block_ptr) < size))
	block_ptr = block_get_next(block_ptr);
      if (block_ptr !=NULL)
	/*
	  A suitable hole has been found.
	*/
	{
	  descr->last_slot_used = slot_ptr;
	  TIMING_EVENT("search_hole:end");
	  return block_create(block_ptr, size);
	}
      else
	/*
	  Go on to the next slot.
	*/
	slot_ptr = slot_get_next(slot_ptr);
    }

/*   block_unlock; */
TIMING_EVENT("search_hole:end");
  return (void *)NULL;
}


void *block_alloc(block_descr_t *descr, size_t size)
{
slot_header_t *slot_ptr;
block_header_t *last_block_set_free;
void * addr;

TIMING_EVENT("block_alloc is starting");
  if (descr == NULL)
    return (void *)NULL;

  /*  block_lock;*/

  /*
    If the last block set free is large enough, use it.
  */
  last_block_set_free = descr->last_block_set_free;
  if (last_block_set_free != NULL && block_get_usable_size(last_block_set_free) >= size)
      return block_create(last_block_set_free, size);

  /*
    Look for a hole large enough in the slots already allocated.
    Begin searching in the slot where the last allocation took place.
  */
   if (descr->last_slot_used != NULL)
     {
       addr = block_search_hole_in_slot_sublist(descr, descr->last_slot_used, NULL, size);
#ifdef DEBUG
       fprintf(stderr, "Starting search at %p... I got %p\n", descr->last_slot_used, addr);
#endif
       if (addr != NULL)
	   return addr;
     }
   /*
     Search for a hole in the remaining slots, if not found.
   */
   if ((slot_ptr = slot_get_first(&descr->slot_descr)) != NULL && slot_ptr != descr->last_slot_used )
       { 
	 addr = block_search_hole_in_slot_sublist(descr, slot_ptr, descr->last_slot_used, size);
#ifdef DEBUG
	 fprintf(stderr, "Searching from %p, got %p\n", slot_ptr, addr);
#endif		
	 if (addr != NULL)
	     return addr;
       }
  /*
    No suitable hole has been found, so a new slot gets allocated.
  */
    return block_alloc_in_new_slot(descr, size);
}


void block_free(block_descr_t *descr, void * addr)
{
  block_header_t *block_ptr1, *block_ptr2;

  if (descr == NULL)
    return;
  
#ifdef DEBUG
  fprintf(stderr, "\nstart of block_free:\n");
#endif
  block_ptr1 = block_get_header_address(addr);
#ifdef DEBUG
  fprintf(stderr, "\nblock_free: block to suppress:\n");
  block_print_header(block_ptr1);
#endif
  block_ptr2 = block_get_next(block_ptr1);
  block_ptr1->size = (size_t)((char *)block_ptr2 - (char *)addr);
  /* 
     Merge with the next block if not empty. 
   */
  if(block_get_usable_size(block_ptr2) != 0)
    {
#ifdef DEBUG
  fprintf(stderr, "Merging with the next block...\n");
#endif
      block_ptr1->size += block_get_overall_size(block_ptr2);
      block_ptr1->next = block_get_next(block_ptr2);
      if (block_get_next(block_ptr2) != NULL)
	block_ptr2->next->prev = block_ptr1;
    }
  /* 
     Merge with the previous block if not empty. 
   */
  block_ptr2 = block_get_prev(block_ptr1);
  if((block_ptr2 != NULL) && (block_get_usable_size(block_ptr2) != 0))
    {
#ifdef DEBUG
  fprintf(stderr, "Merging with the previous block...\n");
#endif
      block_ptr2->size += block_get_overall_size(block_ptr1);
      block_ptr2->next = block_get_next(block_ptr1);
      if (block_get_next(block_ptr1) != NULL)
	block_ptr1->next->prev = block_ptr2;
      block_ptr1 = block_ptr2;
    }
  /*
    If the slot is empty, set it free.
  */
  if ((block_get_prev(block_ptr1) == NULL) && (block_get_next(block_get_next(block_ptr1)) == NULL))
    /*
      There is only one block left in the slot (except for the last empty one).
    */
    {
      slot_header_t *slot_ptr = slot_get_header_address(block_ptr1);
      if (descr->last_slot_used == slot_ptr)
	descr->last_slot_used = NULL;
      /*
	Useless to memorize the the address of the last block set free.
      */
	descr->last_block_set_free = NULL;
      slot_free(&descr->slot_descr, (void *)block_ptr1);
    }
  else
    /*
      Keep the address of the last block set free.
    */
    descr->last_block_set_free = NULL /* ATTENTION : A MODIFIER !!! (was: block_ptr1) */;
#ifdef DEBUG
  fprintf(stderr, "end of block_free:\n");
/*  block_print_list(descr);*/
#endif
}


void block_flush_list(block_descr_t *descr)
{
  slot_flush_list(&descr->slot_descr);
}


void block_exit()
{
  slot_exit();
}

void block_pack_all(block_descr_t *descr)
{

  slot_header_t *slot_ptr = slot_get_first(&descr->slot_descr);
  block_header_t *block_ptr, *next_block_ptr;
#ifdef DEBUG
  fprintf(stderr,"start pack_all:\n");
#endif
   /* 
      pack the address of the first slot 
   */
   old_mad_pack_byte(MAD_IN_HEADER, (char *)&slot_ptr, sizeof(slot_header_t *));
   /* 
      pack the slot list 
   */
   while (slot_ptr != NULL)
     {
       /* 
	  pack the header 
       */
       old_mad_pack_byte(MAD_IN_HEADER, (char*)slot_ptr, sizeof(slot_header_t));
       /*
	 pack the remaining slot data 
       */
       block_ptr = (block_header_t *)slot_get_usable_address(slot_ptr);
       do
	 {
	   old_mad_pack_byte(MAD_IN_HEADER, (char *)block_ptr, sizeof(block_header_t));
	   next_block_ptr = block_get_next(block_ptr);
	   if ((next_block_ptr != NULL) && (block_get_usable_size(block_ptr) == 0))
	     {
	       char *addr = (char *)block_get_usable_address(block_ptr);
	       old_mad_pack_byte(MAD_IN_PLACE, addr, (char *)next_block_ptr - addr);
	     }
	   block_ptr = next_block_ptr;
	 }
       while(block_ptr != NULL);
       slot_ptr = slot_get_next(slot_ptr);
     }
#ifdef DEBUG
   fprintf(stderr, "pack_all ended\n");
#endif
}


void block_unpack_all()
{
  slot_header_t *slot_ptr;
  slot_header_t slot_header;
  void *usable_add;
  block_header_t *block_ptr, *next_block_ptr;

   /* 
      unpack the address of the first slot 
   */
   old_mad_unpack_byte(MAD_IN_HEADER, (char *)&slot_ptr, sizeof(slot_header_t *));
#ifdef DEBUG
   tfprintf(stderr,"unpack_all:the address of the first slot is %p\n", slot_ptr);
#endif

   /* 
      unpack the slots 
   */
   while(slot_ptr != NULL)
     {
       /* 
	  unpack the slot header 
       */
       old_mad_unpack_byte(MAD_IN_HEADER, (char *)&slot_header, sizeof(slot_header_t));
       /* 
	  allocate memory for the slot 
       */
       usable_add = slot_general_alloc(NULL, slot_get_usable_size(&slot_header), NULL, slot_get_usable_address(slot_ptr));
       /* 
	  copy the header 
       */
       *slot_ptr = slot_header;
       /* 
	  unpack the remaining slot data
       */
       block_ptr = (block_header_t *)slot_get_usable_address(slot_ptr);
       do
	 {
	   old_mad_unpack_byte(MAD_IN_HEADER, (char *)block_ptr, sizeof(block_header_t));
	   next_block_ptr = block_get_next(block_ptr);
	   if ((next_block_ptr != NULL) && (block_get_usable_size(block_ptr) == 0))
	     {
	       char *addr = (char *)block_get_usable_address(block_ptr);
	       old_mad_unpack_byte(MAD_IN_PLACE, addr, (char *)next_block_ptr - addr);
	     }
	   block_ptr = next_block_ptr;
	 }
       while(block_ptr != NULL);

       slot_ptr = slot_ptr->next;
     }
}


block_descr_t *block_merge_lists(block_descr_t *src, block_descr_t *dest)
{
 slot_descr_t *src_slot_list = &src->slot_descr;
 slot_descr_t *dest_slot_list = &dest->slot_descr;

 if ((src == NULL)||(dest == NULL))
   return dest;

 src_slot_list = &src->slot_descr;
 dest_slot_list = &dest->slot_descr;

 if (src_slot_list->last_slot == NULL)
   /*
     the source list is empty; return the destination list unchanged
   */
   return dest;

 src_slot_list->last_slot->next = slot_get_first(dest_slot_list); 

 if (slot_get_first(dest_slot_list) != NULL)
   /*
     if the destination list is not empty
   */
   dest_slot_list->slots->prev = src_slot_list->last_slot;
 else
   dest_slot_list->last_slot = src_slot_list->last_slot;

 dest_slot_list->slots = slot_get_first(src_slot_list);
 /*
   reinitialize block descriptor
 */
 block_init_list(src);

 return dest;
}
