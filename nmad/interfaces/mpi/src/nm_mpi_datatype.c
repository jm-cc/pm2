/*
 * NewMadeleine
 * Copyright (C) 2014-2016 (see AUTHORS file)
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

#include "nm_mpi_private.h"
#include <assert.h>
#include <errno.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(MadMPI);

NM_MPI_HANDLE_TYPE(datatype, nm_mpi_datatype_t, _NM_MPI_DATATYPE_OFFSET, 64);

static struct nm_mpi_handle_datatype_s nm_mpi_datatypes;

/* ** Hashtable types definition *************************** */

struct nm_mpi_datatype_hash_key_s
{
  uint32_t hash; /**< MPI_Datatype hash */
};
struct nm_mpi_datatype_hash_entry_s
{
  struct nm_mpi_datatype_hash_key_s*key;
  nm_mpi_datatype_t*p_datatype; /**< pointer to datatype representation */
};

static struct {
  nm_mpi_spinlock_t lock;
  puk_hashtable_t table;
} hashed_datatypes;

static inline int      nm_mpi_datatype_hashtable_init(void);
static inline int      nm_mpi_datatype_hashtable_destroy(void);
static        uint32_t nm_mpi_datatype_hash(const void*_datatype);
static inline uint32_t nm_mpi_datatype_hash_common(const nm_mpi_datatype_t*p_datatype);
static        int      nm_mpi_datatype_eq(const void*_datatype1, const void*_datatype2);
static inline void     nm_mpi_datatype_hashtable_insert(nm_mpi_datatype_t*p_datatype);
static inline void     nm_mpi_datatype_hashtable_remove_hash(uint32_t hash);

/* ** Hashtable types definition *************************** */

struct nm_mpi_datatype_exchange_hash_key_s
{
  nm_gate_t target;
  MPI_Datatype datatype; /**< local MPI_Datatype id */
};
struct nm_mpi_datatype_exchange_hash_entry_s
{
  struct nm_mpi_datatype_exchange_hash_key_s*key;
  uint32_t hash;
};

static struct {
  nm_mpi_spinlock_t lock;
  puk_hashtable_t table;
} exchanged_datatypes;

static inline int      nm_mpi_datatype_exchange_hashtable_init(void);
static inline int      nm_mpi_datatype_exchange_hashtable_destroy(void);
static        uint32_t nm_mpi_datatype_exchange_hash(const void*_datatype);
static        int      nm_mpi_datatype_exchange_eq(const void*_datatype1, const void*_datatype2);
static inline void     nm_mpi_datatype_exchange_insert(nm_gate_t target, MPI_Datatype datatype, uint32_t hash);
static inline void     nm_mpi_datatype_exchange_remove(nm_gate_t target, MPI_Datatype datatype);

/* ** Datatype exchanging interface ************************ */

static        void     nm_mpi_datatype_request_recv(nm_sr_event_t event, const nm_sr_event_info_t*info,
						    void*ref);
static        void     nm_mpi_datatype_request_monitor(nm_sr_event_t event,
						       const nm_sr_event_info_t*info, void*ref);
static        void     nm_mpi_datatype_request_free(nm_sr_event_t event, const nm_sr_event_info_t*info,
						    void*ref);

static struct nm_sr_monitor_s nm_mpi_datatype_requests_monitor = 
  (struct nm_sr_monitor_s) { .p_notifier = &nm_mpi_datatype_request_recv,
			     .event_mask = NM_SR_EVENT_RECV_UNEXPECTED,
			     .p_gate     = NM_ANY_GATE,
			     .tag        = NM_MPI_TAG_PRIVATE_TYPE_ADD,
			     .tag_mask   = NM_MPI_TAG_PRIVATE_BASE | 0x0F };

/** store builtin datatypes */
static void nm_mpi_datatype_store(int id, size_t size, int elements, const char*name);
static void nm_mpi_datatype_free(nm_mpi_datatype_t*p_datatype);
static void nm_mpi_datatype_properties_compute(nm_mpi_datatype_t*p_datatype);
static void nm_mpi_datatype_update_bounds(int blocklength, MPI_Aint displacement, nm_mpi_datatype_t*p_oldtype, nm_mpi_datatype_t*p_newtype);
static void nm_mpi_datatype_traversal_apply(const void*_content, nm_data_apply_t apply, void*_context);
static void nm_mpi_datatype_wrapper_traversal_apply(const void*_content, nm_data_apply_t apply, void*_context);
static void nm_mpi_datatype_serial_traversal_apply(const void*_content, nm_data_apply_t apply, void*_context);
static void nm_mpi_datatype_data_properties_compute(struct nm_data_s*p_data);

const struct nm_data_ops_s nm_mpi_datatype_ops =
  {
    .p_traversal          = &nm_mpi_datatype_traversal_apply,
    .p_properties_compute = &nm_mpi_datatype_data_properties_compute
  };

const struct nm_data_ops_s nm_mpi_datatype_wrapper_ops =
  {
    .p_traversal = &nm_mpi_datatype_wrapper_traversal_apply
  };

const struct nm_data_ops_s nm_mpi_datatype_serialize_ops =
  {
    .p_traversal = &nm_mpi_datatype_serial_traversal_apply
  };

/* ********************************************************* */

NM_MPI_ALIAS(MPI_Type_size,                  mpi_type_size);
NM_MPI_ALIAS(MPI_Type_size_x,                mpi_type_size_x);
NM_MPI_ALIAS(MPI_Type_get_extent,            mpi_type_get_extent);
NM_MPI_ALIAS(MPI_Type_get_extent_x,          mpi_type_get_extent_x);
NM_MPI_ALIAS(MPI_Type_get_true_extent,       mpi_type_get_true_extent);
NM_MPI_ALIAS(MPI_Type_get_true_extent_x,     mpi_type_get_true_extent_x);
NM_MPI_ALIAS(MPI_Type_extent,                mpi_type_extent);
NM_MPI_ALIAS(MPI_Type_lb,                    mpi_type_lb);
NM_MPI_ALIAS(MPI_Type_ub,                    mpi_type_ub);
NM_MPI_ALIAS(MPI_Type_dup,                   mpi_type_dup);
NM_MPI_ALIAS(MPI_Type_create_resized,        mpi_type_create_resized);
NM_MPI_ALIAS(MPI_Type_commit,                mpi_type_commit);
NM_MPI_ALIAS(MPI_Type_free,                  mpi_type_free);
NM_MPI_ALIAS(MPI_Type_contiguous,            mpi_type_contiguous);
NM_MPI_ALIAS(MPI_Type_vector,                mpi_type_vector);
NM_MPI_ALIAS(MPI_Type_hvector,               mpi_type_hvector);
NM_MPI_ALIAS(MPI_Type_indexed,               mpi_type_indexed);
NM_MPI_ALIAS(MPI_Type_hindexed,              mpi_type_hindexed);
NM_MPI_ALIAS(MPI_Type_struct,                mpi_type_struct);
NM_MPI_ALIAS(MPI_Type_create_hvector,        mpi_type_hvector);
NM_MPI_ALIAS(MPI_Type_create_hindexed,       mpi_type_create_hindexed);
NM_MPI_ALIAS(MPI_Type_create_indexed_block,  mpi_type_create_indexed_block);
NM_MPI_ALIAS(MPI_Type_create_hindexed_block, mpi_type_create_hindexed_block);
NM_MPI_ALIAS(MPI_Type_create_subarray,       mpi_type_create_subarray);
NM_MPI_ALIAS(MPI_Type_create_darray,         mpi_type_create_darray);
NM_MPI_ALIAS(MPI_Type_create_struct,         mpi_type_create_struct);
NM_MPI_ALIAS(MPI_Type_get_envelope,          mpi_type_get_envelope);
NM_MPI_ALIAS(MPI_Type_get_contents,          mpi_type_get_contents);
NM_MPI_ALIAS(MPI_Type_set_name,              mpi_type_set_name);
NM_MPI_ALIAS(MPI_Type_get_name,              mpi_type_get_name);
NM_MPI_ALIAS(MPI_Pack,                       mpi_pack);
NM_MPI_ALIAS(MPI_Unpack,                     mpi_unpack);
NM_MPI_ALIAS(MPI_Pack_size,                  mpi_pack_size);

/* ********************************************************* */

