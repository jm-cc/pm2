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
$Log: mad_buffers.c,v $
Revision 1.6  2000/03/27 08:50:50  oaumage
- pre-support decoupage de groupes
- correction au niveau du support du demarrage manuel

Revision 1.5  2000/03/08 17:19:10  oaumage
- support de compilation avec Marcel sans PM2
- pre-support de packages de Threads != Marcel
- utilisation de TBX_MALLOC

Revision 1.4  2000/02/28 11:06:11  rnamyst
Changed #include "" into #include <>.

Revision 1.3  2000/01/13 14:45:55  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant
- mad_channel.c, madeleine.c: amelioration des fonctions `par defaut' au niveau
  du support des drivers

Revision 1.2  1999/12/15 17:31:27  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_buffers.c
 * =============
 */

#include "madeleine.h"

p_mad_buffer_t mad_get_user_send_buffer(void    *ptr,
					size_t   length)
{
  p_mad_buffer_t buffer;

  buffer = mad_alloc_buffer_struct();
  
  buffer->buffer        = ptr;
  buffer->length        = length;
  buffer->bytes_written = length ;
  buffer->bytes_read    = 0;
  buffer->type          = mad_user_buffer;
  buffer->specific      = NULL;

  return buffer;
}

p_mad_buffer_t mad_get_user_receive_buffer(void    *ptr,
					   size_t   length)
{
  p_mad_buffer_t buffer;

  buffer = mad_alloc_buffer_struct();
  
  buffer->buffer        = ptr;
  buffer->length        = length;
  buffer->bytes_written = 0 ;
  buffer->bytes_read    = 0;
  buffer->type          = mad_user_buffer;
  buffer->specific      = NULL;

  return buffer;
}

p_mad_buffer_t mad_alloc_buffer(size_t length)
{
  p_mad_buffer_t buffer ;

  buffer = mad_alloc_buffer_struct();

  buffer->buffer = TBX_MALLOC(length);
  if (buffer->buffer == NULL)
    {
      FAILURE("not enough memory");
    }
  
  buffer->length        = length;
  buffer->bytes_read    = 0;
  buffer->bytes_written = 0;
  buffer->type          = mad_dynamic_buffer;
  buffer->specific      = NULL;
  
  return buffer;
}

tbx_bool_t mad_buffer_full(p_mad_buffer_t buffer)
{
  if (buffer->bytes_written < buffer->length)
    {
      return tbx_false ;
    }
  else
    {
      return tbx_true ;
    }
}

tbx_bool_t mad_more_data(p_mad_buffer_t buffer)
{
  if (buffer->bytes_read < buffer->bytes_written)
    {
      return tbx_true ;
    }
  else
    {
      return tbx_false ;
    }
}

size_t mad_copy_length(p_mad_buffer_t   source,
		       p_mad_buffer_t   destination)
{
  return min((source->bytes_written - source->bytes_read),
	     (destination->length - destination->bytes_written));
}

size_t mad_copy_buffer(p_mad_buffer_t   source,
		       p_mad_buffer_t   destination)
{
  size_t length ;

  LOG_PTR("mad_copy_buffer: source",             source);
  LOG_PTR("mad_copy_buffer: source buffer",      source->buffer);
  LOG_PTR("mad_copy_buffer: destination",        destination);
  LOG_PTR("mad_copy_buffer: destination buffer", destination->buffer);
  
  length = mad_copy_length(source, destination);

  memcpy(destination->buffer + destination->bytes_written,
	 source->buffer + source->bytes_read,
	 length);

  source->bytes_read         += length ;
  destination->bytes_written += length ;

  return length;
}

size_t mad_pseudo_copy_buffer(p_mad_buffer_t   source,
			      p_mad_buffer_t   destination)
{
  size_t length ;

  length = mad_copy_length(source, destination);

  source->bytes_read         += length ;
  destination->bytes_written += length ;

  return length;
}

p_mad_buffer_pair_t mad_make_sub_buffer_pair(p_mad_buffer_t   source,
					     p_mad_buffer_t   destination)
{
  p_mad_buffer_pair_t   pair ;
  size_t                length ;

  length = mad_copy_length(source, destination);
  pair   = mad_alloc_buffer_pair_struct();
  
  pair->dynamic_buffer.buffer        =
    source->buffer + source->bytes_read;
  pair->dynamic_buffer.length        = length;
  pair->dynamic_buffer.bytes_written = length;
  pair->dynamic_buffer.bytes_read    = 0;
  pair->dynamic_buffer.type          = mad_internal_buffer;
  pair->dynamic_buffer.specific      = NULL;

  pair->static_buffer.buffer         =
    destination->buffer + destination->bytes_written;
  pair->static_buffer.length         = length;
  pair->static_buffer.bytes_written  = 0;
  pair->static_buffer.bytes_read     = 0 ;
  pair->static_buffer.type           = mad_internal_buffer;
  pair->static_buffer.specific       = NULL;

  return pair;
}

p_mad_buffer_t mad_duplicate_buffer(p_mad_buffer_t source)
{
  p_mad_buffer_t destination;
  
  destination = mad_alloc_buffer(source->length);
  mad_copy_buffer(source, destination);

  return destination;
}

void mad_make_buffer_group(p_mad_buffer_group_t   buffer_group,
			   p_tbx_list_t           buffer_list,
			   p_mad_link_t           link)
{
  tbx_extract_sub_list(buffer_list, &(buffer_group->buffer_list));
  buffer_group->link = link;
}

p_mad_buffer_t mad_split_buffer(p_mad_buffer_t buffer,
				size_t         limit)
{
  if (buffer->length > limit)
    {
      p_mad_buffer_t new_buffer;

      new_buffer = mad_alloc_buffer_struct();
      new_buffer->buffer        = buffer->buffer + limit;
      new_buffer->length        = buffer->length - limit;
      new_buffer->bytes_written =
	(buffer->bytes_written > limit)?buffer->bytes_written - limit:0;
      new_buffer->bytes_read    =
	(buffer->bytes_read > limit)?buffer->bytes_read - limit:0;
      new_buffer->type          = mad_user_buffer;
      new_buffer->specific      = NULL;

      buffer->length        = limit;
      if (buffer->bytes_written > limit)
	buffer->bytes_written = limit;
      if (buffer->bytes_read > limit)
	buffer->bytes_read = limit;

      return new_buffer;
    }
  else
    return NULL;
}

size_t mad_append_buffer_to_list(p_tbx_list_t   list,
				 p_mad_buffer_t buffer,
				 size_t         position,
				 size_t         limit)
{
  if (limit)
    {
      p_mad_buffer_t new_buffer = buffer;
      
      if (position)
	{
	  new_buffer = mad_split_buffer(buffer, limit - position);
	  
	  tbx_append_list(list, buffer);

	  if (!new_buffer)
	    {
	      return position + buffer->length;
	    }
	}

      do
	{
	  buffer = new_buffer;
	  new_buffer = mad_split_buffer(buffer, limit);
	  tbx_append_list(list, buffer);
	}
      while(new_buffer);

      return buffer->length % limit;
    }
  else
    {
      tbx_append_list(list, buffer);
      return position + buffer->length;
    }
}
