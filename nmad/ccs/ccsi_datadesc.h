#ifndef CCSI_DATADESC_H
#define CCSI_DATADESC_H
#include "ccs_dataloop.h"

typedef struct CCS_iovec
{
    void *base;
    CCS_aint_t len;
} CCS_iovec_t;

typedef struct CCSI_datadesc
{
    int ref_count;

    CCS_datadesc_t handle;
    
    int size;
    int extent;
    int lb;
    int ub;
    int true_lb;
    int true_ub;
    int has_sticky_lb;
    int has_sticky_ub;
    int alignsize;
    
    CCS_datadesc_t eltype;
    int element_size;
    
    int is_contig;
    struct CCSI_Dataloop *dataloop;
    int dataloop_size;
    int dataloop_depth;
}
CCSI_datadesc_t;

#define CCSI_datadesc_handle_ptr(handle_) ((CCSI_datadesc_t *)handle_)
#define CCSI_datadesc_ptr_handle(ptr_) ((CCS_datadesc_t )ptr_)

#define CCSI_datadesc_get_loopdepth_macro(handle_, depth_) do {	\
    if (CCS_datadesc_is_builtin (handle_))				\
	depth_ = 0;							\
    else								\
	depth_ = CCSI_datadesc_handle_ptr (handle_)->dataloop_depth;	\
} while (0)

#define CCSI_datadesc_get_loopsize_macro(handle_, size_) do {		\
    if (CCS_datadesc_is_builtin (handle_))				\
	size_ = 0;							\
    else								\
	size_ = CCSI_datadesc_handle_ptr (handle_)->dataloop_size;    	\
} while (0)

#define CCSI_datadesc_get_loopptr_macro(handle_, lptr_) do {	\
    if (CCS_datadesc_is_builtin (handle_))			\
	lptr_ = 0;						\
    else							\
	lptr_ = CCSI_datadesc_handle_ptr (handle_)->dataloop;	\
} while (0)

#define CCSI_datadesc_get_size_macro(handle_, size_) do {	\
    if (CCS_datadesc_is_builtin (handle_))			\
	size_ = CCSI_datadesc_builtin_size_macro (handle_);	\
    else							\
	size_ = CCSI_datadesc_handle_ptr (handle_)->size;	\
} while (0)

#define CCSI_datadesc_get_basic_type(handle_, eltype_) do {	\
        if (CCS_datadesc_is_builtin (handle_))			\
	eltype_ = handle_;					\
    else							\
	eltype_ = CCSI_datadesc_handle_ptr (handle_)->eltype;	\
} while (0)

#define CCSI_datadesc_get_extent_macro(handle_, extent_) do {	\
    if (CCS_datadesc_is_builtin (handle_))			\
	extent_ = CCSI_datadesc_builtin_size_macro (handle_);	\
    else							\
	extent_ = CCSI_datadesc_handle_ptr (handle_)->extent;	\
} while (0)

#define CCSI_datadesc_builtin_size_macro(handle_) ((int)handle_ & CCS_DATA_SIZE_MASK)

void CCSI_datadesc_add_ref (CCSI_datadesc_t *desc);
int CCSI_datadesc_release_ref (CCSI_datadesc_t *desc);

#endif