__PUK_SYM_INTERNAL
void nm_mpi_datatype_init(void)
{
  nm_mpi_handle_datatype_init(&nm_mpi_datatypes);

  /** Initialize hashed datatypes <-> datatype id hashtable */
  nm_mpi_datatype_hashtable_init();

  /** Initialize exchanged datatypes hashtable */
  nm_mpi_datatype_exchange_hashtable_init();

  /* Initialize the asynchronous mecanism for datatype exchange */
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(MPI_COMM_WORLD);
  nm_session_t       p_session = nm_mpi_communicator_get_session(p_comm);
  nm_sr_session_monitor_set(p_session, &nm_mpi_datatype_requests_monitor);
  
  /* Initialise the basic datatypes */

  /* C types */
  nm_mpi_datatype_store(MPI_CHAR,               sizeof(char), 1, "MPI_CHAR");
  nm_mpi_datatype_store(MPI_UNSIGNED_CHAR,      sizeof(unsigned char), 1, "MPI_UNSIGNED_CHAR");
  nm_mpi_datatype_store(MPI_SIGNED_CHAR,        sizeof(signed char), 1, "MPI_SIGNED_CHAR");
  nm_mpi_datatype_store(MPI_BYTE,               1, 1, "MPI_BYTE");
  nm_mpi_datatype_store(MPI_SHORT,              sizeof(signed short), 1, "MPI_SHORT");
  nm_mpi_datatype_store(MPI_UNSIGNED_SHORT,     sizeof(unsigned short), 1, "MPI_UNSIGNED_INT");
  nm_mpi_datatype_store(MPI_INT,                sizeof(signed int), 1, "MPI_INT");
  nm_mpi_datatype_store(MPI_UNSIGNED,           sizeof(unsigned int), 1, "MPI_UNSIGNED");
  nm_mpi_datatype_store(MPI_LONG,               sizeof(signed long), 1, "MPI_LONG");
  nm_mpi_datatype_store(MPI_UNSIGNED_LONG,      sizeof(unsigned long), 1, "MPI_UNSIGNED_LONG");
  nm_mpi_datatype_store(MPI_FLOAT,              sizeof(float), 1, "MPI_FLOAT");
  nm_mpi_datatype_store(MPI_DOUBLE,             sizeof(double), 1, "MPI_DOUBLE");
  nm_mpi_datatype_store(MPI_LONG_DOUBLE,        sizeof(long double), 1, "MPI_LONG_DOUBLE");
  nm_mpi_datatype_store(MPI_LONG_LONG_INT,      sizeof(long long int), 1, "MPI_LONG_LONG_INT");
  nm_mpi_datatype_store(MPI_UNSIGNED_LONG_LONG, sizeof(unsigned long long int), 1, "MPI_UNSIGNED_LONG_LONG");

  /* MPI-2 C additions */
  nm_mpi_datatype_store(MPI_WCHAR,              sizeof(wchar_t), 1, "MPI_WCHAR");
  nm_mpi_datatype_store(MPI_INT8_T,             sizeof(int8_t), 1, "MPI_INT8_T");
  nm_mpi_datatype_store(MPI_INT16_T,            sizeof(int16_t), 1, "MPI_INT16_T");
  nm_mpi_datatype_store(MPI_INT32_T,            sizeof(int32_t), 1, "MPI_INT32_T");
  nm_mpi_datatype_store(MPI_INT64_T,            sizeof(int64_t), 1, "MPI_INT64_T");
  nm_mpi_datatype_store(MPI_UINT8_T,            sizeof(uint8_t), 1, "MPI_UINT8_T");
  nm_mpi_datatype_store(MPI_UINT16_T,           sizeof(uint16_t), 1, "MPI_UINT16_T");
  nm_mpi_datatype_store(MPI_UINT32_T,           sizeof(uint32_t), 1, "MPI_UINT32_T");
  nm_mpi_datatype_store(MPI_UINT64_T,           sizeof(uint64_t), 1, "MPI_UINT64_T");
  nm_mpi_datatype_store(MPI_AINT,               sizeof(MPI_Aint), 1, "MPI_AINT");
  nm_mpi_datatype_store(MPI_OFFSET,             sizeof(MPI_Offset), 1, "MPI_OFFSET");
  nm_mpi_datatype_store(MPI_C_BOOL,             sizeof(_Bool), 1, "MPI_C_BOOLD");
  nm_mpi_datatype_store(MPI_C_FLOAT_COMPLEX,    sizeof(float _Complex), 1, "MPI_C_FLOAT_COMPLEX");
  nm_mpi_datatype_store(MPI_C_DOUBLE_COMPLEX,   sizeof(double _Complex), 1, "MPI_C_DOUBLE_COMPLEX");
  nm_mpi_datatype_store(MPI_C_LONG_DOUBLE_COMPLEX, sizeof(long double _Complex), 1, "MPI_C_LONG_DOUBLE_COMPLEX");
  
  /* FORTRAN types */
  nm_mpi_datatype_store(MPI_CHARACTER,          sizeof(char), 1, "MPI_CHARACTER");
  nm_mpi_datatype_store(MPI_LOGICAL,            sizeof(float), 1, "MPI_LOGICAL");
  nm_mpi_datatype_store(MPI_REAL,               sizeof(float), 1, "MPI_REAL");
  nm_mpi_datatype_store(MPI_REAL2,              2, 1, "MPI_REAL2");
  nm_mpi_datatype_store(MPI_REAL4,              4, 1, "MPI_REAL4");
  nm_mpi_datatype_store(MPI_REAL8,              8, 1, "MPI_REAL8");
  nm_mpi_datatype_store(MPI_REAL16,             16, 1, "MPI_REAL16");
  nm_mpi_datatype_store(MPI_DOUBLE_PRECISION,   sizeof(double), 1, "MPI_DOUBLE_PRECISION");
  nm_mpi_datatype_store(MPI_INTEGER,            sizeof(float), 1, "MPI_INTEGER");
  nm_mpi_datatype_store(MPI_INTEGER1,           1, 1, "MPI_INTEGER1");
  nm_mpi_datatype_store(MPI_INTEGER2,           2, 1, "MPI_INTEGER2");
  nm_mpi_datatype_store(MPI_INTEGER4,           4, 1, "MPI_INTEGER4");
  nm_mpi_datatype_store(MPI_INTEGER8,           8, 1, "MPI_INTEGER8");
  nm_mpi_datatype_store(MPI_INTEGER16,          16, 1, "MPI_INTEGER16");
  nm_mpi_datatype_store(MPI_PACKED,             sizeof(char), 1, "MPI_PACKED");

  /* FORTRAN COMPLEX types */
  nm_mpi_datatype_store(MPI_COMPLEX,            sizeof(complex float), 1, "MPI_COMPLEX");
  nm_mpi_datatype_store(MPI_COMPLEX4,           8, 1, "MPI_COMPLEX4");
  nm_mpi_datatype_store(MPI_COMPLEX8,           16, 1, "MPI_COMPLEX8");
  nm_mpi_datatype_store(MPI_COMPLEX16,          32, 1, "MPI_COMPLEX16");
  nm_mpi_datatype_store(MPI_COMPLEX32,          64, 1, "MPI_COMPLEX32");
  nm_mpi_datatype_store(MPI_DOUBLE_COMPLEX,     sizeof(complex double), 1, "MPI_DOUBLE_COMPLEX");

  /* C struct types */
  nm_mpi_datatype_store(MPI_2INT,               sizeof(struct { int    val; int loc; }), 2, "MPI_2INT");
  nm_mpi_datatype_store(MPI_SHORT_INT,          sizeof(struct { short  val; int loc; }), 2, "MPI_SHORT_INT");
  nm_mpi_datatype_store(MPI_LONG_INT,           sizeof(struct { long   val; int loc; }), 2, "MPI_LONG_INT");
  nm_mpi_datatype_store(MPI_FLOAT_INT,          sizeof(struct { float  val; int loc; }), 2, "MPI_FLOAT_INT");
  nm_mpi_datatype_store(MPI_DOUBLE_INT,         sizeof(struct { double val; int loc; }), 2, "MPI_DOUBLE_INT");
  nm_mpi_datatype_store(MPI_LONG_DOUBLE_INT,    sizeof(struct { long double val; int loc; }), 2, "MPI_LONG_DOUBLE_INT");

  /* FORTRAN struct types */
  nm_mpi_datatype_store(MPI_2INTEGER,           sizeof(float) + sizeof(float), 2, "MPI_2INTEGER");
  nm_mpi_datatype_store(MPI_2REAL,              sizeof(float) + sizeof(float), 2, "MPI_2REAL");
  nm_mpi_datatype_store(MPI_2DOUBLE_PRECISION,  sizeof(double) + sizeof(double), 2, "MPI_2DOUBLE_PRECISION");
  /* MPI-3 */
  nm_mpi_datatype_store(MPI_COUNT,              sizeof(MPI_Count), 1, "MPI_COUNT");

  /* TODO- Fortran types: 2COMPLEX, 2DOUBLE_COMPLEX */
  nm_mpi_datatype_store(MPI_UB,                 0, 0, "MPI_UB");
  nm_mpi_datatype_store(MPI_LB,                 0, 0, "MPI_LB");
}

__PUK_SYM_INTERNAL
void nm_mpi_datatype_exit(void)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(MPI_COMM_WORLD);
  nm_session_t       p_session = nm_mpi_communicator_get_session(p_comm);
  nm_sr_session_monitor_remove(p_session, &nm_mpi_datatype_requests_monitor);
  nm_mpi_datatype_hashtable_destroy();
  nm_mpi_datatype_exchange_hashtable_destroy();
  nm_mpi_handle_datatype_finalize(&nm_mpi_datatypes, &nm_mpi_datatype_free);
}

/* ********************************************************* */

/** store a basic datatype */
static void nm_mpi_datatype_store(int id, size_t size, int count, const char*name)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_handle_datatype_store(&nm_mpi_datatypes, id);
  /* initialize known properties */
  p_datatype->committed = 0;
  p_datatype->is_compact = 0;
  p_datatype->is_contig = 0;
  p_datatype->combiner = MPI_COMBINER_NAMED;
  p_datatype->refcount = 2;
  p_datatype->lb = 0;
  p_datatype->size = size;
  p_datatype->count = count;
  p_datatype->elements = count;
  p_datatype->extent = size;
  p_datatype->name = strdup(name);
  p_datatype->true_lb = 0;
  p_datatype->true_extent = size;
  p_datatype->ser_size = 0;
  p_datatype->p_serialized = NULL;
  p_datatype->hash = nm_mpi_datatype_hash_common(p_datatype) + puk_hash_oneatatime((void*)&id, sizeof(int));
  nm_mpi_datatype_hashtable_insert(p_datatype);
  /* compute properties through traversal */
  nm_mpi_datatype_properties_compute(p_datatype);
  /* check computed values consistency*/
  assert(p_datatype->is_contig);
  assert(p_datatype->props.size == size);
  assert(p_datatype->props.blocks == 1);
}

/** allocate a new datatype and init with default values */
static nm_mpi_datatype_t*nm_mpi_datatype_alloc(nm_mpi_type_combiner_t combiner, size_t size, int count)
{
  nm_mpi_datatype_t*p_newtype = nm_mpi_handle_datatype_alloc(&nm_mpi_datatypes);
  p_newtype->combiner = combiner;
  p_newtype->committed = 0;
  p_newtype->is_compact = 0;
  p_newtype->size = size;
  p_newtype->count = count;
  p_newtype->elements = count;
  p_newtype->lb = MPI_UNDEFINED;
  p_newtype->extent = MPI_UNDEFINED;
  p_newtype->refcount = 1;
  p_newtype->is_contig = 0;
  p_newtype->name = NULL;
  p_newtype->attrs = NULL;
  p_newtype->true_lb = MPI_UNDEFINED;
  p_newtype->true_extent = MPI_UNDEFINED;
  p_newtype->ser_size = sizeof(nm_mpi_datatype_ser_t) - sizeof(union nm_mpi_datatype_ser_param_u);
  p_newtype->p_serialized = NULL;
  p_newtype->hash = 0;
  return p_newtype;
}

/* ********************************************************* */
/* ** Datatype hash table management *********************** */

static inline int nm_mpi_datatype_hashtable_init(void)
{
  hashed_datatypes.table = puk_hashtable_new(&nm_mpi_datatype_hash, &nm_mpi_datatype_eq);
  nm_mpi_spin_init(&hashed_datatypes.lock);
  return MPI_SUCCESS;
}

static inline int nm_mpi_datatype_hashtable_destroy(void)
{
  nm_mpi_spin_lock(&hashed_datatypes.lock);
  struct nm_mpi_datatype_hash_entry_s*e;
  struct nm_mpi_datatype_hash_key_s  *k;
  do
    {
      puk_hashtable_getentry(hashed_datatypes.table, &k, &e);
      if(k != NULL)
	{
	  puk_hashtable_remove(hashed_datatypes.table, k);
	  FREE_AND_SET_NULL(e->key);
	  FREE_AND_SET_NULL(e);
	}
    }
  while(k != NULL);
  nm_mpi_spin_unlock(&hashed_datatypes.lock);
  puk_hashtable_delete(hashed_datatypes.table);
  return MPI_SUCCESS;
}

static uint32_t nm_mpi_datatype_hash(const void*_datatype)
{
  const struct nm_mpi_datatype_hash_key_s*d = _datatype;
  return d->hash;
}

static inline uint32_t nm_mpi_datatype_hash_common(const nm_mpi_datatype_t*p_datatype)
{
  uint32_t z = puk_hash_oneatatime((const void*)&p_datatype->combiner, sizeof(nm_mpi_type_combiner_t))
    + puk_hash_oneatatime((const void*)&p_datatype->count, sizeof(MPI_Count))
    + puk_hash_oneatatime((const void*)&p_datatype->elements, sizeof(MPI_Count))
    + puk_hash_oneatatime((const void*)&p_datatype->is_contig, sizeof(int))
    + puk_hash_oneatatime((const void*)&p_datatype->lb, sizeof(MPI_Aint))
    + puk_hash_oneatatime((const void*)&p_datatype->extent, sizeof(MPI_Aint))
    + puk_hash_oneatatime((const void*)&p_datatype->size, sizeof(size_t));
  return z;
}

static int nm_mpi_datatype_eq(const void*_datatype1, const void*_datatype2)
{
  const struct nm_mpi_datatype_hash_key_s*d1 = _datatype1;
  const struct nm_mpi_datatype_hash_key_s*d2 = _datatype2;
  const int eq = (d1->hash == d2->hash);
  return eq;
}

