
/*
 * Mad_buffers.c
 * =============
 */

#include "madeleine.h"

p_mad_buffer_t
mad_get_user_send_buffer(void   *ptr,
			 size_t  length)
{
  p_mad_buffer_t buffer = NULL;

  buffer = mad_alloc_buffer_struct();
  
  buffer->buffer        = ptr;
  buffer->length        = length;
  buffer->bytes_written = length ;
  buffer->bytes_read    = 0;
  buffer->type          = mad_user_buffer;
  buffer->specific      = NULL;

  return buffer;
}

p_mad_buffer_t
mad_get_user_receive_buffer(void   *ptr,
			    size_t  length)
{
  p_mad_buffer_t buffer = NULL;

  buffer = mad_alloc_buffer_struct();
  
  buffer->buffer        = ptr;
  buffer->length        = length;
  buffer->bytes_written = 0 ;
  buffer->bytes_read    = 0;
  buffer->type          = mad_user_buffer;
  buffer->specific      = NULL;

  return buffer;
}

p_mad_buffer_t
mad_alloc_buffer(size_t length)
{
  p_mad_buffer_t buffer = NULL;

  buffer = mad_alloc_buffer_struct();

  buffer->buffer = TBX_MALLOC(length);
  
  if (!buffer->buffer)
    FAILURE("not enough memory");
  
  buffer->length        = length;
  buffer->bytes_read    = 0;
  buffer->bytes_written = 0;
  buffer->type          = mad_dynamic_buffer;
  buffer->specific      = NULL;
  
  return buffer;
}

tbx_bool_t
mad_buffer_full(p_mad_buffer_t buffer)
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

tbx_bool_t
mad_more_data(p_mad_buffer_t buffer)
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

size_t
mad_copy_length(p_mad_buffer_t source,
		p_mad_buffer_t destination)
{
  return min((source->bytes_written - source->bytes_read),
	     (destination->length - destination->bytes_written));
}

size_t
mad_copy_buffer(p_mad_buffer_t source,
		p_mad_buffer_t destination)
{
  size_t length = 0;

  length = mad_copy_length(source, destination);

  memcpy(destination->buffer + destination->bytes_written,
	 source->buffer + source->bytes_read, length);

  source->bytes_read         += length ;
  destination->bytes_written += length ;

  return length;
}

size_t
mad_pseudo_copy_buffer(p_mad_buffer_t source,
		       p_mad_buffer_t destination)
{
  size_t length = 0;

  length = mad_copy_length(source, destination);

  source->bytes_read         += length ;
  destination->bytes_written += length ;

  return length;
}

p_mad_buffer_pair_t
mad_make_sub_buffer_pair(p_mad_buffer_t source,
			 p_mad_buffer_t destination)
{
  p_mad_buffer_pair_t pair   = NULL;
  size_t              length =    0;

  length = mad_copy_length(source, destination);
  pair   = mad_alloc_buffer_pair_struct();
  
  pair->dynamic_buffer.buffer        = source->buffer + source->bytes_read;
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

p_mad_buffer_t
mad_duplicate_buffer(p_mad_buffer_t source)
{
  p_mad_buffer_t destination = NULL;
  
  destination = mad_alloc_buffer(source->length);
  mad_copy_buffer(source, destination);

#ifdef MAD_FORWARDING
  destination->informations = source->informations;
#endif /* MAD_FORWARDING */

  return destination;
}

void
mad_make_buffer_group(p_mad_buffer_group_t buffer_group,
		      p_tbx_list_t         buffer_list,
		      p_mad_link_t         link)
{
  tbx_extract_sub_list(buffer_list, &(buffer_group->buffer_list));
  buffer_group->link = link;
}

p_mad_buffer_t
mad_split_buffer(p_mad_buffer_t buffer,
		 size_t         limit)
{
  if (buffer->length > limit)
    {
      p_mad_buffer_t new_buffer = NULL;

      new_buffer = mad_alloc_buffer_struct();
      new_buffer->buffer        = buffer->buffer + limit;
      new_buffer->length        = buffer->length - limit;
      new_buffer->bytes_written =
	(buffer->bytes_written > limit)?buffer->bytes_written - limit:0;
      new_buffer->bytes_read    =
	(buffer->bytes_read > limit)?buffer->bytes_read - limit:0;
      new_buffer->type          = mad_user_buffer;
      new_buffer->specific      = NULL;

      buffer->length = limit;
      
      if (buffer->bytes_written > limit)
	{
	  buffer->bytes_written = limit;
	}
      
      if (buffer->bytes_read > limit)
	{
	  buffer->bytes_read = limit;
	}
      
      return new_buffer;
    }
  else
    return NULL;
}

size_t
mad_append_buffer_to_list(p_tbx_list_t   list,
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
      while (new_buffer);

      return buffer->length % limit;
    }
  else
    {
      tbx_append_list(list, buffer);
      return position + buffer->length;
    }
}
