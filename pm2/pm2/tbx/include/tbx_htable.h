
/*
 * Tbx_htable.h : hash-table
 * ============----------------
 */

#ifndef TBX_HTABLE_H
#define TBX_HTABLE_H

/*
 * Data types 
 * ----------
 */
typedef char             *tbx_htable_key_t;
typedef tbx_htable_key_t *p_tbx_htable_key_t;

typedef int                        tbx_htable_bucket_count_t;
typedef tbx_htable_bucket_count_t *p_tbx_htable_bucket_count_t;

typedef int                         tbx_htable_element_count_t;
typedef tbx_htable_element_count_t *p_tbx_htable_element_count_t;

/*
 * Data structures 
 * ---------------
 */
typedef struct s_tbx_htable_element
{
  tbx_htable_key_t        key;
  void                   *object;
  p_tbx_htable_element_t  next;
} tbx_htable_element_t;

typedef struct s_tbx_htable
{
  TBX_SHARED;
  tbx_htable_bucket_count_t   nb_bucket;
  p_tbx_htable_element_t     *bucket_array;
  tbx_htable_element_count_t  nb_element;
} tbx_htable_t;

#endif /* TBX_HTABLE_H */