static inline void nm_mpi_datatype_hashtable_insert(nm_mpi_datatype_t*p_datatype)
{
  struct nm_mpi_datatype_hash_entry_s*e;
  struct nm_mpi_datatype_hash_key_s*key = malloc(sizeof(struct nm_mpi_datatype_hash_key_s));
  key->hash = p_datatype->hash;
  nm_mpi_spin_lock(&hashed_datatypes.lock);
  e = puk_hashtable_lookup(hashed_datatypes.table, key);
  if(NULL == e)
    {
      e = malloc(sizeof(struct nm_mpi_datatype_hash_entry_s));
      e->key = key;
      e->p_datatype = p_datatype;
      puk_hashtable_insert(hashed_datatypes.table, key, e);
    }
  else
    {
      FREE_AND_SET_NULL(key);
    }
  nm_mpi_spin_unlock(&hashed_datatypes.lock);
}

static inline void nm_mpi_datatype_hashtable_remove_hash(uint32_t hash)
{
  struct nm_mpi_datatype_hash_entry_s*e;
  struct nm_mpi_datatype_hash_key_s key = (struct nm_mpi_datatype_hash_key_s){ .hash = hash };
  nm_mpi_spin_lock(&hashed_datatypes.lock);
  e = puk_hashtable_lookup(hashed_datatypes.table, &key);
  puk_hashtable_remove(hashed_datatypes.table, &key);
  nm_mpi_spin_unlock(&hashed_datatypes.lock);
  nm_mpi_datatype_unlock(e->p_datatype);
  FREE_AND_SET_NULL(e->key);
  FREE_AND_SET_NULL(e);
}

__PUK_SYM_INTERNAL
nm_mpi_datatype_t*nm_mpi_datatype_hashtable_get(uint32_t datatype_hash)
{
  struct nm_mpi_datatype_hash_entry_s*e;
  struct nm_mpi_datatype_hash_key_s key = (struct nm_mpi_datatype_hash_key_s){ .hash = datatype_hash };
  nm_mpi_spin_lock(&hashed_datatypes.lock);
  e = puk_hashtable_lookup(hashed_datatypes.table, &key);
  nm_mpi_spin_unlock(&hashed_datatypes.lock);
  return e ? e->p_datatype : NULL;
}

/* ** Request monitoring functions for datatypes *********** */

static void nm_mpi_datatype_request_monitor(nm_sr_event_t event, const nm_sr_event_info_t*info, void*ref)
{
  /* Retrieve parameters */
  assert(ref);
  nm_mpi_request_t  *p_req = (nm_mpi_request_t*)ref;
  nm_sr_request_t*p_nm_req = info->req.p_request;
  nm_tag_t tag = 0;
  nm_sr_request_get_tag(p_nm_req, &tag);
  assert(tag);
  nm_session_t   p_session = NULL;
  nm_sr_request_get_session(p_nm_req, &p_session);
  assert(p_session);
  /* Deserialize the datatypes */
  void*tmp;
  const void*end = p_req->rbuf + p_req->count;
  MPI_Datatype new_datatype;
  nm_mpi_datatype_ser_t*p_ser_datatype;
  for(tmp = p_req->rbuf; tmp < end; tmp += p_ser_datatype->ser_size)
    {
      p_ser_datatype = tmp;
      nm_mpi_datatype_deserialize(p_ser_datatype, &new_datatype);
    }
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(new_datatype);
  assert(p_datatype);
  nm_mpi_datatype_exchange_insert(p_req->gate, p_datatype->id, p_datatype->hash);
  /* Send back a echo to unlock */
  tag += 1; /* Keeps the sequence number */
  nm_mpi_request_t        *p_req_resp = nm_mpi_request_alloc();
  p_req_resp->gate                    = p_req->gate;
  p_req_resp->rbuf                    = NULL;
  p_req_resp->count                   = 0;
  p_req_resp->user_tag                = tag;
  p_req_resp->p_datatype              = nm_mpi_datatype_get(MPI_BYTE);
  p_req_resp->request_type            = NM_MPI_REQUEST_RECV;
  p_req_resp->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req_resp->request_persistent_type = NM_MPI_REQUEST_ZERO;
  nm_sr_send_init(p_session, &p_req_resp->request_nmad);
  nm_sr_send_pack_contiguous(p_session, &p_req_resp->request_nmad, NULL, 0);
  nm_sr_request_set_ref(&p_req_resp->request_nmad, p_req_resp);
  nm_sr_request_monitor(p_session, &p_req_resp->request_nmad, NM_SR_EVENT_FINALIZED,
			&nm_mpi_datatype_request_free);
  nm_sr_send_isend(p_session, &p_req_resp->request_nmad, p_req_resp->gate, tag);
  FREE_AND_SET_NULL(p_req->rbuf);
  nm_mpi_request_free(p_req);  
}

static void nm_mpi_datatype_request_recv(nm_sr_event_t event, const nm_sr_event_info_t*info, void*ref)
{
  const nm_tag_t             tag = info->recv_unexpected.tag;
  if(NM_MPI_TAG_PRIVATE_TYPE_ADD == (tag & (NM_MPI_TAG_PRIVATE_BASE | 0x0F))) {
    const nm_gate_t           from = info->recv_unexpected.p_gate;      
    const size_t               len = info->recv_unexpected.len;
    const nm_session_t   p_session = info->recv_unexpected.p_session;
    nm_mpi_request_t        *p_req = nm_mpi_request_alloc();
    p_req->gate                    = from;
    p_req->rbuf                    = malloc(len);
    p_req->count                   = len;
    p_req->user_tag                = tag;
    p_req->p_datatype              = nm_mpi_datatype_get(MPI_BYTE);
    p_req->request_type            = NM_MPI_REQUEST_RECV;
    p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
    p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
    nm_sr_recv_init(p_session, &p_req->request_nmad);
    nm_sr_recv_unpack_contiguous(p_session, &p_req->request_nmad, p_req->rbuf, p_req->count);
    nm_sr_request_set_ref(&p_req->request_nmad, p_req);
    nm_sr_request_monitor(p_session, &p_req->request_nmad, NM_SR_EVENT_FINALIZED,
			  &nm_mpi_datatype_request_monitor);
    nm_sr_recv_match_event(p_session, &p_req->request_nmad, info);
    nm_sr_recv_post(p_session, &p_req->request_nmad);
  } else /* ERROR */
    NM_MPI_FATAL_ERROR("How did you get here? You shouldn't have...\n");
}

__PUK_SYM_INTERNAL
int nm_mpi_datatype_send(nm_gate_t gate, nm_mpi_datatype_t*p_datatype)
{
  assert(gate);
  assert(p_datatype);
  if(p_datatype->id < _NM_MPI_DATATYPE_OFFSET)
    {
      return MPI_SUCCESS;
    }
  struct nm_mpi_datatype_exchange_hash_entry_s*entry;
  struct nm_mpi_datatype_exchange_hash_key_s key = (struct nm_mpi_datatype_exchange_hash_key_s)
    { .target = gate, .datatype = p_datatype->id };
  nm_mpi_spin_lock(&exchanged_datatypes.lock);
  entry = puk_hashtable_lookup(exchanged_datatypes.table, &key);
  nm_mpi_spin_unlock(&exchanged_datatypes.lock);
  if(entry && entry->hash == p_datatype->hash){
    return MPI_SUCCESS;
  }
  nm_mpi_datatype_exchange_insert(gate, p_datatype->id, p_datatype->hash);
  static uint16_t sequence = 0;
  uint16_t seq = __sync_fetch_and_add(&sequence, 1);
  int err = MPI_SUCCESS;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(MPI_COMM_WORLD);
  nm_session_t       p_session = nm_mpi_communicator_get_session(p_comm);
  nm_sr_request_t req;
  struct nm_data_s data;
  /* Send datatypes */
  nm_data_mpi_datatype_serial_set(&data, p_datatype);
  nm_tag_t nm_tag = NM_MPI_TAG_PRIVATE_TYPE_ADD | nm_mpi_rma_seq_to_tag(seq);
  nm_sr_send_init(p_session, &req);
  nm_sr_send_pack_data(p_session, &req, &data);
  err = nm_sr_send_isend(p_session, &req, gate, nm_tag);
  nm_sr_swait(p_session, &req);
  /* Recieve echo */
  nm_tag += 1; /* Keeps the sequence number */
  nm_sr_recv_init(p_session, &req);
  nm_sr_recv_unpack_contiguous(p_session, &req, NULL, 0);
  err = nm_sr_recv_irecv(p_session, &req, gate, nm_tag, NM_MPI_TAG_PRIVATE_BASE | 0xFFFF0F);
  nm_sr_rwait(p_session, &req);
  /* Insert into exchanged hashtable */
  nm_mpi_datatype_exchange_insert(gate, p_datatype->id, p_datatype->hash);
  return err;
}

static void nm_mpi_datatype_request_free(nm_sr_event_t event, const nm_sr_event_info_t*info, void*ref)
{
  if(ref)
    nm_mpi_request_free(ref);
}

/* ** Exchanged datatypes hashtable functions ************** */

static inline int nm_mpi_datatype_exchange_hashtable_init(void)
{
  exchanged_datatypes.table = puk_hashtable_new(&nm_mpi_datatype_exchange_hash,
						&nm_mpi_datatype_exchange_eq);
  nm_mpi_spin_init(&exchanged_datatypes.lock);
  return MPI_SUCCESS;
}

static inline int nm_mpi_datatype_exchange_hashtable_destroy(void)
{
  nm_mpi_spin_lock(&exchanged_datatypes.lock);
  struct nm_mpi_datatype_exchange_hash_entry_s*e;
  struct nm_mpi_datatype_exchange_hash_key_s  *k;
  for(puk_hashtable_getentry(exchanged_datatypes.table, &k, &e);
      puk_hashtable_size(exchanged_datatypes.table);
      puk_hashtable_getentry(exchanged_datatypes.table, &k, &e)) {
    puk_hashtable_remove(exchanged_datatypes.table, k);
    FREE_AND_SET_NULL(e->key);
    FREE_AND_SET_NULL(e);
  }
  nm_mpi_spin_unlock(&exchanged_datatypes.lock);
  puk_hashtable_delete(exchanged_datatypes.table);
  return MPI_SUCCESS;
}

static uint32_t nm_mpi_datatype_exchange_hash(const void*_datatype)
{
  const struct nm_mpi_datatype_exchange_hash_key_s*d = _datatype;
  uint32_t z = puk_hash_oneatatime((const void*)&d->target, sizeof(nm_gate_t)) +
    puk_hash_oneatatime((const void*)&d->datatype, sizeof(MPI_Datatype));
  return z;
}

static int nm_mpi_datatype_exchange_eq(const void*_datatype1, const void*_datatype2)
{
  const struct nm_mpi_datatype_exchange_hash_key_s*d1 = _datatype1;
  const struct nm_mpi_datatype_exchange_hash_key_s*d2 = _datatype2;
  const int eq = ((d1->target == d2->target) && (d1->datatype == d2->datatype));
  return eq;
}

