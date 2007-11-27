#ifndef CCS_H
#define CCS_H

#include <stdio.h>
#include <stdlib.h>

/* CCS API */

/* types */

//#warning need a check for this
typedef int CCS_aint_t;

typedef unsigned CCS_context_t;
typedef struct CCS_token *CCS_token_t;
typedef unsigned CCS_node_t;

typedef unsigned long long CCS_atomic_type_t;

typedef CCS_aint_t CCS_datadesc_t;

#define CCS_DATA_NULL   ((CCS_datadesc_t)0x000)
#define CCS_DATA8       ((CCS_datadesc_t)0x001)
#define CCS_DATA16      ((CCS_datadesc_t)0x002)
#define CCS_DATA32      ((CCS_datadesc_t)0x004)
#define CCS_DATA64      ((CCS_datadesc_t)0x008)
#define CCS_DATA128     ((CCS_datadesc_t)0x010)
#define CCS_DATA_LB     ((CCS_datadesc_t)0x100)
#define CCS_DATA_UB     ((CCS_datadesc_t)0x200)
#define CCS_DATA_PACKED ((CCS_datadesc_t)0x300)
#define CCS_DATA_LAST CCS_DATA_PACKED

#define CCS_DATA_SIZE_MASK 0xff
#define CCS_datadesc_is_builtin(desc_) (desc_ <= CCS_DATA_LAST)

enum {
    CCS_SUCCESS, /* Everything's A-OK */
    CCS_FAILURE, /* General failure */
    CCS_NOMEM    /* Operation was unable to complete because of failure to allocate memory  */
};

/* process management */

int CCS_init ();
int CCS_finalize ();
int CCS_new_context (CCS_context_t *context);

int CCS_barrier ();

int CCS_get_size (CCS_node_t *size);
int CCS_get_rank (CCS_node_t *rank);

/* eventually spawn stuff */

/* memory registration */

int CCS_register_mem (void *addr, unsigned len);
int CCS_deregister_mem (void *addr, unsigned len);

/* callback */

typedef void (*CCS_callback_t) (void *);

/* active messages */

#define CCS_MAX_HANDLERS 256 /* must be less than 256 */
#define CCS_MAX_HANDLER_ARGS 16 /* must be less than 256 */
#define CCS_MAX_CONTEXTS 256 /* must be less than 256 */
#define CCS_MAX_AM_LENGTH 16384

typedef unsigned CCS_handler_arg_t;

typedef void (*CCS_amhandler_t) (CCS_token_t token, void *buffer, unsigned buf_len, unsigned num_args, ...);

int CCS_sender_rank (CCS_token_t token, CCS_node_t *sender);

int CCS_register_handler (CCS_context_t context, unsigned handler_id, CCS_amhandler_t handler_ptr);

int CCS_amrequest (CCS_context_t context, CCS_node_t node, void *buf, unsigned count, CCS_datadesc_t datadesc,
		   unsigned handler_id, unsigned num_args, ...);
int CCS_amreply (CCS_token_t token, void *buf, unsigned count, CCS_datadesc_t datadesc, unsigned handler_id, unsigned num_args, ...);

/* RDMA */

int CCS_put (CCS_node_t node,
	     void *src_buf, unsigned src_count, CCS_datadesc_t src_datadesc, 
	     void *dest_buf, unsigned dest_count, CCS_datadesc_t dest_datadesc, CCS_callback_t callback, void *callback_arg);
int CCS_get (CCS_node_t node,
	     void *src_buf, unsigned src_count, CCS_datadesc_t src_datadesc, 
	     void *dest_buf, unsigned dest_count, CCS_datadesc_t dest_datadesc, CCS_callback_t callback, void *callback_arg);

/* atomics */

typedef enum CCS_atomic_op {
    CCS_ATOMIC_SWAP,
    CCS_ATOMIC_COMPARESWAP,
    CCS_ATOMIC_FETCHADD,
    CCS_ATOMIC_READ,
    CCS_ATOMIC_WRITE,
    CCS_NUM_ATOMIC_OPS
} CCS_atomic_op_t;

int CCS_atomic (CCS_node_t node, CCS_atomic_op_t op, CCS_atomic_type_t *var, CCS_atomic_type_t val,
		 CCS_atomic_type_t cmp, CCS_atomic_type_t *result, CCS_callback_t callback, void *callback_arg);

/* fence */

int CCS_fence (CCS_node_t node);
int CCS_allfence ();

/* datatype */

int CCS_datadesc_extent (CCS_datadesc_t datadesc);
int CCS_datadesc_size (CCS_datadesc_t datadesc);

int CCS_datadesc_create_contiguous (int count, CCS_datadesc_t olddesc, CCS_datadesc_t *newdesc);
int CCS_datadesc_create_vector (int count, int blocklength, int stride, CCS_datadesc_t olddesc, CCS_datadesc_t *newdesc);
int CCS_datadesc_create_struct (int count, int *array_of_blocklengths, CCS_aint_t *array_of_displacements,
				 CCS_datadesc_t *array_of_descs, CCS_datadesc_t *newdesc);
int CCS_datadesc_create_indexed (int count, int *array_of_blocklengths, CCS_aint_t *array_of_displacements, CCS_datadesc_t olddesc,
				  CCS_datadesc_t *newdesc);
int CCS_datadesc_create_blockindexed (int count, int blocklength, CCS_aint_t *array_of_displacements, CCS_datadesc_t olddesc,
				       CCS_datadesc_t *newdesc);

void CCS_datadesc_free (CCS_datadesc_t *desc);

int CCS_pack (void *dest, void *src, int count, CCS_datadesc_t desc, unsigned bytes);
int CCS_unpack (void *dest, void *src, int count, CCS_datadesc_t desc, unsigned bytes);

/* progress */

void CCS_poll ();

/* utility */

void * CCS_malloc (int size);

void CCS_free (void *p);

#endif
