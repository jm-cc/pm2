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


#define NM_TAGS_PREALLOC 255

/* ** Tag-indexed containers- Warning: black-magic here! */

#if defined(NM_TAGS_AS_FLAT_ARRAY)
#  define NM_TAG_TABLE_TYPE(NAME, ENTRY_TYPE) NM_TAG_TABLE_FLAT_ARRAY(NAME, ENTRY_TYPE) 
#elif defined(NM_TAGS_AS_HASHTABLE)
#  define NM_TAG_TABLE_TYPE(NAME, ENTRY_TYPE) NM_TAG_TABLE_HASHTABLE(NAME, ENTRY_TYPE) 
#elif defined(NM_TAGS_AS_INDIRECT_HASH)
#  define NM_TAG_TABLE_TYPE(NAME, ENTRY_TYPE) NM_TAG_TABLE_INDIRECT_HASH(NAME, ENTRY_TYPE) 
#endif

/** checks whether tags are equal. */
static inline int nm_tag_eq(nm_core_tag_t tag1, nm_core_tag_t tag2);
/** checks whether matching applies on tags- (recv_tag & mask) == lookup_tag
 */
static inline int nm_tag_match(nm_core_tag_t recv_tag, nm_core_tag_t lookup_tag, nm_core_tag_t mask);

/* Implementation of tag-indexed container: as flat array */

#ifdef NM_TAGS_AS_FLAT_ARRAY

static inline int nm_tag_eq(nm_core_tag_t tag1, nm_core_tag_t tag2)
{
  return tag1 == tag2;
}
static inline int nm_tag_match(nm_core_tag_t recv_tag, nm_core_tag_t lookup_tag, nm_core_tag_t mask)
{
  return ((recv_tag & mask) == lookup_tag);
}

#define NM_TAG_TABLE_FLAT_ARRAY(NAME, ENTRY_TYPE) \
  struct NAME##_table_s \
  { \
    ENTRY_TYPE _array[256]; \
  }; \
  static inline void NAME##_table_init(struct NAME##_table_s*t) \
  { \
    nm_core_tag_t i; \
    for(i = 0; i < 255; i++) \
    { \
      NAME##_ctor(&t->_array[i], i); \
    } \
   } \
  static inline void NAME##_table_destroy(struct NAME##_table_s*t) \
  { \
    nm_core_tag_t i; \
    for(i = 0; i < 255; i++) \
    { \
      NAME##_dtor(&t->_array[i]); \
    } \
   } \
  static inline ENTRY_TYPE* NAME##_get(struct NAME##_table_s*table, nm_core_tag_t tag) \
  { \
    return &table->_array[tag]; \
  }
#endif /* NM_TAGS_AS_FLAT_ARRAY */



/* Implementation of tag-indexed container: as a Puk hashtable */

#if defined(NM_TAGS_AS_HASHTABLE)

static inline int nm_tag_eq(nm_core_tag_t tag1, nm_core_tag_t tag2)
{
  return tag1 == tag2;
}
static inline int nm_tag_match(nm_core_tag_t recv_tag, nm_core_tag_t lookup_tag, nm_core_tag_t mask)
{
  return ((recv_tag & mask) == lookup_tag);
}

static uint32_t nm_tag_hash(const void*_tag)
{
  /* use the tag itself as a hashing key, since patterns or
   * periods are likely to be powers of 2, and hashtable size
   * is always a prime number.
   */
  const nm_core_tag_t tag = (unsigned long)_tag;
  return (uint32_t)tag;
}
static int nm_tag_hasheq(const void*_tag1, const void*_tag2)
{
  const nm_core_tag_t tag1 = (intptr_t)_tag1;
  const nm_core_tag_t tag2 = (intptr_t)_tag2;
  return (tag1 == tag2);
}