static inline void nm_mpi_datatype_exchange_insert(nm_gate_t target, MPI_Datatype datatype, uint32_t hash)
{
  struct nm_mpi_datatype_exchange_hash_entry_s*e;
  struct nm_mpi_datatype_exchange_hash_key_s*key = malloc(sizeof(struct nm_mpi_datatype_exchange_hash_key_s));
  key->target = target;
  key->datatype = datatype;
  nm_mpi_spin_lock(&exchanged_datatypes.lock);
  e = puk_hashtable_lookup(exchanged_datatypes.table, key);
  if(NULL == e)
    {
      e = malloc(sizeof(struct nm_mpi_datatype_exchange_hash_entry_s));
      e->key = key;
      puk_hashtable_insert(exchanged_datatypes.table, key, e);
    }
  else
    {
      FREE_AND_SET_NULL(key);
    }
  nm_mpi_spin_unlock(&exchanged_datatypes.lock);
  e->hash = hash;
}

static inline void nm_mpi_datatype_exchange_remove(nm_gate_t target, MPI_Datatype datatype)
{
  struct nm_mpi_datatype_exchange_hash_entry_s*e;
  struct nm_mpi_datatype_exchange_hash_key_s key = (struct nm_mpi_datatype_exchange_hash_key_s)
    { .target = target, .datatype = datatype };
  nm_mpi_spin_lock(&exchanged_datatypes.lock);
  e = puk_hashtable_lookup(exchanged_datatypes.table, &key);
  puk_hashtable_remove(exchanged_datatypes.table, &key);
  nm_mpi_spin_unlock(&exchanged_datatypes.lock);
  FREE_AND_SET_NULL(e->key);
  FREE_AND_SET_NULL(e);
}

/* ********************************************************* */

int mpi_type_size(MPI_Datatype datatype, int *size)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *size = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  *size = p_datatype->size;
  return MPI_SUCCESS;
}

int mpi_type_size_x(MPI_Datatype datatype, MPI_Count*size)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *size = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  *size = p_datatype->size;
  return MPI_SUCCESS;
}

int mpi_type_get_extent(MPI_Datatype datatype, MPI_Aint*lb, MPI_Aint*extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *extent = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  if(lb != NULL)
    *lb = p_datatype->lb;
  if(extent != NULL)
    *extent = p_datatype->extent;
  return MPI_SUCCESS;
}

int mpi_type_get_extent_x(MPI_Datatype datatype, MPI_Count*lb, MPI_Count*extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *extent = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  if(lb != NULL)
    *lb = p_datatype->lb;
  if(extent != NULL)
    *extent = p_datatype->extent;
  return MPI_SUCCESS;
}
  
int mpi_type_extent(MPI_Datatype datatype, MPI_Aint*extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *extent = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  *extent = p_datatype->extent;
  return MPI_SUCCESS;
}

int mpi_type_get_true_extent(MPI_Datatype datatype,  MPI_Aint*true_lb, MPI_Aint*true_extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *true_extent = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  if(true_lb != NULL)
    *true_lb = p_datatype->true_lb;
  if(true_extent != NULL)
    *true_extent = p_datatype->true_extent;
  return MPI_SUCCESS;
}

int mpi_type_get_true_extent_x(MPI_Datatype datatype, MPI_Count*true_lb, MPI_Count*true_extent)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *true_extent = MPI_UNDEFINED;
      return MPI_ERR_TYPE;
    }
  if(true_lb != NULL)
    *true_lb = p_datatype->true_lb;
  if(true_extent != NULL)
    *true_extent = p_datatype->true_extent;
  return MPI_SUCCESS;
}

int mpi_type_lb(MPI_Datatype datatype,MPI_Aint*lb)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  *lb = p_datatype->lb;
  return MPI_SUCCESS;
}
int mpi_type_ub(MPI_Datatype datatype, MPI_Aint*displacement)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  *displacement = p_datatype->lb + p_datatype->extent;
  return MPI_SUCCESS;
}

int mpi_type_dup(MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  nm_mpi_datatype_t *p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_DUP, p_oldtype->size, 1);
  *newtype = p_newtype->id;
  p_newtype->is_contig = p_oldtype->is_contig;
  p_newtype->extent    = p_oldtype->extent;
  p_newtype->lb        = p_oldtype->lb;
  p_newtype->DUP.p_old_type = p_oldtype;
  p_newtype->ser_size += sizeof(struct nm_mpi_datatype_ser_param_dup_s);
  __sync_add_and_fetch(&p_oldtype->refcount, 1);
  if(p_oldtype->attrs)
    {
      int err = nm_mpi_attrs_copy(p_oldtype->id, p_oldtype->attrs, &p_newtype->attrs);
      if(err)
	{
	  nm_mpi_datatype_free(p_newtype);
	  *newtype = MPI_DATATYPE_NULL;
	  return err;
	}
    }
  p_newtype->hash  = nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += p_oldtype->hash;
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_DUP;
  p_newtype->p_serialized->count = 1;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  p_newtype->p_serialized->p.DUP.old_type = p_oldtype->hash;
  if(p_oldtype->committed)
    nm_mpi_datatype_properties_compute(p_newtype);
  return MPI_SUCCESS;
}

int mpi_type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_RESIZED, p_oldtype->size, 1);
  *newtype = p_newtype->id;
  p_newtype->is_contig = (p_oldtype->is_contig && (lb == 0) && (extent == p_oldtype->size));
  p_newtype->lb        = lb;
  p_newtype->extent    = extent;
  p_newtype->RESIZED.p_old_type = p_oldtype;
  p_newtype->ser_size += sizeof(struct nm_mpi_datatype_ser_param_resized_s);
  p_newtype->ser_size += p_oldtype->ser_size;
  __sync_add_and_fetch(&p_oldtype->refcount, 1);
  p_newtype->hash  = nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += p_oldtype->hash;
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_RESIZED;
  p_newtype->p_serialized->count = 1;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  p_newtype->p_serialized->p.RESIZED.old_type = p_oldtype->hash;
  p_newtype->p_serialized->p.RESIZED.lb = lb;
  p_newtype->p_serialized->p.RESIZED.extent = extent;
  return MPI_SUCCESS;
}

int mpi_type_commit(MPI_Datatype*datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  nm_mpi_datatype_properties_compute(p_datatype);
  assert(p_datatype->lb != MPI_UNDEFINED);
  assert(p_datatype->extent != MPI_UNDEFINED);
  p_datatype->committed = 1;
  return MPI_SUCCESS;
}

int mpi_type_free(MPI_Datatype*datatype)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(*datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  p_datatype->refcount--;
  *datatype = MPI_DATATYPE_NULL;
  if(p_datatype->refcount > 0)
    {
      return MPI_ERR_DATATYPE_ACTIVE;
    }
  else
    {
      nm_mpi_datatype_free(p_datatype);
      return MPI_SUCCESS;
    }
}

int mpi_type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_CONTIGUOUS, p_oldtype->size * count, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = p_oldtype->is_contig;
  p_newtype->lb = p_oldtype->lb;
  p_newtype->extent = p_oldtype->extent * count;
  p_newtype->CONTIGUOUS.p_old_type = p_oldtype;
  p_newtype->ser_size += sizeof(struct nm_mpi_datatype_ser_param_contiguous_s);
  __sync_add_and_fetch(&p_oldtype->refcount, 1);
  p_newtype->hash  = nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += p_oldtype->hash;
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_CONTIGUOUS;
  p_newtype->p_serialized->count = count;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  p_newtype->p_serialized->p.CONTIGUOUS.old_type = p_oldtype->hash;
  return MPI_SUCCESS;
}

int mpi_type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_VECTOR, p_oldtype->size * count * blocklength, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->lb = p_oldtype->lb;
  p_newtype->extent = p_oldtype->extent * blocklength + (count - 1) * stride * p_oldtype->extent;
  p_newtype->VECTOR.p_old_type = p_oldtype;
  p_newtype->VECTOR.stride = stride;
  p_newtype->VECTOR.blocklength = blocklength;
  p_newtype->ser_size += sizeof(struct nm_mpi_datatype_ser_param_vector_s);
  __sync_add_and_fetch(&p_oldtype->refcount, 1);
  p_newtype->hash  = nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += p_oldtype->hash;
  p_newtype->hash += puk_hash_oneatatime((const void*)&stride, sizeof(int));
  p_newtype->hash += puk_hash_oneatatime((const void*)&blocklength, sizeof(int));
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_VECTOR;
  p_newtype->p_serialized->count = count;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  p_newtype->p_serialized->p.VECTOR.old_type = p_oldtype->hash;
  p_newtype->p_serialized->p.VECTOR.stride = stride;
  p_newtype->p_serialized->p.VECTOR.blocklength = blocklength;
  return MPI_SUCCESS;
}

int mpi_type_hvector(int count, int blocklength, MPI_Aint hstride, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t *p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_HVECTOR, p_oldtype->size * count * blocklength, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->lb = p_oldtype->lb;
  p_newtype->extent = p_oldtype->extent * blocklength + (count - 1) * hstride;
  p_newtype->HVECTOR.p_old_type = p_oldtype;
  p_newtype->HVECTOR.hstride = hstride;
  p_newtype->HVECTOR.blocklength = blocklength;
  p_newtype->ser_size += sizeof(struct nm_mpi_datatype_ser_param_hvector_s);
  __sync_add_and_fetch(&p_oldtype->refcount, 1);
  p_newtype->hash  = nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += p_oldtype->hash;
  p_newtype->hash += puk_hash_oneatatime((const void*)&hstride, sizeof(MPI_Aint));
  p_newtype->hash += puk_hash_oneatatime((const void*)&blocklength, sizeof(int));
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_HVECTOR;
  p_newtype->p_serialized->count = count;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  p_newtype->p_serialized->p.HVECTOR.old_type = p_oldtype->hash;
  p_newtype->p_serialized->p.HVECTOR.hstride = hstride;
  p_newtype->p_serialized->p.HVECTOR.blocklength = blocklength;
  return MPI_SUCCESS;
}

int mpi_type_indexed(int count, int *array_of_blocklengths, int *array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int i;
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_INDEXED, 0, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->INDEXED.p_old_type = p_oldtype;
  p_newtype->INDEXED.p_map = malloc(count * sizeof(struct nm_mpi_type_indexed_map_s));
  for(i = 0; i < count ; i++)
    {
      p_newtype->INDEXED.p_map[i].blocklength = array_of_blocklengths[i];
      p_newtype->INDEXED.p_map[i].displacement = array_of_displacements[i];
      p_newtype->size += p_oldtype->size * array_of_blocklengths[i];
      nm_mpi_datatype_update_bounds(p_newtype->INDEXED.p_map[i].blocklength,
				    p_newtype->INDEXED.p_map[i].displacement * p_oldtype->extent,
				    p_oldtype, p_newtype);
    }
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  p_newtype->ser_size += sizeof(struct nm_mpi_datatype_ser_param_indexed_s);
  p_newtype->ser_size += count * 2 * sizeof(int);
  __sync_add_and_fetch(&p_oldtype->refcount, 1);
  p_newtype->hash  = nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += p_oldtype->hash;
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_blocklengths, count * sizeof(int));
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_displacements, count * sizeof(int));
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_INDEXED;
  p_newtype->p_serialized->count = count;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  p_newtype->p_serialized->p.INDEXED.old_type = p_oldtype->hash;
  void*tmp = &p_newtype->p_serialized->p.INDEXED.DATA;
  memcpy(tmp, array_of_blocklengths, count * sizeof(int));
  tmp += count * sizeof(int);
  memcpy(tmp, array_of_displacements, count * sizeof(int));
  return MPI_SUCCESS;
}

