/* Warning: All the following is NOT reentrant.*/
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

#include "tbx.h"

typedef struct be_cell { unsigned long number; struct be_cell * next;} be_cell;

typedef be_cell *be_list;

be_list init_be_list(long nb_preallocated_cells); //Warning: To be called only
                                                  //         ONCE!!! 
void add_to_be_list(be_list *my_list, unsigned long to_add);

unsigned long remove_first_from_be_list(be_list *my_list);

int remove_if_noted(be_list *my_list, unsigned long to_remove);

void fprintf_be_list(be_list the_list);
