\
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

#include "dsmlib_belist.h"

p_tbx_memory_t my_preallocated_memory;

be_list init_be_list(long nb_preallocated_be_cells)
{
  be_list *my_be_list = tmalloc(sizeof(be_list));
  tbx_malloc_init(&my_preallocated_memory, sizeof(be_cell) + 4, nb_preallocated_be_cells,"dsmlib_belist");
  marcel_mutex_init(&(my_be_list->the_mutex), NULL);
  my_be_list->the_list = NULL;
  return *my_be_list;;
}

void add_to_be_list(be_list *my_list, unsigned long to_add){
  be_cell *my_new_be_cell = (be_cell *)tbx_malloc(my_preallocated_memory);
  my_new_be_cell->number = to_add;
//  fprintf(stderr, "Page %lu soon added to list\n", to_add);
  if(my_list->the_list == NULL){
    my_new_be_cell->next = NULL;
    my_list->the_list = my_new_be_cell;
  }
  else{
    my_new_be_cell->next = my_list->the_list;
    my_list->the_list = my_new_be_cell;
  }
  return;
}


unsigned long remove_first_from_be_list(be_list *my_list){
  be_cell *first_be_cell;
  unsigned long back;

  if(my_list->the_list == NULL)
    return (unsigned long)-1;
  back = my_list->the_list->number;
  first_be_cell = my_list->the_list;
  my_list->the_list = my_list->the_list->next;
  tbx_free(my_preallocated_memory,(void *)first_be_cell);
  return back;
}


int remove_if_noted(be_list *my_list, unsigned long to_remove){
  be_cell *pathfinder, *follower;
  
  if (my_list->the_list == NULL)
    return 0;
  if(my_list->the_list->number == to_remove){
    pathfinder = my_list->the_list;
    my_list->the_list = my_list->the_list->next;
    tbx_free(my_preallocated_memory,(void *)pathfinder);
    return 1;
  }

  pathfinder = my_list->the_list->next;
  follower = my_list->the_list;
  while(pathfinder != NULL)
    {
      if(pathfinder->number == to_remove)
	{
	  follower->next = pathfinder->next;
	  tbx_free(my_preallocated_memory, (void *)pathfinder);
	  return 1;
	}
      else
	{
	  follower = pathfinder;
	  pathfinder = pathfinder->next;
	}
    }
  return 0;
}

void fprintf_be_list(be_list my_be_list){
  be_cell *rest;
  
  rest = my_be_list.the_list;

  while(rest != NULL){
    fprintf(stderr,"%lu ", rest->number);
    rest = rest->next;
  }
  fprintf(stderr, " end of page list\n");
  return;
}

void lock_be_list(be_list my_list){
  marcel_mutex_lock(&my_list.the_mutex);
}
void unlock_be_list(be_list my_list){
  marcel_mutex_unlock(&my_list.the_mutex);
}