int mpi_type_hindexed(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype) 
{
  int i;
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_HINDEXED, 0, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->HINDEXED.p_old_type = p_oldtype;
  p_newtype->HINDEXED.p_map = malloc(count * sizeof(struct nm_mpi_type_hindexed_map_s));
  for(i = 0; i < count ; i++)
    {
      p_newtype->HINDEXED.p_map[i].blocklength = array_of_blocklengths[i];
      p_newtype->HINDEXED.p_map[i].displacement = array_of_displacements[i];
      p_newtype->size += p_oldtype->size * array_of_blocklengths[i];
      nm_mpi_datatype_update_bounds(p_newtype->HINDEXED.p_map[i].blocklength,
				    p_newtype->HINDEXED.p_map[i].displacement,
				    p_oldtype, p_newtype);
    }
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  p_newtype->ser_size += sizeof(struct nm_mpi_datatype_ser_param_hindexed_s);
  p_newtype->ser_size += count * (sizeof(int) + sizeof(MPI_Aint));
  __sync_add_and_fetch(&p_oldtype->refcount, 1);
  p_newtype->hash  = nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += p_oldtype->hash;
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_blocklengths, count * sizeof(int));
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_displacements, count * sizeof(MPI_Aint));
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_HINDEXED;
  p_newtype->p_serialized->count = count;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  p_newtype->p_serialized->p.HINDEXED.old_type = p_oldtype->hash;
  void*tmp = &p_newtype->p_serialized->p.INDEXED.DATA;
  memcpy(tmp, array_of_blocklengths, count * sizeof(int));
  tmp += count * sizeof(int);
  memcpy(tmp, array_of_displacements, count * sizeof(MPI_Aint));
  return MPI_SUCCESS;
}

int mpi_type_create_hindexed(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  return mpi_type_hindexed(count, (int*)array_of_blocklengths, (MPI_Aint*)array_of_displacements, oldtype, newtype);
}

int mpi_type_create_indexed_block(int count, int blocklength, const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_INDEXED_BLOCK, count * blocklength * p_oldtype->size, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->INDEXED_BLOCK.p_old_type = p_oldtype;
  p_newtype->INDEXED_BLOCK.blocklength = blocklength;
  p_newtype->INDEXED_BLOCK.array_of_displacements = malloc(count * sizeof(int));
  int i;
  for(i = 0; i < count ; i++)
    {
      p_newtype->INDEXED_BLOCK.array_of_displacements[i] = array_of_displacements[i];
      nm_mpi_datatype_update_bounds(p_newtype->INDEXED_BLOCK.blocklength,
				    p_newtype->INDEXED_BLOCK.array_of_displacements[i] * p_oldtype->extent,
				    p_oldtype, p_newtype);
    }
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  p_newtype->ser_size += sizeof(struct nm_mpi_datatype_ser_param_indexed_block_s);
  p_newtype->ser_size += count * sizeof(int);
  __sync_add_and_fetch(&p_oldtype->refcount, 1);
  p_newtype->hash  = nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += p_oldtype->hash;
  p_newtype->hash += puk_hash_oneatatime((const void*)&blocklength, sizeof(int));
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_displacements, count * sizeof(int));
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_INDEXED_BLOCK;
  p_newtype->p_serialized->count = count;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  p_newtype->p_serialized->p.INDEXED_BLOCK.old_type = p_oldtype->hash;
  p_newtype->p_serialized->p.INDEXED_BLOCK.blocklength = blocklength;
  void*tmp = &p_newtype->p_serialized->p.INDEXED_BLOCK.DATA;
  memcpy(tmp, array_of_displacements, count * sizeof(int));
  return MPI_SUCCESS;
}

int mpi_type_create_hindexed_block(int count, int blocklength, const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype*newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_HINDEXED_BLOCK, count * blocklength * p_oldtype->size, count);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->HINDEXED_BLOCK.p_old_type = p_oldtype;
  p_newtype->HINDEXED_BLOCK.blocklength = blocklength;
  p_newtype->HINDEXED_BLOCK.array_of_displacements = malloc(count * sizeof(int));
  int i;
  for(i = 0; i < count ; i++)
    {
      p_newtype->HINDEXED_BLOCK.array_of_displacements[i] = array_of_displacements[i];
      nm_mpi_datatype_update_bounds(p_newtype->HINDEXED_BLOCK.blocklength,
				    p_newtype->HINDEXED_BLOCK.array_of_displacements[i],
				    p_oldtype, p_newtype);
    }
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  p_newtype->ser_size += sizeof(struct nm_mpi_datatype_ser_param_hindexed_block_s);
  p_newtype->ser_size += count * sizeof(MPI_Aint);
  __sync_add_and_fetch(&p_oldtype->refcount, 1);
  p_newtype->hash  = nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += p_oldtype->hash;
  p_newtype->hash += puk_hash_oneatatime((const void*)&blocklength, sizeof(int));
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_displacements, count * sizeof(MPI_Aint));
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_HINDEXED_BLOCK;
  p_newtype->p_serialized->count = count;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  p_newtype->p_serialized->p.HINDEXED_BLOCK.old_type = p_oldtype->hash;
  p_newtype->p_serialized->p.HINDEXED_BLOCK.blocklength = blocklength;
  void*tmp = &p_newtype->p_serialized->p.INDEXED_BLOCK.DATA;
  memcpy(tmp, array_of_displacements, count * sizeof(MPI_Aint));
  return MPI_SUCCESS;
}

int mpi_type_struct(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype *array_of_types, MPI_Datatype *newtype)
{
  int i;
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_STRUCT, 0, count);
  *newtype = p_newtype->id;
  p_newtype->ser_size += count * (sizeof(int) + sizeof(MPI_Aint) + sizeof(MPI_Datatype));
  p_newtype->is_contig = 0;
  p_newtype->STRUCT.p_map = malloc(count * sizeof(struct nm_mpi_type_struct_map_s));
  uint32_t old_types_hashes[count];
  for(i = 0; i < count; i++)
    {
      nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(array_of_types[i]);
      if(p_datatype == NULL)
	{
	  nm_mpi_handle_datatype_free(&nm_mpi_datatypes, p_datatype);
	  *newtype = MPI_DATATYPE_NULL;
	  return MPI_ERR_TYPE;
	}
      p_newtype->STRUCT.p_map[i].p_old_type   = p_datatype;
      p_newtype->STRUCT.p_map[i].blocklength  = array_of_blocklengths[i];
      p_newtype->STRUCT.p_map[i].displacement = array_of_displacements[i];
      p_newtype->size += p_newtype->STRUCT.p_map[i].blocklength * p_datatype->size;
      __sync_add_and_fetch(&p_datatype->refcount, 1);
      nm_mpi_datatype_update_bounds(p_newtype->STRUCT.p_map[i].blocklength,
				    p_newtype->STRUCT.p_map[i].displacement,
				    p_newtype->STRUCT.p_map[i].p_old_type,
				    p_newtype);
      p_newtype->hash    += p_datatype->hash;
      old_types_hashes[i] = p_datatype->hash;
    }
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  p_newtype->hash += nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_blocklengths, count * sizeof(int));
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_displacements, count * sizeof(int));
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_STRUCT;
  p_newtype->p_serialized->count = count;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  void*tmp = &p_newtype->p_serialized->p.STRUCT.DATA;
  memcpy(tmp, old_types_hashes, count * sizeof(uint32_t));
  tmp += count * sizeof(uint32_t);
  memcpy(tmp, array_of_blocklengths, count * sizeof(int));
  tmp += count * sizeof(int);
  memcpy(tmp, array_of_displacements, count * sizeof(MPI_Aint));
  return MPI_SUCCESS;
}

int mpi_type_create_subarray(int ndims, const int array_of_sizes[], const int array_of_subsizes[], const int array_of_starts[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_get(oldtype);
  if(p_oldtype == NULL)
    {
      *newtype = MPI_DATATYPE_NULL;
      return MPI_ERR_TYPE;
    }
  __sync_add_and_fetch(&p_oldtype->refcount, 1);
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_alloc(MPI_COMBINER_SUBARRAY, p_oldtype->size, 1);
  *newtype = p_newtype->id;
  p_newtype->is_contig = 0;
  p_newtype->SUBARRAY.p_dims = malloc(ndims * sizeof(struct nm_mpi_type_subarray_dim_s));
  p_newtype->SUBARRAY.ndims = ndims;
  p_newtype->SUBARRAY.order = order;
  p_newtype->SUBARRAY.p_old_type = p_oldtype;
  MPI_Count elements = 1;
  MPI_Aint last_offset = 0, first_offset = 0;
  MPI_Count blocklength = 1;
  int d;
  for(d = ndims - 1; d >= 0; d--)
    {
      p_newtype->SUBARRAY.p_dims[d].size     = array_of_sizes[d];
      p_newtype->SUBARRAY.p_dims[d].subsize  = array_of_subsizes[d];
      p_newtype->SUBARRAY.p_dims[d].start    = array_of_starts[d];
      elements     *= array_of_subsizes[d];
      blocklength  *= array_of_sizes[d];
      first_offset += array_of_starts[d] * blocklength;
      last_offset  += (array_of_starts[d] + array_of_subsizes[d]) * blocklength;
    }
  nm_mpi_datatype_update_bounds(1, first_offset, p_oldtype, p_newtype);
  nm_mpi_datatype_update_bounds(1, last_offset, p_oldtype, p_newtype);
  p_newtype->size      = elements * p_oldtype->size;
  p_newtype->elements  = elements;
  p_newtype->ser_size += sizeof(struct nm_mpi_datatype_ser_param_subarray_s);
  p_newtype->ser_size += ndims * 3 * sizeof(int);
  if(p_newtype->lb == MPI_UNDEFINED)
    p_newtype->lb = 0;
  if(p_newtype->extent == MPI_UNDEFINED)
    p_newtype->extent = 0;
  p_newtype->hash  = nm_mpi_datatype_hash_common(p_newtype);
  p_newtype->hash += p_oldtype->hash;
  p_newtype->hash += puk_hash_oneatatime((const void*)&ndims, sizeof(int));
  p_newtype->hash += puk_hash_oneatatime((const void*)&order, sizeof(int));
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_sizes, ndims * sizeof(int));
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_subsizes, ndims * sizeof(int));
  p_newtype->hash += puk_hash_oneatatime((const void*)array_of_starts, ndims * sizeof(int));
  p_newtype->p_serialized = malloc(p_newtype->ser_size);
  p_newtype->p_serialized->hash = p_newtype->hash;
  p_newtype->p_serialized->combiner = MPI_COMBINER_SUBARRAY;
  p_newtype->p_serialized->count = p_newtype->count;
  p_newtype->p_serialized->ser_size = p_newtype->ser_size;
  p_newtype->p_serialized->p.SUBARRAY.old_type = p_oldtype->hash;
  p_newtype->p_serialized->p.SUBARRAY.ndims = ndims;
  p_newtype->p_serialized->p.SUBARRAY.order = order;
  void*tmp = &p_newtype->p_serialized->p.SUBARRAY.DATA;
  memcpy(tmp, array_of_sizes, ndims * sizeof(int));
  tmp += ndims * sizeof(int);
  memcpy(tmp, array_of_subsizes, ndims * sizeof(int));
  tmp += ndims * sizeof(int);
  memcpy(tmp, array_of_starts, ndims * sizeof(int));
  return MPI_SUCCESS;
}

int mpi_type_create_darray(int size, int rank, int ndims,
			   const int array_of_gsizes[], const int array_of_distribs[],
			   const int array_of_dargs[], const int array_of_psizes[],
			   int order, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  padico_warning("madmpi: MPI_Type_create_darray()- unsupported.\n");
  return MPI_ERR_OTHER;
}
  

