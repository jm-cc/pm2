/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#ifndef NM_TAGS_H
#define NM_TAGS_H

/* ** Tag-indexed containers- Warning: black-magic here! */

#if defined(NM_TAGS_AS_FLAT_ARRAY)
#  define NM_TAG_TABLE_TYPE(NAME, ENTRY_TYPE) NM_TAG_TABLE_FLAT_ARRAY(NAME, ENTRY_TYPE) 
#elif defined(NM_TAGS_AS_HASHTABLE)
#  define NM_TAG_TABLE_TYPE(NAME, ENTRY_TYPE) NM_TAG_TABLE_HASHTABLE(NAME, ENTRY_TYPE) 
#elif defined(NM_TAGS_AS_INDIRECT_HASH)
#  define NM_TAG_TABLE_TYPE(NAME, ENTRY_TYPE) NM_TAG_TABLE_INDIRECT_HASH(NAME, ENTRY_TYPE) 
#endif

/* Implementation of tag-indexed container: as flat array */

#ifdef NM_TAGS_AS_FLAT_ARRAY

static inline int nm_tag_eq(nm_tag_t tag1, nm_tag_t tag2)
{
  return tag1 == tag2;
}

#define NM_TAG_TABLE_FLAT_ARRAY(NAME, ENTRY_TYPE) \
  struct NAME##_table_s \
  { \
    ENTRY_TYPE _array[256]; \
  }; \
  static inline void NAME##_table_init(struct NAME##_table_s*t) \
  { \
    nm_tag_t i; \
    for(i = 0; i < 255; i++) \
    { \
      NAME##_ctor(&t->_array[i], i); \
    } \
   } \
  static inline void NAME##_table_destroy(struct NAME##_table_s*t) \
  { \
    nm_tag_t i; \
    for(i = 0; i < 255; i++) \
    { \
      NAME##_dtor(&t->_array[i]); \
    } \
   } \
  static inline ENTRY_TYPE* NAME##_get(struct NAME##_table_s*table, nm_tag_t tag) \
  { \
    return &table->_array[tag]; \
  }
#endif /* NM_TAGS_AS_FLAT_ARRAY */



/* Implementation of tag-indexed container: as a Puk hashtable */

#if defined(NM_TAGS_AS_HASHTABLE)

static inline int nm_tag_eq(nm_tag_t tag1, nm_tag_t tag2)
{
  return tag1 == tag2;
}

static uint32_t nm_tag_hash(const void*_tag)
{
  /* use the tag itself as a hashing key, since patterns or
   * periods are likely to be powers of 2, and hashtable size
   * is always a prime number.
   */
  const nm_tag_t tag = (unsigned long)_tag;
  return (uint32_t)tag;
}
static int nm_tag_hasheq(const void*_tag1, const void*_tag2)
{
  const nm_tag_t tag1 = (intptr_t)_tag1;
  const nm_tag_t tag2 = (intptr_t)_tag2;
  return (tag1 == tag2);
}

