/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

              Eager Release Consistency Implementation
                         Vincent Bernardi

                      2000 All Rights Reserved


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

#include "dsmlib_belist.h"

p_tbx_memory_t my_preallocated_memory;

be_list init_be_list(long nb_preallocated_be_cells)
{
  
  tbx_malloc_init(&my_preallocated_memory, sizeof(be_cell), nb_preallocated_be_cells);

  return NULL;
}

void add_to_be_list(be_list *my_list, unsigned long to_add){
  be_cell *my_new_be_cell = (be_cell *)tbx_malloc(my_preallocated_memory);
  my_new_be_cell->number = to_add;
  fprintf(stderr, "Page %lu soon added to list\n", to_add);
  if(my_list == NULL){
    my_new_be_cell->next = NULL;
    *my_list = my_new_be_cell;
  }
  else{
    my_new_be_cell->next = *my_list;
    *my_list = my_new_be_cell;
  }
  return;
}


unsigned long remove_first_from_be_list(be_list *my_list){
  be_cell *first_be_cell;
  unsigned long back;

  if(*my_list == NULL)
    return (unsigned long)-1;
  back = (*my_list)->number;
  first_be_cell = *my_list;
  *my_list = (*my_list)->next;
  tbx_free(my_preallocated_memory,(void *)first_be_cell);
  return back;
}


int remove_if_noted(be_list *my_list, unsigned long to_remove){
  be_cell *pathfinder, *follower;
  
  if (*my_list == NULL)
    return 0;
  if((*my_list)->number == to_remove){
    pathfinder = *my_list;
    (*my_list) = (*my_list)->next;
    tbx_free(my_preallocated_memory,(void *)pathfinder);
    return 1;
  }

  pathfinder = (*my_list)->next;
  follower = *my_list;
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

void fprintf_be_list(be_list the_list){
  be_list rest;
  
  rest = the_list;

  while(rest != NULL){
    fprintf(stderr,"%lu ", rest->number);
    rest = rest->next;
  }
  fprintf(stderr, "\n");
  return;
}