int mpi_type_create_struct(int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Datatype array_of_types[], MPI_Datatype *newtype)
{
  return mpi_type_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype);
}


int mpi_type_get_envelope(MPI_Datatype datatype, int*num_integers, int*num_addresses, int*num_datatypes, int*combiner)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  *combiner = p_datatype->combiner;
  switch(p_datatype->combiner)
    {
    case MPI_COMBINER_NAMED:
      *num_integers  = 0;
      *num_addresses = 0;
      *num_datatypes = 0;
      break;
    case MPI_COMBINER_CONTIGUOUS:
      *num_integers  = 1;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_DUP:
      *num_integers  = 0;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_RESIZED:
      *num_integers  = 0;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_VECTOR:
      *num_integers  = 3;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_HVECTOR:
      *num_integers  = 2;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_INDEXED:
      *num_integers  = p_datatype->count;
      *num_addresses = p_datatype->count;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_HINDEXED:
      *num_integers  = 1;
      *num_addresses = p_datatype->count;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_INDEXED_BLOCK:
      *num_integers  = 1;
      *num_addresses = p_datatype->count;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_HINDEXED_BLOCK:
      *num_integers  = 1;
      *num_addresses = p_datatype->count;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_SUBARRAY:
      *num_integers  = 3 * p_datatype->SUBARRAY.ndims;
      *num_addresses = 0;
      *num_datatypes = 1;
      break;
    case MPI_COMBINER_STRUCT:
      *num_integers  = p_datatype->count;
      *num_addresses = p_datatype->count;
      *num_datatypes = p_datatype->count;
      break;
    }
  return MPI_SUCCESS;
}

int mpi_type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes,
			  int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[])
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
  switch(p_datatype->combiner)
    {
    case MPI_COMBINER_NAMED:
      break;
    case MPI_COMBINER_CONTIGUOUS:
      array_of_datatypes[0] = p_datatype->CONTIGUOUS.p_old_type->id;
      break;
    case MPI_COMBINER_DUP:
      array_of_integers[0]  = p_datatype->count;
      array_of_datatypes[0] = p_datatype->DUP.p_old_type->id;
      break;
    case MPI_COMBINER_RESIZED:
      array_of_integers[0]  = p_datatype->count;
      array_of_datatypes[0] = p_datatype->RESIZED.p_old_type->id;
      break;
    case MPI_COMBINER_VECTOR:
      array_of_integers[0]  = p_datatype->count;
      array_of_integers[1]  = p_datatype->VECTOR.stride;
      array_of_integers[2]  = p_datatype->VECTOR.blocklength;
      array_of_datatypes[0] = p_datatype->VECTOR.p_old_type->id;
      break;
    case MPI_COMBINER_HVECTOR:
      array_of_integers[0]  = p_datatype->count;
      array_of_integers[1]  = p_datatype->HVECTOR.hstride;
      array_of_integers[2]  = p_datatype->HVECTOR.blocklength;
      array_of_datatypes[0] = p_datatype->HVECTOR.p_old_type->id;
      break;
    case MPI_COMBINER_INDEXED:
      array_of_integers[0]  = p_datatype->count;
      break;
    case MPI_COMBINER_HINDEXED:
      padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
      break;
    case MPI_COMBINER_INDEXED_BLOCK:
      padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
      break;
    case MPI_COMBINER_HINDEXED_BLOCK:
      padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
      break;
    case MPI_COMBINER_SUBARRAY:
      padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
      break;
    case MPI_COMBINER_STRUCT:
      padico_fatal("MPI_Type_get_contents()- not supported yet.\n");
      break;
    }
  return MPI_SUCCESS;
}


__PUK_SYM_INTERNAL
size_t nm_mpi_datatype_size(nm_mpi_datatype_t*p_datatype)
{
  return p_datatype->size;
}

__PUK_SYM_INTERNAL
nm_mpi_datatype_t* nm_mpi_datatype_get(MPI_Datatype datatype)
{
  return nm_mpi_handle_datatype_get(&nm_mpi_datatypes, datatype);
}

__PUK_SYM_INTERNAL
int nm_mpi_datatype_unlock(nm_mpi_datatype_t*p_datatype)
{
  int nb_ref = __sync_sub_and_fetch(&p_datatype->refcount, 1);
  assert(nb_ref >= 0);
  if((nb_ref == 0) && (p_datatype->id >= _NM_MPI_DATATYPE_OFFSET))
    {
      nm_mpi_datatype_free(p_datatype);
    }
  return MPI_SUCCESS;
}

static void nm_mpi_datatype_free(nm_mpi_datatype_t*p_datatype)
{
  switch(p_datatype->combiner)
    {
    case MPI_COMBINER_INDEXED:
      if(p_datatype->INDEXED.p_map != NULL)
	FREE_AND_SET_NULL(p_datatype->INDEXED.p_map);
      break;
    case MPI_COMBINER_HINDEXED:
      if(p_datatype->HINDEXED.p_map != NULL)
	FREE_AND_SET_NULL(p_datatype->HINDEXED.p_map);
      break;
    case MPI_COMBINER_INDEXED_BLOCK:
      if(p_datatype->INDEXED_BLOCK.array_of_displacements != NULL)
	FREE_AND_SET_NULL(p_datatype->INDEXED_BLOCK.array_of_displacements);
      break;
    case MPI_COMBINER_HINDEXED_BLOCK:
      if(p_datatype->HINDEXED_BLOCK.array_of_displacements != NULL)
	FREE_AND_SET_NULL(p_datatype->HINDEXED_BLOCK.array_of_displacements);
      break;
    case MPI_COMBINER_SUBARRAY:
      if(p_datatype->SUBARRAY.p_dims != NULL)
	FREE_AND_SET_NULL(p_datatype->SUBARRAY.p_dims);
      break;
    case MPI_COMBINER_STRUCT:
      if(p_datatype->STRUCT.p_map != NULL)
	FREE_AND_SET_NULL(p_datatype->STRUCT.p_map);
      break;
    default:
      break;
    }
  if(p_datatype->name != NULL)
    {
      FREE_AND_SET_NULL(p_datatype->name);
    }
  if(p_datatype->p_serialized != NULL)
    {
      FREE_AND_SET_NULL(p_datatype->p_serialized);
    }
  nm_mpi_attrs_destroy(p_datatype->id, &p_datatype->attrs);
  nm_mpi_handle_datatype_free(&nm_mpi_datatypes, p_datatype);
}

/** recursively call traversal function on all sub data; slow (regular) version
 */
#define NM_MPI_DATATYPE_TRAVERSAL_LOOP_PLAIN(P_OLD_TYPE, BLOCKLENGTH, LPTR) \
  {									\
    int j;								\
    for(j = 0; j < p_datatype->count; j++)				\
      {									\
	const struct nm_data_mpi_datatype_s sub =			\
	  { .ptr        = (LPTR),					\
	    .p_datatype = (P_OLD_TYPE),					\
	    .count      = (BLOCKLENGTH) };				\
	nm_mpi_datatype_traversal_apply(&sub, apply, _context);		\
      }									\
  }

/** recursively call traversal function on all sub data;
 * fast version optimized for terminal recursions whith same type for all blocks
 */
#define NM_MPI_DATATYPE_TRAVERSAL_LOOP(P_OLD_TYPE, BLOCKLENGTH, LPTR)	\
  if((P_OLD_TYPE)->is_compact)						\
    {									\
      int j;								\
      for(j = 0; j < p_datatype->count; j++)				\
	{								\
	  void*const lptr = (LPTR);					\
	  const nm_len_t chunk_size = (BLOCKLENGTH) * (P_OLD_TYPE)->size; \
	  (*apply)(lptr, chunk_size, _context);				\
	}								\
    }									\
  else									\
    {									\
      NM_MPI_DATATYPE_TRAVERSAL_LOOP_PLAIN(P_OLD_TYPE, BLOCKLENGTH, LPTR); \
    }
 
/** apply a function to every chunk of data in datatype */
static void nm_mpi_datatype_traversal_apply(const void*_content, const nm_data_apply_t apply, void*_context)
{
  const struct nm_data_mpi_datatype_s*const p_data = _content;
  const struct nm_mpi_datatype_s*const p_datatype = p_data->p_datatype;
  const int count = p_data->count;
  void*ptr = p_data->ptr;
  assert(p_datatype->refcount > 0);
  if(p_datatype->is_compact || (count == 0))
    {
      (*apply)((void*)ptr, count * p_datatype->size, _context);
    }
  else
    {
      int i;
      for(i = 0; i < count; i++)
	{
	  switch(p_datatype->combiner)
	    {
	    case MPI_COMBINER_CONTIGUOUS:
	      {
		const struct nm_data_mpi_datatype_s sub = 
		  { .ptr = ptr, .p_datatype = p_datatype->CONTIGUOUS.p_old_type, .count = p_datatype->count };
		nm_mpi_datatype_traversal_apply(&sub, apply, _context);
	      }
	      break;

	    case MPI_COMBINER_DUP:
	      {
		const struct nm_data_mpi_datatype_s sub = 
		  { .ptr = ptr, .p_datatype = p_datatype->DUP.p_old_type, .count = 1 };
		nm_mpi_datatype_traversal_apply(&sub, apply, _context);
	      }
	      break;

	    case MPI_COMBINER_RESIZED:
	      {
		const struct nm_data_mpi_datatype_s sub = 
		  { .ptr = ptr, .p_datatype = p_datatype->RESIZED.p_old_type, .count = 1 };
		nm_mpi_datatype_traversal_apply(&sub, apply, _context);
	      }
	      break;
	      
	    case MPI_COMBINER_VECTOR:
	      NM_MPI_DATATYPE_TRAVERSAL_LOOP(p_datatype->VECTOR.p_old_type,
					     p_datatype->VECTOR.blocklength,
					     ptr + j * p_datatype->VECTOR.stride * p_datatype->VECTOR.p_old_type->extent);
	      break;

	    case MPI_COMBINER_HVECTOR:
	      NM_MPI_DATATYPE_TRAVERSAL_LOOP(p_datatype->HVECTOR.p_old_type,
					     p_datatype->HVECTOR.blocklength, 
					     ptr + j * p_datatype->HVECTOR.hstride);
	      break;
	      
	    case MPI_COMBINER_INDEXED:
	      NM_MPI_DATATYPE_TRAVERSAL_LOOP(p_datatype->INDEXED.p_old_type,
					     p_datatype->INDEXED.p_map[j].blocklength,
					     ptr + p_datatype->INDEXED.p_map[j].displacement * p_datatype->INDEXED.p_old_type->extent);
	      break;

	    case MPI_COMBINER_HINDEXED:
	      NM_MPI_DATATYPE_TRAVERSAL_LOOP(p_datatype->HINDEXED.p_old_type,
					     p_datatype->HINDEXED.p_map[j].blocklength,
					     ptr + p_datatype->HINDEXED.p_map[j].displacement);
	      break;

	    case MPI_COMBINER_INDEXED_BLOCK:
	      NM_MPI_DATATYPE_TRAVERSAL_LOOP(p_datatype->INDEXED_BLOCK.p_old_type,
					     p_datatype->INDEXED_BLOCK.blocklength,
					     ptr + p_datatype->INDEXED_BLOCK.array_of_displacements[j] * p_datatype->INDEXED_BLOCK.p_old_type->extent);
	      break;
	      
	    case MPI_COMBINER_HINDEXED_BLOCK:
	      NM_MPI_DATATYPE_TRAVERSAL_LOOP(p_datatype->HINDEXED_BLOCK.p_old_type,
					     p_datatype->HINDEXED_BLOCK.blocklength,
					     ptr + p_datatype->HINDEXED_BLOCK.array_of_displacements[j]);
	      break;

	    case MPI_COMBINER_STRUCT:
	      /* different oldtype for each field; cannot call optimized loop */
	      NM_MPI_DATATYPE_TRAVERSAL_LOOP_PLAIN(p_datatype->STRUCT.p_map[j].p_old_type,
						   p_datatype->STRUCT.p_map[j].blocklength,
						   ptr + p_datatype->STRUCT.p_map[j].displacement);
	      break;

	    case MPI_COMBINER_SUBARRAY:
	      {
		int done = 0;
		int cur[p_datatype->SUBARRAY.ndims]; /* current N-dim index */
		int k;
		/* init index */
		for(k = 0; k < p_datatype->SUBARRAY.ndims; k++)
		  {
		    cur[k] = p_datatype->SUBARRAY.p_dims[k].start;
		  }
		while(!done)
		  {
		    /* calculate linear offset for current N-dim index */
		    MPI_Count blocklength = 1;
		    MPI_Aint offset = 0;
		    for(k = 0; k < p_datatype->SUBARRAY.ndims; k++)
		      {
			const int d = (p_datatype->SUBARRAY.order == MPI_ORDER_C) ?
			  (p_datatype->SUBARRAY.ndims - k - 1) : k;
			offset      += cur[d] * blocklength;
			blocklength *= p_datatype->SUBARRAY.p_dims[d].size;
		      }
		    /* apply datatype function */
		    const struct nm_data_mpi_datatype_s sub =
		      { .ptr        = ptr + offset * p_datatype->SUBARRAY.p_old_type->extent,
			.p_datatype = p_datatype->SUBARRAY.p_old_type,
			.count      = 1 };
		    nm_mpi_datatype_traversal_apply(&sub, apply, _context);
		    /* increment N-dim index */
		    for(k = 0; k < p_datatype->SUBARRAY.ndims; k++)
		      {
			const int d = (p_datatype->SUBARRAY.order == MPI_ORDER_C) ?
			  (p_datatype->SUBARRAY.ndims - k - 1) : k;
			if(cur[d] + 1 < p_datatype->SUBARRAY.p_dims[d].start + p_datatype->SUBARRAY.p_dims[d].subsize)
			  {
			    cur[d]++;
			    break;
			  }
			cur[d] = p_datatype->SUBARRAY.p_dims[d].start; /* reset dim and propagate carry */
		      }
		    done = (k >= p_datatype->SUBARRAY.ndims);
		  }
	      }
	      break;
	      
	    case MPI_COMBINER_NAMED:
	      /* we should not be there: NAMED types are supposed to be contiguous/copmpact and caught by optimized path */
	      (*apply)(ptr, p_datatype->size, _context);
	      if(p_datatype->committed)
		{
		  NM_MPI_FATAL_ERROR("internal error- NAMED datatype %s not contiguous.\n", p_datatype->name);
		}
	      break;

	    default:
	      NM_MPI_FATAL_ERROR("cannot filter datatype with combiner %d\n", p_datatype->combiner);
	    }
	  ptr += p_datatype->extent;
	}
    }
}

