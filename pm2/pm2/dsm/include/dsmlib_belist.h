/* Warning: All the following is NOT reentrant.*/
#include "tbx.h"

typedef struct be_cell { unsigned long number; struct be_cell * next;} be_cell;

typedef be_cell *be_list;

be_list init_be_list(long nb_preallocated_cells); //Warning: To be called only
                                                  //         ONCE!!! 
void add_to_be_list(be_list *my_list, unsigned long to_add);

unsigned long remove_first_from_be_list(be_list *my_list);

int remove_if_noted(be_list *my_list, unsigned long to_remove);

void fprintf_be_list(be_list the_list);