#define NM_TAG_TABLE_HASHTABLE(NAME, ENTRY_TYPE) \
  struct NAME##_table_s \
  { \
    puk_hashtable_t _h; \
    p_tbx_memory_t _mem; \
  }; \
  static inline void NAME##_table_init(struct NAME##_table_s*t) \
  { \
    t->_h = puk_hashtable_new(&nm_tag_hash, &nm_tag_hasheq); \
    tbx_malloc_extended_init(&t->_mem, sizeof(ENTRY_TYPE), NM_TAGS_PREALLOC, "nmad/taghash/" #NAME, 1); \
  } \
  static inline void NAME##_table_destroy(struct NAME##_table_s*t) \
  { \
    puk_hashtable_enumerator_t e = puk_hashtable_enumerator_new(t->_h); \
    ENTRY_TYPE*data = puk_hashtable_enumerator_next_data(e); \
    while(data) \
    { \
      NAME##_dtor(data); \
      tbx_free(t->_mem, data); \
      data = puk_hashtable_enumerator_next_data(e); \
    } \
    puk_hashtable_delete(t->_h); \
    tbx_malloc_clean(t->_mem); \
  } \
  static inline ENTRY_TYPE* NAME##_get(struct NAME##_table_s*table, nm_tag_t tag) \
  { \
    const nm_tag_t shift_tag = tag + 1; /* 0 is forbidden as a hashing key- shift tags */ \
    void*key = (void*)((intptr_t)shift_tag); \
    ENTRY_TYPE*e = puk_hashtable_lookup(table->_h, key); \
    if(tbx_unlikely(e == NULL)) { \
      e = tbx_malloc(table->_mem); \
      NAME##_ctor(e, tag); \
      puk_hashtable_insert(table->_h, key, e); \
    } \
    return e; \
  }

#endif /* NM_TAGS_AS_HASHTABLE */


#if defined(NM_TAGS_AS_INDIRECT_HASH)

static inline int nm_tag_eq(nm_tag_t tag1, nm_tag_t tag2)
{
  return (memcmp(&tag1, &tag2, sizeof(nm_tag_t)) == 0);
}

static uint32_t nm_tag_indirect_hash(const void*_tag)
{
  /* const nm_tag_t*p_tag = (const nm_tag_t*)_tag; */
  const uint32_t*p_hash = (const uint32_t*)_tag;
  return *p_hash;
}
static int nm_tag_indirect_eq(const void*_tag1, const void*_tag2)
{
  const nm_tag_t*p_tag1 = (const nm_tag_t*)_tag1;
  const nm_tag_t*p_tag2 = (const nm_tag_t*)_tag2;
  return (memcmp(p_tag1, p_tag2, sizeof(nm_tag_t)) == 0);
}

#define NM_TAG_TABLE_INDIRECT_HASH(NAME, ENTRY_TYPE)			\
  struct NAME##_table_s							\
  {									\
    puk_hashtable_t _h;							\
    p_tbx_memory_t _mem;						\
  };									\
  struct NAME##_entry_s							\
  {									\
    ENTRY_TYPE _data;							\
    nm_tag_t _tag;							\
  };									\
  static inline void NAME##_table_init(struct NAME##_table_s*t)		\
  {									\
    t->_h = puk_hashtable_new(&nm_tag_indirect_hash, &nm_tag_indirect_eq); \
    tbx_malloc_extended_init(&t->_mem, sizeof(struct NAME##_entry_s), NM_TAGS_PREALLOC, "nmad/taghashentry/" #NAME, 1); \
  }									\
  static inline void NAME##_table_destroy(struct NAME##_table_s*t)	\
  {									\
    puk_hashtable_enumerator_t e = puk_hashtable_enumerator_new(t->_h); \
    ENTRY_TYPE*data = puk_hashtable_enumerator_next_data(e);		\
    while(data)								\
      {									\
	NAME##_dtor(data);						\
	tbx_free(t->_mem, data);					\
	data = puk_hashtable_enumerator_next_data(e);			\
      }									\
    puk_hashtable_delete(t->_h);					\
    tbx_malloc_clean(t->_mem);						\
  }									\
  static inline ENTRY_TYPE* NAME##_get(struct NAME##_table_s*table, nm_tag_t tag) \
  {									\
    const void*key = (const void*)&tag;					\
    ENTRY_TYPE*e = puk_hashtable_lookup(table->_h, key);		\
    if(tbx_unlikely(e == NULL))						\
      {									\
	struct NAME##_entry_s*entry = tbx_malloc(table->_mem);		\
	key = &entry->_tag;						\
	e   = &entry->_data;						\
	NAME##_ctor(&entry->_data, tag);				\
	entry->_tag = tag;						\
	puk_hashtable_insert(table->_h, key, e);			\
      }									\
    return e;								\
  }

#endif /* NM_TAGS_AS_INDIRECT_HASH */


#endif /* NM_TAGS_H */