/** update bounds (lb, ub, extent) in newtype to take into account the given block of data */
static void nm_mpi_datatype_update_bounds(int blocklength, MPI_Aint displacement, nm_mpi_datatype_t*p_oldtype, nm_mpi_datatype_t*p_newtype)
{
  p_newtype->elements += blocklength - 1; /* 'elements' was initialized with 'count'; don't count it twice */
  if(blocklength > 0)
    {
      const intptr_t cur_lb = p_newtype->lb;
      const intptr_t cur_ub = ((p_newtype->lb != MPI_UNDEFINED) && (p_newtype->extent != MPI_UNDEFINED)) ? (p_newtype->lb + p_newtype->extent) : MPI_UNDEFINED;
      intptr_t new_lb = cur_lb;
      intptr_t new_ub = cur_ub;
      if(p_oldtype->id == MPI_LB)
	{
	  new_lb = displacement;
	}
      else if(p_oldtype->id == MPI_UB)
	{
	  new_ub = displacement;
	}
      else
	{
	  const intptr_t local_lb = displacement + p_oldtype->lb ;
	  const intptr_t local_ub = displacement + p_oldtype->lb + blocklength * p_oldtype->extent;
	  if((cur_lb == MPI_UNDEFINED) || (local_lb < cur_lb))
	    {
	      new_lb = local_lb;
	    }
	  if((cur_ub == MPI_UNDEFINED) || (local_ub > cur_ub))
	    {
	      new_ub = local_ub;
	    }
	}
      if(new_lb != MPI_UNDEFINED)
	p_newtype->lb = new_lb;
      if(new_lb != MPI_UNDEFINED)
	{
	  if(new_ub != MPI_UNDEFINED)
	    p_newtype->extent = new_ub - new_lb;
	  else
	    p_newtype->extent = 0 - new_lb; /* negative extent in case only lb is given */
	}
    }
}

static void nm_mpi_datatype_data_properties_compute(struct nm_data_s*p_data)
{
  const struct nm_data_mpi_datatype_s*const p_data_mpi = nm_data_mpi_datatype_content(p_data);
  const struct nm_mpi_datatype_s*const p_datatype = p_data_mpi->p_datatype;
  const int count = p_data_mpi->count;
  const struct nm_data_properties_s props =
    {
      .blocks = count * p_datatype->props.blocks,
      .size   = count * p_datatype->props.size,
      .is_contig = p_datatype->props.is_contig && ((count < 2) ||
						   (p_datatype->lb == 0 && p_datatype->extent == p_datatype->size))
    };
  p_data->props = props;
}

struct nm_mpi_datatype_properties_context_s
{
  void*baseptr;
  void*blockend; /**< end of previous block */
  struct nm_mpi_datatype_properties_s
  {
    intptr_t true_lb;
    intptr_t true_ub;
  } props;
  struct nm_data_properties_s nm_data_props;
};
static void nm_mpi_datatype_properties_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_mpi_datatype_properties_context_s*p_context = _context;
  p_context->nm_data_props.size += len;
  p_context->nm_data_props.blocks += 1;
  if(p_context->nm_data_props.is_contig)
    {
      if((p_context->blockend != NULL) && (ptr != p_context->blockend))
	p_context->nm_data_props.is_contig = 0;
    }
  p_context->blockend = ptr + len;
  const intptr_t local_lb = ptr - p_context->baseptr;
  if(local_lb < p_context->props.true_lb || p_context->props.true_lb == MPI_UNDEFINED)
    p_context->props.true_lb = local_lb;
  const intptr_t local_ub = (ptr + len) - p_context->baseptr;
  if(local_ub > p_context->props.true_ub || p_context->props.true_ub == MPI_UNDEFINED)
    p_context->props.true_ub = local_ub;
}
static void nm_mpi_datatype_properties_compute(nm_mpi_datatype_t*p_datatype)
{
  struct nm_mpi_datatype_properties_context_s context =
    {
      .baseptr = NULL,
      .blockend = NULL,
      .props.true_lb = MPI_UNDEFINED,
      .props.true_ub = MPI_UNDEFINED,
      .nm_data_props.is_contig = 1,
      .nm_data_props.blocks    = 0,
      .nm_data_props.size      = 0
    };
  const struct nm_data_mpi_datatype_s content = { .ptr = NULL, .p_datatype = p_datatype, .count = 1 };
  nm_mpi_datatype_traversal_apply(&content, &nm_mpi_datatype_properties_apply, &context);
  p_datatype->is_contig   = context.nm_data_props.is_contig;
  p_datatype->true_lb     = context.props.true_lb;
  p_datatype->true_extent = context.props.true_ub - context.props.true_lb;
  p_datatype->props       = context.nm_data_props;
  p_datatype->is_compact  = (p_datatype->is_contig && (p_datatype->size == p_datatype->extent) && (p_datatype->lb == 0));
  p_datatype->committed = 1;
}

/** status for nm_mpi_datatype_*_memcpy */
struct nm_mpi_datatype_filter_memcpy_s
{
  void*pack_ptr; /**< pointer on packed data in memory */
};
/** Pack data to memory 
 */
static void nm_mpi_datatype_pack_memcpy(void*data_ptr, nm_len_t size, void*_status)
{
  struct nm_mpi_datatype_filter_memcpy_s*status = _status;
  memcpy(status->pack_ptr, data_ptr, size);
  status->pack_ptr += size;
}
/** Unpack data from memory
 */
static void nm_mpi_datatype_unpack_memcpy(void*data_ptr, nm_len_t size, void*_status)
{
  struct nm_mpi_datatype_filter_memcpy_s*status = _status;
  memcpy(data_ptr, status->pack_ptr, size);
  status->pack_ptr += size;
}

/** Pack data into a contiguous buffers.
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_pack(void*outbuf, const void*inbuf, nm_mpi_datatype_t*p_datatype, int count)
{
  struct nm_mpi_datatype_filter_memcpy_s context = { .pack_ptr = (void*)outbuf };
  const struct nm_data_mpi_datatype_s content = { .ptr = (void*)inbuf, .p_datatype = p_datatype, .count = count };
  nm_mpi_datatype_traversal_apply(&content, &nm_mpi_datatype_pack_memcpy, &context);
}

/** Unpack data from a contiguous buffers.
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_unpack(const void*inbuf, void*outbuf, nm_mpi_datatype_t*p_datatype, int count)
{
  struct nm_mpi_datatype_filter_memcpy_s context = { .pack_ptr = (void*)inbuf };
  const struct nm_data_mpi_datatype_s content = { .ptr = outbuf, .p_datatype = p_datatype, .count = count };
  nm_mpi_datatype_traversal_apply(&content, &nm_mpi_datatype_unpack_memcpy, &context);
}

/** Copy data using datatype layout
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_copy(const void*src_buf, nm_mpi_datatype_t*p_src_type, int src_count,
			  void*dest_buf, nm_mpi_datatype_t*p_dest_type, int dest_count)
{
  if(p_src_type == p_dest_type && p_src_type->is_contig && src_count == dest_count)
    {
      int i;
      for(i = 0; i < src_count; i++)
	{
	  memcpy(dest_buf + i * p_dest_type->extent, src_buf + i * p_src_type->extent, p_src_type->size);
	}
    }
  else if(src_count > 0)
    {
      void*ptr = malloc(p_src_type->size * src_count);
      nm_mpi_datatype_pack(ptr, src_buf, p_src_type, src_count);
      nm_mpi_datatype_unpack(ptr, dest_buf, p_dest_type, dest_count);
      free(ptr);
    }
}

/** Apply a function to every chunk of data in datatype and to the corresponding header
 */
static void nm_mpi_datatype_wrapper_traversal_apply(const void*_content, nm_data_apply_t apply,
						    void*_context)
{
  const struct nm_data_mpi_datatype_wrapper_s*const p_data = _content;
  (*apply)((void*)&p_data->header, sizeof(struct nm_data_mpi_datatype_header_s), _context);
  nm_mpi_datatype_traversal_apply((void*)&p_data->data, apply, _context);
}

/** apply a serialization function to a datatype 
 */