#define NM_TAG_TABLE_HASHTABLE(NAME, ENTRY_TYPE)			\
  PUK_ALLOCATOR_TYPE(NAME##_entry, ENTRY_TYPE);				\
  struct NAME##_table_s							\
  {									\
    puk_hashtable_t _h;							\
    NAME##_entry_allocator_t _allocator;				\
  };									\
  static inline void NAME##_table_init(struct NAME##_table_s*t)		\
  {									\
    t->_h = puk_hashtable_new(&nm_tag_hash, &nm_tag_hasheq);		\
    t->_allocator = NAME##_entry_allocator_new(NM_TAGS_PREALLOC);	\
  }									\
  static inline void NAME##_table_destroy(struct NAME##_table_s*t)	\
  {									\
    puk_hashtable_enumerator_t e = puk_hashtable_enumerator_new(t->_h); \
    ENTRY_TYPE*data = puk_hashtable_enumerator_next_data(e);		\
    while(data)								\
      {									\
	NAME##_dtor(data);						\
	NAME##_entry_free(t->_allocator, data);				\
	data = puk_hashtable_enumerator_next_data(e);			\
      }									\
    puk_hashtable_enumerator_delete(e);					\
    puk_hashtable_delete(t->_h);					\
    NAME##_entry_allocator_delete(t->_allocator);			\
  }									\
  static inline ENTRY_TYPE* NAME##_get(struct NAME##_table_s*t, nm_core_tag_t tag) \
  {									\
    const nm_core_tag_t shift_tag = tag + 1; /* 0 is forbidden as a hashing key- shift tags */ \
    void*key = (void*)((intptr_t)shift_tag);				\
    ENTRY_TYPE*e = puk_hashtable_lookup(t->_h, key);			\
    if(tbx_unlikely(e == NULL))						\
      {									\
	e = NAME##_entry_malloc(t->_allocator);				\
	NAME##_ctor(e, tag);						\
	puk_hashtable_insert(t->_h, key, e);				\
      }									\
    return e;								\
  }

#endif /* NM_TAGS_AS_HASHTABLE */


#if defined(NM_TAGS_AS_INDIRECT_HASH)

static inline int nm_tag_eq(nm_core_tag_t tag1, nm_core_tag_t tag2)
{
  return ((tag1.hashcode == tag2.hashcode) && (tag1.tag == tag2.tag));
}
static inline int nm_tag_match(nm_core_tag_t recv_tag, nm_core_tag_t lookup_tag, nm_core_tag_t mask)
{
  return ((recv_tag.tag & mask.tag) == lookup_tag.tag) &&
    ((recv_tag.hashcode & mask.hashcode) == lookup_tag.hashcode);
}

static uint32_t nm_tag_indirect_hash(const void*_tag)
{
  const nm_core_tag_t*p_tag = (const nm_core_tag_t*)_tag;
  const uint32_t hash = p_tag->tag + p_tag->hashcode;
  return hash;
}
static int nm_tag_indirect_eq(const void*_tag1, const void*_tag2)
{
  const nm_core_tag_t*p_tag1 = (const nm_core_tag_t*)_tag1;
  const nm_core_tag_t*p_tag2 = (const nm_core_tag_t*)_tag2;
  return ((p_tag1->hashcode == p_tag2->hashcode) && (p_tag1->tag == p_tag2->tag));
}

#define NM_TAG_TABLE_INDIRECT_HASH(NAME, ENTRY_TYPE)			\
  struct NAME##_entry_s							\
  {									\
    ENTRY_TYPE _data;							\
    nm_core_tag_t _tag;							\
  };									\
  PUK_ALLOCATOR_TYPE(NAME##_entry, struct NAME##_entry_s);		\
  struct NAME##_table_s							\
  {									\
    puk_hashtable_t _h;							\
    NAME##_entry_allocator_t _allocator;				\
  };									\
  static inline void NAME##_table_init(struct NAME##_table_s*t)		\
  {									\
    t->_h = puk_hashtable_new(&nm_tag_indirect_hash, &nm_tag_indirect_eq); \
    t->_allocator = NAME##_entry_allocator_new(NM_TAGS_PREALLOC);	\
  }									\
  static inline void NAME##_table_destroy(struct NAME##_table_s*t)	\
  {									\
    puk_hashtable_enumerator_t e = puk_hashtable_enumerator_new(t->_h); \
    struct NAME##_entry_s*entry = puk_hashtable_enumerator_next_data(e); \
    while(entry)							\
      {									\
	NAME##_dtor(&entry->_data);					\
	NAME##_entry_free(t->_allocator, entry);			\
	entry = puk_hashtable_enumerator_next_data(e);			\
      }									\
    puk_hashtable_enumerator_delete(e);					\
    puk_hashtable_delete(t->_h);					\
    NAME##_entry_allocator_delete(t->_allocator);			\
  }									\
  static inline ENTRY_TYPE* NAME##_get(struct NAME##_table_s*t, nm_core_tag_t tag) \
  {									\
    const void*key = (const void*)&tag;					\
    ENTRY_TYPE*e = puk_hashtable_lookup(t->_h, key);			\
    if(tbx_unlikely(e == NULL))						\
      {									\
	struct NAME##_entry_s*entry = NAME##_entry_malloc(t->_allocator); \
	key = &entry->_tag;						\
	e   = &entry->_data;						\
	NAME##_ctor(&entry->_data, tag);				\
	entry->_tag = tag;						\
	puk_hashtable_insert(t->_h, key, e);				\
      }									\
    return e;								\
  }

#endif /* NM_TAGS_AS_INDIRECT_HASH */


#endif /* NM_TAGS_H */
