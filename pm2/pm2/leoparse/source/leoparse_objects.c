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
$Log: leoparse_objects.c,v $
Revision 1.1  2000/12/19 16:57:48  oaumage
- finalisation de leoparse
- exemples pour leoparse
- modification des macros de logging
- version typesafe de certaines macros
- finalisation des tables de hachage
- finalisation des listes de recherche


______________________________________________________________________________
*/
#include "leoparse.h"

/*
 * leoparse_objects.c
 * ------------------
 */
p_leoparse_object_t
leoparse_get_object(p_leoparse_htable_entry_t entry)
{
  if (entry->type == leoparse_e_object)
    {
      return entry->object;
    }
  else
    FAILURE("parse error : object expected");
}

p_tbx_slist_t
leoparse_get_slist(p_leoparse_htable_entry_t entry)
{
  if (entry->type == leoparse_e_slist)
    {
      return entry->slist;
    }
  else
    FAILURE("parse error : list expected");
}

p_tbx_htable_t
leoparse_get_htable(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_htable)
    {
      return object->htable;
    }
  else
    FAILURE("parse error : table expected");
}

char *
leoparse_get_string(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_string)
    {
      return object->string;
    }
  else
    FAILURE("parse error : string expected");
}

char *
leoparse_get_id(p_leoparse_object_t object)
{
  if (object->type == leoparse_o_id)
    {
      return object->id;
    }
  else
    FAILURE("parse error : id expected");
}

p_tbx_slist_t
leoparse_read_slist(p_tbx_htable_t  htable,
		    char           *key)
{
  p_leoparse_htable_entry_t entry  = NULL;
  p_tbx_slist_t             result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      result = leoparse_get_slist(entry);
    }
  
  return result;
}

char *
leoparse_read_id(p_tbx_htable_t  htable,
		 char           *key)
{
  p_leoparse_htable_entry_t  entry  = NULL;
  p_leoparse_object_t        object = NULL;
  char                      *result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_get_object(entry);
      result = leoparse_get_id(object);
    }

  return result;
}

char *
leoparse_read_string(p_tbx_htable_t  htable,
		     char           *key)
{
  p_leoparse_htable_entry_t  entry  = NULL;
  p_leoparse_object_t        object = NULL;
  char                      *result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_get_object(entry);
      result = leoparse_get_string(object);
    }

  return result;
}

p_tbx_htable_t
leoparse_read_htable(p_tbx_htable_t  htable,
		     char           *key)
{
  p_leoparse_htable_entry_t entry  = NULL;
  p_leoparse_object_t       object = NULL;
  p_tbx_htable_t            result = NULL;

  entry = tbx_htable_get(htable, key);

  if (entry)
    {
      object = leoparse_get_object(entry);
      result = leoparse_get_htable(object);
    }

  return result;
}
