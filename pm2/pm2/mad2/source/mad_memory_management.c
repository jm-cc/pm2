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
$Log: mad_memory_management.c,v $
Revision 1.5  2000/11/10 14:17:56  oaumage
- nouvelle procedure d'initialisation

Revision 1.4  2000/02/28 11:06:15  rnamyst
Changed #include "" into #include <>.

Revision 1.3  2000/01/13 14:45:58  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant
- mad_channel.c, madeleine.c: amelioration des fonctions `par defaut' au niveau
  du support des drivers

Revision 1.2  1999/12/15 17:31:28  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * mad_memory_management.c
 * -----------------------
 */

#include "madeleine.h"

/* MACROS */
#define INITIAL_BUFFER_NUM 1024
#define INITIAL_BUFFER_GROUP_NUM 64
#define INITIAL_BUFFER_PAIR_NUM 64

/* data structures */
static p_tbx_memory_t mad_buffer_memory;
static p_tbx_memory_t mad_buffer_group_memory;
static p_tbx_memory_t mad_buffer_pair_memory;

/* functions */

/* tbx_memory_manager_init:
   initialize mad memory manager */
void
mad_memory_manager_init(int    argc,
			char **argv)
{
  tbx_malloc_init(&mad_buffer_memory,
		  sizeof(mad_buffer_t),
		  INITIAL_BUFFER_NUM);  
  tbx_malloc_init(&mad_buffer_group_memory,
		  sizeof(mad_buffer_group_t),
		  INITIAL_BUFFER_GROUP_NUM);
  tbx_malloc_init(&mad_buffer_pair_memory,
		  sizeof(mad_buffer_pair_t),
		  INITIAL_BUFFER_PAIR_NUM);
}

void
mad_memory_manager_clean()
{
  tbx_malloc_clean(mad_buffer_memory);
  tbx_malloc_clean(mad_buffer_group_memory);
  tbx_malloc_clean(mad_buffer_pair_memory);
}

p_mad_buffer_t
mad_alloc_buffer_struct()
{
  return tbx_malloc(mad_buffer_memory);
}

void
mad_free_buffer_struct(p_mad_buffer_t buffer)
{
  tbx_free(mad_buffer_memory, buffer);
}

void
mad_free_buffer(p_mad_buffer_t buffer)
{
  if (buffer->type == mad_dynamic_buffer)
    {
      if (buffer->buffer)
	{
	  free(buffer->buffer);
	}
    }
  tbx_free(mad_buffer_memory, buffer);
}

p_mad_buffer_group_t
mad_alloc_buffer_group_struct()
{
  return tbx_malloc(mad_buffer_group_memory);
}

void
mad_free_buffer_group_struct(p_mad_buffer_group_t buffer_group)
{
  tbx_free(mad_buffer_group_memory, buffer_group);
}

void
mad_foreach_free_buffer_group_struct(void *object)
{
  tbx_free(mad_buffer_group_memory, object);
}

void
mad_foreach_free_buffer(void *object)
{
  p_mad_buffer_t buffer = object;
  
  if (buffer->type == mad_dynamic_buffer)
    {
      free(buffer->buffer);
    }

  tbx_free(mad_buffer_memory, object);
}

p_mad_buffer_pair_t
mad_alloc_buffer_pair_struct()
{
  return tbx_malloc(mad_buffer_pair_memory);
}

void
mad_free_buffer_pair_struct(p_mad_buffer_pair_t buffer_pair)
{
  tbx_free(mad_buffer_pair_memory, buffer_pair);
}

void
mad_foreach_free_buffer_pair_struct(void *object)
{
  tbx_free(mad_buffer_pair_memory, object);
}
