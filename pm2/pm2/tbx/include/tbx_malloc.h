
/*
 * tbx_malloc.h
 * ------------
 */

#ifndef TBX_MALLOC_H
#define TBX_MALLOC_H

/*
 * tbx_safe_malloc
 * ---------------
 */
typedef enum 
{
  tbx_safe_malloc_ERRORS_ONLY,
  tbx_safe_malloc_VERBOSE
} tbx_safe_malloc_mode_t, *p_tbx_safe_malloc_mode_t;


/*
 * tbx_malloc
 * ----------
 */
typedef struct s_tbx_memory
{
  TBX_SHARED; 
  void    *first_mem;
  void    *current_mem;
  size_t   block_len;
  long     mem_len;
  void    *first_free;
  long     first_new;
} tbx_memory_t;

#endif /* TBX_MALLOC_H */
