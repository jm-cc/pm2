
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
