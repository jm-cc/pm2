/*
 * leoparse_types.h
 * ----------------
 */

#ifndef __LEOPARSE_TYPES_H
#define __LEOPARSE_TYPES_H

typedef enum e_leoparse_entry_type
{
  leoparse_e_undefined = 0,
  leoparse_e_slist,
  leoparse_e_object,
} leoparse_entry_type_t;

typedef struct s_leoparse_htable_entry
{
  char                  *id;
  p_leoparse_object_t    object;
  p_tbx_slist_t          slist;
  leoparse_entry_type_t  type;
} leoparse_htable_entry_t;

typedef enum e_leoparse_object_type
{
  leoparse_o_undefined = 0,
  leoparse_o_id,
  leoparse_o_string,
  leoparse_o_htable,
} leoparse_object_type_t;

typedef struct s_leoparse_object
{
  leoparse_object_type_t  type;
  p_tbx_htable_t          htable;
  char                   *string;
  char                   *id;
} leoparse_object_t;

#endif /* __LEOPARSE_TYPES_H */