static void nm_mpi_datatype_serial_traversal_apply(const void*_content, nm_data_apply_t apply, void*_context)
{
  int i;
  const struct nm_mpi_datatype_s*const p_datatype = *(void**)_content;
  switch(p_datatype->combiner)
    {
    case MPI_COMBINER_NAMED:
      return;
    case MPI_COMBINER_CONTIGUOUS:
      {
	nm_mpi_datatype_serial_traversal_apply(&p_datatype->CONTIGUOUS.p_old_type, apply, _context);
      }
      break;
    case MPI_COMBINER_DUP:
      {
	nm_mpi_datatype_serial_traversal_apply(&p_datatype->DUP.p_old_type, apply, _context);
      }
      break;
    case MPI_COMBINER_RESIZED:
      {
	nm_mpi_datatype_serial_traversal_apply(&p_datatype->RESIZED.p_old_type, apply, _context);
      }
      break;
    case MPI_COMBINER_VECTOR:
      {
	nm_mpi_datatype_serial_traversal_apply(&p_datatype->VECTOR.p_old_type, apply, _context);
      }
      break;
    case MPI_COMBINER_HVECTOR:
      {
	nm_mpi_datatype_serial_traversal_apply(&p_datatype->HVECTOR.p_old_type, apply, _context);
      }
      break;
    case MPI_COMBINER_INDEXED:
      {
	nm_mpi_datatype_serial_traversal_apply(&p_datatype->INDEXED.p_old_type, apply, _context);
      }
      break;
    case MPI_COMBINER_HINDEXED:
      {
	nm_mpi_datatype_serial_traversal_apply(&p_datatype->HINDEXED.p_old_type, apply, _context);
      }
      break;
    case MPI_COMBINER_INDEXED_BLOCK:
      {
	nm_mpi_datatype_serial_traversal_apply(&p_datatype->INDEXED_BLOCK.p_old_type, apply, _context);
      }
      break;
    case MPI_COMBINER_HINDEXED_BLOCK:
      {
	nm_mpi_datatype_serial_traversal_apply(&p_datatype->HINDEXED_BLOCK.p_old_type, apply, _context);
      }
      break;
    case MPI_COMBINER_SUBARRAY:
      {
	nm_mpi_datatype_serial_traversal_apply(&p_datatype->SUBARRAY.p_old_type, apply, _context);
      }
      break;
    case MPI_COMBINER_STRUCT:
      for(i = 0; i < p_datatype->count; ++i)
	{
	  nm_mpi_datatype_serial_traversal_apply(&p_datatype->STRUCT.p_map[i].p_old_type, apply, _context);
	}
      break;
    default:
      NM_MPI_FATAL_ERROR("madmpi: cannot serialize datatype with combiner %d\n", p_datatype->combiner);
    }
  assert(p_datatype->p_serialized);
  (*apply)((void*)p_datatype->p_serialized, p_datatype->ser_size, _context);
}

/** deserialize a datatype 
 */
__PUK_SYM_INTERNAL
void nm_mpi_datatype_deserialize(nm_mpi_datatype_ser_t*p_datatype, MPI_Datatype*newtype)
{
  switch(p_datatype->combiner)
    {
    case MPI_COMBINER_CONTIGUOUS:
      {
	nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_hashtable_get(p_datatype->p.CONTIGUOUS.old_type);
	MPI_Type_contiguous(p_datatype->count, p_oldtype->id, newtype);
      }
      break;
    case MPI_COMBINER_DUP:
      {
	nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_hashtable_get(p_datatype->p.DUP.old_type);
	MPI_Type_dup(p_oldtype->id, newtype);
      }
      break;
    case MPI_COMBINER_RESIZED:
      {
	nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_hashtable_get(p_datatype->p.RESIZED.old_type);
	MPI_Type_create_resized(p_oldtype->id, p_datatype->p.RESIZED.lb, p_datatype->p.RESIZED.extent, newtype);
      }
      break;
    case MPI_COMBINER_VECTOR:
      {
	nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_hashtable_get(p_datatype->p.VECTOR.old_type);
	MPI_Type_vector(p_datatype->count, p_datatype->p.VECTOR.blocklength, p_datatype->p.VECTOR.stride, p_oldtype->id, newtype);	
      }
      break;
    case MPI_COMBINER_HVECTOR:
      {
	nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_hashtable_get(p_datatype->p.HVECTOR.old_type);
	MPI_Type_hvector(p_datatype->count, p_datatype->p.HVECTOR.blocklength, p_datatype->p.HVECTOR.hstride, p_oldtype->id, newtype);
      }
      break;
    case MPI_COMBINER_INDEXED:
      {
	nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_hashtable_get(p_datatype->p.INDEXED.old_type);
	int*aob = (void*)&p_datatype->p.INDEXED.DATA;
	int*aod = aob + p_datatype->count;
	MPI_Type_indexed(p_datatype->count, (int*)aob, (int*)aod, p_oldtype->id, newtype);
      }
      break;
    case MPI_COMBINER_HINDEXED:
      {
	nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_hashtable_get(p_datatype->p.HINDEXED.old_type);
	int*aob = (void*)&p_datatype->p.INDEXED.DATA;
	MPI_Aint*aod = (MPI_Aint*)(aob + p_datatype->count);
	MPI_Type_hindexed(p_datatype->count, (int*)aob, (MPI_Aint*)aod, p_oldtype->id, newtype);
      }
      break;
    case MPI_COMBINER_INDEXED_BLOCK:
      {
	nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_hashtable_get(p_datatype->p.INDEXED_BLOCK.old_type);
	int*aod = (void*)&p_datatype->p.INDEXED_BLOCK.DATA;
	MPI_Type_create_indexed_block(p_datatype->count, p_datatype->p.INDEXED_BLOCK.blocklength, (int*)aod, p_oldtype->id, newtype);
      }
      break;
    case MPI_COMBINER_HINDEXED_BLOCK:
      {
	nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_hashtable_get(p_datatype->p.HINDEXED_BLOCK.old_type);
	MPI_Aint*aod = (void*)&p_datatype->p.HINDEXED_BLOCK.DATA;
	MPI_Type_create_hindexed_block(p_datatype->count, p_datatype->p.HINDEXED_BLOCK.blocklength, (MPI_Aint*)aod, p_oldtype->id, newtype);
      }
      break;
    case MPI_COMBINER_SUBARRAY:
      {
	int*sizes = (void*)&p_datatype->p.SUBARRAY.DATA;;
	int*subsizes = sizes + p_datatype->p.SUBARRAY.ndims;
	int*starts = subsizes + p_datatype->p.SUBARRAY.ndims;
	nm_mpi_datatype_t*p_oldtype = nm_mpi_datatype_hashtable_get(p_datatype->p.SUBARRAY.old_type);
	MPI_Type_create_subarray(p_datatype->p.SUBARRAY.ndims, sizes, subsizes, starts, p_datatype->p.SUBARRAY.order, p_oldtype->id, newtype);
      }
      break;
    case MPI_COMBINER_STRUCT:
      {
	void*tmp = (void*)&p_datatype->p.STRUCT.DATA;
	uint32_t*old_hashes = tmp;
	MPI_Datatype aodt[p_datatype->count];
	int i;
	nm_mpi_datatype_t*p_oldtype;
	for(i = 0; i < p_datatype->count; ++i)
	  {
	    p_oldtype = nm_mpi_datatype_hashtable_get(old_hashes[i]);
	    aodt[i] = p_oldtype->id;
	  }
	tmp += p_datatype->count * sizeof(MPI_Datatype);
	int*aob = tmp;
	tmp += p_datatype->count * sizeof(int);
	MPI_Aint*aod = tmp;
	MPI_Type_struct(p_datatype->count, aob, aod, aodt, newtype);
      }
      break;
    default:
      NM_MPI_FATAL_ERROR("madmpi: cannot deserialize datatype with combiner %d\n", p_datatype->combiner);
    }
  nm_mpi_datatype_t*p_newtype = nm_mpi_datatype_get(*newtype), *p_saved_type;
  uint32_t       newtype_hash = p_newtype->hash;
  nm_mpi_datatype_hashtable_insert(p_newtype);
  p_saved_type = nm_mpi_datatype_hashtable_get(newtype_hash);
  *newtype = p_saved_type->id;
  if(p_saved_type == p_newtype)
    {
      nm_mpi_datatype_properties_compute(p_newtype);
    }
  else
    {
      nm_mpi_datatype_unlock(p_newtype);
    }
}

int mpi_pack(void*inbuf, int incount, MPI_Datatype datatype, void*outbuf, int outsize, int*position, MPI_Comm comm)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  size_t size = incount * nm_mpi_datatype_size(p_datatype);
  assert(outsize >= size);
  nm_mpi_datatype_pack(outbuf + *position, inbuf, p_datatype, incount);
  *position += size;
  return MPI_SUCCESS;
}

int mpi_unpack(void*inbuf, int insize, int*position, void*outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  size_t size = outcount * nm_mpi_datatype_size(p_datatype);
  assert(insize >= size);
  nm_mpi_datatype_unpack(inbuf + *position, outbuf, p_datatype, outcount);
  *position += size;
  return MPI_SUCCESS;
}

int mpi_pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int*size)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  *size = incount * nm_mpi_datatype_size(p_datatype);
  return MPI_SUCCESS;
}

int mpi_type_set_name(MPI_Datatype datatype, char*type_name)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  if(p_datatype->name != NULL)
    {
      free(p_datatype->name);
    }
  p_datatype->name = strndup(type_name, MPI_MAX_OBJECT_NAME);
  return MPI_SUCCESS;
}

int mpi_type_get_name(MPI_Datatype datatype, char*type_name, int*resultlen)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  if(p_datatype->name != NULL)
    {
      strncpy(type_name, p_datatype->name, MPI_MAX_OBJECT_NAME);
    }
  else
    {
      type_name[0] = '\0';
    }
  *resultlen = strlen(type_name);
  return MPI_SUCCESS;
}

int mpi_type_create_keyval(MPI_Type_copy_attr_function*copy_fn, MPI_Type_delete_attr_function*delete_fn, int*keyval, void*extra_state)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_new();
  p_keyval->copy_fn = copy_fn;
  p_keyval->delete_fn = delete_fn;
  p_keyval->extra_state = extra_state;
  *keyval = p_keyval->id;
  return MPI_SUCCESS;
}

int mpi_type_free_keyval(int*keyval)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(*keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_keyval_delete(p_keyval);
  *keyval = MPI_KEYVAL_INVALID;
  return MPI_SUCCESS;
}

int mpi_type_delete_attr(MPI_Datatype datatype, int keyval)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  int err = nm_mpi_attr_delete(p_datatype->id, p_datatype->attrs, p_keyval);
  return err;
}

int mpi_type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val)
{
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(type_keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  if(p_datatype->attrs == NULL)
    {
      p_datatype->attrs = puk_hashtable_new_ptr();
    }
  int err = nm_mpi_attr_put(p_datatype->id, p_datatype->attrs, p_keyval, attribute_val);
  return err;
}

int mpi_type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag)
{
  int err = MPI_SUCCESS;
  *flag = 0;
  /* Check type validity */
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(NULL == p_datatype) {
    return MPI_ERR_TYPE;
  }  
  struct nm_mpi_keyval_s*p_keyval = nm_mpi_keyval_get(type_keyval);
  if(p_keyval == NULL)
    return MPI_ERR_KEYVAL;
  nm_mpi_attr_get(p_datatype->attrs, p_keyval, attribute_val, flag);
  return err;
}
