
#include <ccs.h>
#include <ccsi.h>
#include <ccs_dataloop.h>
#include <segment.h>

#define CCSI_DATADESC_CONTIG_LB_UB(cnt_, old_lb_, old_ub_, old_extent_, lb_, ub_) do {	\
    if (cnt_ == 0) {									\
	lb_ = old_lb_;									\
	ub_ = old_ub_;									\
    }											\
    else if (old_ub_ >= old_lb_) {							\
        lb_ = old_lb_;									\
        ub_ = old_ub_ + (old_extent_) * (cnt_ - 1);					\
    }											\
    else /* negative extent */ {							\
	lb_ = old_lb_ + (old_extent_) * (cnt_ - 1);					\
	ub_ = old_ub_;									\
    }											\
} while (0)

#define CCSI_DATADESC_VECTOR_LB_UB(cnt_, stride_, blklen_, old_lb_, old_ub_, old_extent_, lb_, ub_) do {	\
    if (cnt_ == 0 || blklen_ == 0) {										\
	lb_ = old_lb_;												\
	ub_ = old_ub_;												\
    }														\
    else if (stride_ >= 0 && (old_extent_) >= 0) {								\
	lb_ = old_lb_;												\
	ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1) +							\
	    (stride_) * ((cnt_) - 1);										\
    }														\
    else if (stride_ < 0 && (old_extent_) >= 0) {								\
	lb_ = old_lb_ + (stride_) * ((cnt_) - 1);								\
	ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1);							\
    }														\
    else if (stride_ >= 0 && (old_extent_) < 0) {								\
	lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1);							\
	ub_ = old_ub_ + (stride_) * ((cnt_) - 1);								\
    }														\
    else {													\
	lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1) +							\
	    (stride_) * ((cnt_) - 1);										\
	ub_ = old_ub_;												\
    }														\
} while (0)

#define CCSI_DATADESC_BLOCK_LB_UB(cnt_, disp_, old_lb_, old_ub_, old_extent_, lb_, ub_) do {	\
    if (cnt_ == 0) {										\
	lb_ = old_lb_ + (disp_);								\
	ub_ = old_ub_ + (disp_);								\
    }												\
    else if (old_ub_ >= old_lb_) {								\
        lb_ = old_lb_ + (disp_);								\
        ub_ = old_ub_ + (disp_) + (old_extent_) * ((cnt_) - 1);					\
    }												\
    else /* negative extent */ {								\
	lb_ = old_lb_ + (disp_) + (old_extent_) * ((cnt_) - 1);					\
	ub_ = old_ub_ + (disp_);								\
    }												\
} while (0)

int
CCS_datadesc_create_contiguous (int count, CCS_datadesc_t olddesc, CCS_datadesc_t *newdesc)
{
    int errno = CCS_SUCCESS;
    int is_builtin;
    int el_sz;
    CCS_datadesc_t el_type;
    CCSI_datadesc_t *new_desc;

    new_desc = CCS_malloc (sizeof (CCSI_datadesc_t));

    if (!new_desc)
	return CCS_FAILURE;

    new_desc->ref_count = 1;
    new_desc->handle = CCSI_datadesc_ptr_handle (new_desc);
    new_desc->dataloop = NULL;
    new_desc->dataloop_size = -1;
    new_desc->dataloop_depth = -1;

    is_builtin = CCS_datadesc_is_builtin (olddesc);

    if (count == 0)
    {
	
	new_desc->size = 0;
	new_desc->has_sticky_ub = 0;
	new_desc->has_sticky_lb = 0;
	new_desc->lb = 0;
	new_desc->ub = 0;
	new_desc->true_lb = 0;
	new_desc->true_ub = 0;
	new_desc->extent = 0;
	new_desc->alignsize = 0;
	new_desc->element_size = 0;
	new_desc->eltype = 0;
	new_desc->is_contig = 1;

	errno = CCSI_Dataloop_create_contiguous(0,
						 CCS_DATA8, /* dummy type */
						 &(new_desc->dataloop),
						 &(new_desc->dataloop_size),
						 &(new_desc->dataloop_depth),
						 0);
	if (errno)
	    return CCS_FAILURE;
	
	*newdesc = new_desc->handle;

	return CCS_SUCCESS;
    }
    else if (is_builtin)
    {
	el_sz = CCSI_datadesc_builtin_size_macro (olddesc);
	el_type = olddesc;

	new_desc->size = count * el_sz;
	new_desc->has_sticky_ub = 0;
	new_desc->has_sticky_lb = 0;
	new_desc->true_lb = 0;
	new_desc->lb = 0;
	new_desc->true_ub = count * el_sz;
	new_desc->ub = new_desc->true_ub;
	new_desc->alignsize = el_sz;
	new_desc->extent = new_desc->ub - new_desc->lb;
	new_desc->eltype = el_type;
	new_desc->is_contig = 1;
    }
    else
    {
	CCSI_datadesc_t *old_desc;

	old_desc = CCSI_datadesc_handle_ptr (olddesc);
	el_type = old_desc->eltype;
	el_sz   = old_desc->element_size;

	new_desc->size = count * old_desc->size;
	new_desc->has_sticky_ub = old_desc->has_sticky_ub;
	new_desc->has_sticky_lb = old_desc->has_sticky_lb;
	
	CCSI_DATADESC_CONTIG_LB_UB(count,
				    old_desc->lb,
				    old_desc->ub,
				    old_desc->extent,
				    new_desc->lb,
				    new_desc->ub);

	new_desc->true_lb = new_desc->lb + (old_desc->true_lb - old_desc->lb);
	new_desc->true_ub = new_desc->ub + (old_desc->true_ub - old_desc->ub);
	new_desc->extent = new_desc->ub - new_desc->lb;

	new_desc->alignsize = old_desc->alignsize;
	new_desc->element_size = old_desc->element_size;
	new_desc->eltype = el_type;
	new_desc->is_contig = old_desc->is_contig;
    }

    errno = CCSI_Dataloop_create_contiguous(count,
					     olddesc,
					     &(new_desc->dataloop),
					     &(new_desc->dataloop_size),
					     &(new_desc->dataloop_depth),
					     0);
    if (errno)
	return CCS_FAILURE;
    
    *newdesc = new_desc->handle;

    return CCS_SUCCESS;
}

int
CCS_datadesc_create_vector (int count, int blocklength, int stride, CCS_datadesc_t olddesc, CCS_datadesc_t *newdesc)
{
    int errno = CCS_SUCCESS;
    int is_builtin, old_is_contig;
    int el_sz;
    CCS_datadesc_t el_type;
    int old_lb, old_ub, old_extent, old_true_lb, old_true_ub;

    CCSI_datadesc_t *new_desc;

    new_desc = CCS_malloc (sizeof (CCSI_datadesc_t));

    if (!new_desc)
	return CCS_FAILURE;

    new_desc->ref_count = 1;
    new_desc->handle = CCSI_datadesc_ptr_handle (new_desc);
    new_desc->dataloop = NULL;
    new_desc->dataloop_size = -1;
    new_desc->dataloop_depth = -1;

    is_builtin = CCS_datadesc_is_builtin (olddesc);

    if (count == 0)
    {
	
	new_desc->size = 0;
	new_desc->has_sticky_ub = 0;
	new_desc->has_sticky_lb = 0;
	new_desc->lb = 0;
	new_desc->ub = 0;
	new_desc->true_lb = 0;
	new_desc->true_ub = 0;
	new_desc->extent = 0;
	new_desc->alignsize = 0;
	new_desc->eltype = 0;
	new_desc->is_contig = 1;

	errno = CCSI_Dataloop_create_vector(0,
					     0,
					     0,
					     0,
					     CCS_DATA8, /* dummy type */
					     &(new_desc->dataloop),
					     &(new_desc->dataloop_size),
					     &(new_desc->dataloop_depth),
					     0);
	if (errno)
	    return CCS_FAILURE;
	
	*newdesc = new_desc->handle;

	return CCS_SUCCESS;
    }
    else if (is_builtin)
    {
	el_sz = CCSI_datadesc_builtin_size_macro (olddesc);
	el_type = olddesc;

	old_lb = 0;
	old_true_lb= 0;
	old_ub = el_sz;
	old_true_ub = el_sz;
	old_extent = el_sz;
	old_is_contig = 1;

	new_desc->size = count * blocklength * el_sz;
	new_desc->has_sticky_lb = 0;
	new_desc->has_sticky_ub = 0;

	new_desc->alignsize = el_sz; /* ??? */
	new_desc->eltype = el_type;
    }
    else
    {
	CCSI_datadesc_t *old_desc;

	old_desc = CCSI_datadesc_handle_ptr (olddesc);
	el_type = old_desc->eltype;

	old_lb = old_desc->lb;
	old_true_lb = old_desc->true_lb;
	old_ub = old_desc->ub;
	old_true_ub = old_desc->true_ub;
	old_extent = old_desc->extent;
	old_is_contig = old_desc->is_contig;

	new_desc->size = count * blocklength * old_desc->size;
	new_desc->has_sticky_lb = old_desc->has_sticky_lb;
	new_desc->has_sticky_ub = old_desc->has_sticky_ub;

	new_desc->alignsize = old_desc->alignsize;
	new_desc->eltype = el_type;
    }

    CCSI_DATADESC_VECTOR_LB_UB(count,
				stride,
				blocklength,
				old_lb,
				old_ub,
				old_extent,
				new_desc->lb,
				new_desc->ub);
    
    new_desc->true_lb = new_desc->lb + (old_true_lb - old_lb);
    new_desc->true_ub = new_desc->ub + (old_true_ub - old_ub);
    new_desc->extent  = new_desc->ub - new_desc->lb;

    /* new type is only contig for N types if old one was and
     * size and extent of new type are equivalent.
     *
     * Q: is this really stringent enough?  is there an overlap case
     *    for which this would fail?
     */
    new_desc->is_contig = (new_desc->size == new_desc->extent) ? old_is_contig : 0;

    /* fill in dataloop(s) */
    errno = CCSI_Dataloop_create_vector(count,
					 blocklength,
					 stride,
					 1,
					 olddesc,
					 &(new_desc->dataloop),
					 &(new_desc->dataloop_size),
					 &(new_desc->dataloop_depth),
					 0);
    if (errno)
	return CCS_FAILURE;
    
    *newdesc = new_desc->handle;

    return CCS_SUCCESS;
}

static int
CCSI_datadesc_struct_alignsize (int count, CCS_datadesc_t *array_of_descs)
{
    int i;
    int max_alignsize = 0;
    int tmp_alignsize;

    for (i = 0; i < count; ++i)
    {
	/* shouldn't be called with an LB or UB, but we'll handle it nicely */
	if (array_of_descs[i] == CCS_DATA_LB || array_of_descs[i] == CCS_DATA_UB)
	    continue;
	else if (CCS_datadesc_is_builtin (array_of_descs[i]))
	{
	    tmp_alignsize = CCSI_datadesc_builtin_size_macro (array_of_descs[i]);
	}
	else
	{
	    CCSI_datadesc_t *desc;	    

	    desc = CCSI_datadesc_handle_ptr(array_of_descs[i]);
	    tmp_alignsize = desc->alignsize;
	}
	if (max_alignsize < tmp_alignsize)
	    max_alignsize = tmp_alignsize;
    }

    return max_alignsize;
}

int
CCS_datadesc_create_struct (int count, int *array_of_blocklengths, CCS_aint_t *array_of_displacements, CCS_datadesc_t *array_of_descs,
			     CCS_datadesc_t *newdesc)
{
    int errno = CCS_SUCCESS;
    int i;
    int old_are_contig = 1;
    int found_sticky_lb = 0;
    int found_sticky_ub = 0;
    int found_true_lb = 0;
    int found_true_ub = 0;
    int size = 0;
    int el_sz = 0;
    CCS_datadesc_t el_type = CCS_DATA_NULL;
    CCS_aint_t true_lb_disp = 0;
    CCS_aint_t true_ub_disp = 0;
    CCS_aint_t sticky_lb_disp = 0;
    CCS_aint_t sticky_ub_disp = 0;
    
    CCSI_datadesc_t *new_desc;

    new_desc = CCS_malloc (sizeof (CCSI_datadesc_t));

    if (!new_desc)
	return CCS_FAILURE;

    new_desc->ref_count = 1;
    new_desc->handle = CCSI_datadesc_ptr_handle (new_desc);
    new_desc->dataloop = NULL;
    new_desc->dataloop_size = -1;
    new_desc->dataloop_depth = -1;

    if (count == 0)
    {
	
	new_desc->size = 0;
	new_desc->has_sticky_ub = 0;
	new_desc->has_sticky_lb = 0;
	new_desc->lb = 0;
	new_desc->ub = 0;
	new_desc->true_lb = 0;
	new_desc->true_ub = 0;
	new_desc->extent = 0;
	new_desc->alignsize = 0;
	new_desc->eltype = 0;
	new_desc->is_contig = 1;

	errno = CCSI_Dataloop_create_struct(0,
					     NULL,
					     NULL,
					     NULL,
					     &(new_desc->dataloop),
					     &(new_desc->dataloop_size),
					     &(new_desc->dataloop_depth),
					     0);
	if (errno)
	    return CCS_FAILURE;
	
	*newdesc = new_desc->handle;

	return CCS_SUCCESS;
    }

    for (i = 0; i < count; ++i)
    {
	int is_builtin = CCS_datadesc_is_builtin (array_of_descs[i]);
	CCS_aint_t tmp_lb, tmp_ub, tmp_true_lb, tmp_true_ub;
	int tmp_el_sz;
	CCS_datadesc_t tmp_el_type;
	CCSI_datadesc_t *old_desc = NULL;

	if (is_builtin)
	{
	    /* Q: DO LB or UBs count in element counts? */
	    tmp_el_sz   = CCSI_datadesc_builtin_size_macro (array_of_descs[i]);
	    tmp_el_type = array_of_descs[i];

	    
	    CCSI_DATADESC_BLOCK_LB_UB(array_of_blocklengths[i],
				       array_of_displacements[i],
				       0,
				       tmp_el_sz,
				       tmp_el_sz,
				       tmp_lb,
				       tmp_ub);
	    
	    tmp_true_lb = tmp_lb;
	    tmp_true_ub = tmp_ub;
	    
	    size += tmp_el_sz * array_of_blocklengths[i];
	}
	else
	{
	    old_desc = CCSI_datadesc_handle_ptr (array_of_descs[i]);

	    tmp_el_sz   = old_desc->element_size;
	    tmp_el_type = old_desc->eltype;

	    CCSI_DATADESC_BLOCK_LB_UB(array_of_blocklengths[i],
				       array_of_displacements[i],
				       old_desc->lb,
				       old_desc->ub,
				       old_desc->extent,
				       tmp_lb,
				       tmp_ub);
	    tmp_true_lb = tmp_lb + (old_desc->true_lb - old_desc->lb);
	    tmp_true_ub = tmp_ub + (old_desc->true_ub - old_desc->ub);

	    size += old_desc->size * array_of_blocklengths[i];
	}

	/* element size and type */
	if (i == 0)
	{
	    el_sz = tmp_el_sz;
	    el_type = tmp_el_type;
	}
	else if (el_sz != tmp_el_sz)
	{
	    /* Q: should LB and UB have any effect here? */
	    el_sz = -1;
	    el_type = CCS_DATA_NULL;
	}
	else if (el_type != tmp_el_type)
	{
	    /* Q: should we set el_sz = -1 even though the same? */
	    el_type = CCS_DATA_NULL;
	}

	/* keep lowest sticky lb */
	if ((array_of_descs[i] == CCS_DATA_LB) || (!is_builtin && old_desc->has_sticky_lb))
	{
	    if (!found_sticky_lb)
	    {
		found_sticky_lb = 1;
		sticky_lb_disp  = tmp_lb;
	    }
	    else if (sticky_lb_disp > tmp_lb)
	    {
		sticky_lb_disp = tmp_lb;
	    }
	}

	/* keep highest sticky ub */
	if ((array_of_descs[i] == CCS_DATA_UB) || 
	    (!is_builtin && old_desc->has_sticky_ub))
	{
	    if (!found_sticky_ub)
	    {
		found_sticky_ub = 1;
		sticky_ub_disp  = tmp_ub;
	    }
	    else if (sticky_ub_disp < tmp_ub)
	    {
		sticky_ub_disp = tmp_ub;
	    }
	}

	/* keep lowest true lb and highest true ub */
	if (array_of_descs[i] != CCS_DATA_UB && array_of_descs[i] != CCS_DATA_LB)
	{
	    if (!found_true_lb)
	    {
		found_true_lb = 1;
		true_lb_disp  = tmp_true_lb;
	    }
	    else if (true_lb_disp > tmp_true_lb)
	    {
		true_lb_disp = tmp_true_lb;
	    }
	    if (!found_true_ub)
	    {
		found_true_ub = 1;
		true_ub_disp  = tmp_true_ub;
	    }
	    else if (true_ub_disp < tmp_true_ub)
	    {
		true_ub_disp = tmp_true_ub;
	    }
	}

	if (!is_builtin && !old_desc->is_contig)
	{
	    old_are_contig = 0;
	}
    }
    
    new_desc->element_size = el_sz;
    new_desc->eltype = el_type;

    new_desc->has_sticky_lb = found_sticky_lb;
    new_desc->true_lb = true_lb_disp;
    new_desc->lb = (found_sticky_lb) ? sticky_lb_disp : true_lb_disp;

    new_desc->has_sticky_ub = found_sticky_ub;
    new_desc->true_ub = true_ub_disp;
    new_desc->ub = (found_sticky_ub) ? sticky_ub_disp : true_ub_disp;

    new_desc->alignsize = CCSI_datadesc_struct_alignsize (count, array_of_descs);

    new_desc->extent = new_desc->ub - new_desc->lb;
    if ((!found_sticky_lb) && (!found_sticky_ub))
    {
	/* account for padding */
	CCS_aint_t epsilon = new_desc->extent % new_desc->alignsize;

	if (epsilon)
	{
	    new_desc->ub += (new_desc->alignsize - epsilon);
	    new_desc->extent = new_desc->ub - new_desc->lb;
	}
    }

    new_desc->size = size;

    /* new type is contig for N types if its size and extent are the
     * same, and the old type was also contiguous
     */
    if ((new_desc->size == new_desc->extent) && old_are_contig)
    {
	new_desc->is_contig = 1;
    }
    else
    {
	new_desc->is_contig = 0;
    }

    /* fill in dataloop(s) */
    errno = CCSI_Dataloop_create_struct(count,
					 array_of_blocklengths,
					 array_of_displacements,
					 array_of_descs,
					 &(new_desc->dataloop),
					 &(new_desc->dataloop_size),
					 &(new_desc->dataloop_depth),
					 CCSI_DATALOOP_HOMOGENEOUS);
    
    if (errno)
	return CCS_FAILURE;
    
    *newdesc = new_desc->handle;

    return CCS_SUCCESS;
}

static int
CCSI_datadesc_create_indexed_zero_size (CCSI_datadesc_t *new_desc)
{
    int errno = CCS_SUCCESS;
    new_desc->has_sticky_ub = 0;
    new_desc->has_sticky_lb = 0;
    
    new_desc->alignsize = 0;
    new_desc->element_size = 0;
    new_desc->eltype = 0;
    
    new_desc->size = 0;
    new_desc->lb = 0;
    new_desc->ub = 0;
    new_desc->true_lb = 0;
    new_desc->true_ub = 0;
    new_desc->extent = 0;
    
    new_desc->is_contig  = 1;
    
    errno = CCSI_Dataloop_create_indexed(0,
					NULL,
					NULL,
					0,
					CCS_DATA8, /* dummy type */
					&(new_desc->dataloop),
					&(new_desc->dataloop_size),
					&(new_desc->dataloop_depth),
					0);
    if (errno)
	return CCS_FAILURE;
    
    return CCS_SUCCESS;
}

/* CCSI_datadesc_indexed_count_contig()
 *
 * Determines the actual number of contiguous blocks represented by the
 * blocklength/displacement arrays.  This might be less than count (as
 * few as 1).
 *
 * Extent passed in is for the original type.
 */
int
CCSI_datadesc_indexed_count_contig (int count, int *array_of_blocklengths, CCS_aint_t *array_of_displacements, CCS_aint_t old_extent)
{
    int i, contig_count = 1;
    int cur_blklen = array_of_blocklengths[0];


    CCS_aint_t cur_bdisp = array_of_displacements[0];
    
    for (i = 1; i < count; i++)
    {
	if (array_of_blocklengths[i] == 0)
	{
	    continue;
	}
	else if (cur_bdisp + cur_blklen * old_extent == array_of_displacements[i])
	{
	    /* adjacent to current block; add to block */
	    cur_blklen += array_of_blocklengths[i];
	}
	else
	{
	    cur_bdisp  = array_of_displacements[i];
	    cur_blklen = array_of_blocklengths[i];
	    contig_count++;
	}
    }
    return contig_count;
}

int
CCS_datadesc_create_indexed (int count, int *array_of_blocklengths, CCS_aint_t *array_of_displacements, CCS_datadesc_t olddesc,
			      CCS_datadesc_t *newdesc)
{
    int errno = CCS_SUCCESS;
    int is_builtin, old_is_contig;
    int i, contig_count;
    int el_sz, el_ct, old_ct, old_sz;
    CCS_aint_t old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    CCS_aint_t min_lb = 0, max_ub = 0;
    CCS_datadesc_t el_type;

    CCSI_datadesc_t *new_desc;

    new_desc = CCS_malloc (sizeof (CCSI_datadesc_t));

    if (!new_desc)
	return CCS_FAILURE;

    new_desc->ref_count = 1;
    new_desc->handle = CCSI_datadesc_ptr_handle (new_desc);

    new_desc->dataloop_size = -1;
    new_desc->dataloop = NULL;
    new_desc->dataloop_depth = -1;

    is_builtin = CCS_datadesc_is_builtin (olddesc);

    if (count == 0)
    {
	errno = CCSI_datadesc_create_indexed_zero_size(new_desc);
	if (errno == CCS_SUCCESS) {
	    *newdesc = new_desc->handle;
	}
	return errno;
    }
    else if (is_builtin)
    {
	/* builtins are handled differently than user-defined types because
	 * they have no associated dataloop or datatype structure.
	 */
	el_sz = CCSI_datadesc_builtin_size_macro (olddesc);
	old_sz = el_sz;
	el_ct = 1;
	el_type = olddesc;

	old_lb = 0;
	old_true_lb = 0;
	old_ub = el_sz;
	old_true_ub = el_sz;
	old_extent = el_sz;
	old_is_contig = 1;

	new_desc->has_sticky_ub = 0;
	new_desc->has_sticky_lb = 0;

	new_desc->alignsize = el_sz; /* ??? */
	new_desc->element_size = el_sz;
	new_desc->eltype = el_type;
    }
    else
    {
	/* user-defined base type (oldtype) */
	CCSI_datadesc_t *old_desc;

	old_desc = CCSI_datadesc_handle_ptr (olddesc);
	el_sz = old_desc->element_size;
	old_sz = old_desc->size;
	el_type = old_desc->eltype;

	old_lb = old_desc->lb;
	old_true_lb = old_desc->true_lb;
	old_ub = old_desc->ub;
	old_true_ub = old_desc->true_ub;
	old_extent = old_desc->extent;
	old_is_contig = old_desc->is_contig;

	new_desc->has_sticky_lb = old_desc->has_sticky_lb;
	new_desc->has_sticky_ub = old_desc->has_sticky_ub;
	new_desc->element_size = el_sz;
	new_desc->eltype = el_type;
    }


    /* find the first nonzero blocklength element */
    i = 0;
    while (i < count && array_of_blocklengths[i] == 0)
	i++;

    if (i == count)
    {
	/* no nonzero blocks; treat the same as the count == 0 case */
	errno = CCSI_datadesc_create_indexed_zero_size(new_desc);
	if (errno == CCS_SUCCESS) {
	    *newdesc = new_desc->handle;
	}
	return errno;
    }

    /* priming for loop */
    old_ct = array_of_blocklengths[i];
    
    CCSI_DATADESC_BLOCK_LB_UB((CCS_aint_t) array_of_blocklengths[i],
			       array_of_displacements[i],
			       old_lb,
			       old_ub,
			       old_extent,
			       min_lb,
			       max_ub);
    
    /* determine min lb, max ub, and count of old types in remaining
     * nonzero size blocks
     */
    for (i++; i < count; i++)
    {
	CCS_aint_t tmp_lb, tmp_ub;
	
	if (array_of_blocklengths[i] > 0)
	{
	    old_ct += array_of_blocklengths[i]; /* add more oldtypes */
	
	    /* calculate ub and lb for this block */
	    CCSI_DATADESC_BLOCK_LB_UB((CCS_aint_t) array_of_blocklengths[i],
				       array_of_displacements[i],
				       old_lb,
				       old_ub,
				       old_extent,
				       tmp_lb,
				       tmp_ub);
	    
	    if (tmp_lb < min_lb)
		min_lb = tmp_lb;
	    if (tmp_ub > max_ub)
		max_ub = tmp_ub;
	}
    }
    
    new_desc->size = old_ct * old_sz;
    
    new_desc->lb = min_lb;
    new_desc->ub = max_ub;
    new_desc->true_lb = min_lb + (old_true_lb - old_lb);
    new_desc->true_ub = max_ub + (old_true_ub - old_ub);
    new_desc->extent = max_ub - min_lb;
    
    /* new type is only contig for N types if it's all one big
     * block, its size and extent are the same, and the old type
     * was also contiguous.
     */
    contig_count = CCSI_datadesc_indexed_count_contig(count,
						       array_of_blocklengths,
						       array_of_displacements,
						       old_extent);
    
    if ((contig_count == 1) && (new_desc->size == new_desc->extent))
    {
	new_desc->is_contig = old_is_contig;
    }
    else
    {
	new_desc->is_contig = 0;
    }

    /* fill in dataloop(s) */
    errno = CCSI_Dataloop_create_indexed(count,
					  array_of_blocklengths,
					  array_of_displacements,
					  1,
					  olddesc,
					  &(new_desc->dataloop),
					  &(new_desc->dataloop_size),
					  &(new_desc->dataloop_depth),
					  0);
    if (errno)
	return CCS_FAILURE;

    *newdesc = new_desc->handle;
    return CCS_SUCCESS;
}

int
CCSI_datadesc_blockindexed_count_contig(int count, int blklen, CCS_aint_t *disp_array, CCS_aint_t old_extent)
{
    int i, contig_count = 1;
    int cur_bdisp = disp_array[0];
    
    for (i = 1; i < count; i++)
    {
	if (cur_bdisp + blklen * old_extent != disp_array[i])
	{
	    contig_count++;
	}
	cur_bdisp = disp_array[i];
    }
    
    return contig_count;
}

int
CCS_datadesc_create_blockindexed (int count, int blocklength, CCS_aint_t *array_of_displacements, CCS_datadesc_t olddesc,
				   CCS_datadesc_t *newdesc)
{
    int errno = CCS_SUCCESS, i;
    int is_builtin, contig_count, old_is_contig;
    CCS_aint_t el_sz;
    CCS_datadesc_t el_type;
    CCS_aint_t old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    CCS_aint_t min_lb = 0, max_ub = 0;

    CCSI_datadesc_t *new_desc;

    new_desc = CCS_malloc (sizeof (CCSI_datadesc_t));

    if (!new_desc)
	return CCS_FAILURE;

    new_desc->ref_count = 1;
    new_desc->handle = CCSI_datadesc_ptr_handle (new_desc);

    new_desc->dataloop_size = -1;
    new_desc->dataloop = NULL;
    new_desc->dataloop_depth = -1;

    is_builtin = CCS_datadesc_is_builtin (olddesc);

    if (count == 0)
    {
	/* we are interpreting the standard here based on the fact that
	 * with a zero count there is nothing in the typemap.
	 *
	 * we handle this case explicitly to get it out of the way.
	 */
	new_desc->size = 0;
	new_desc->has_sticky_ub = 0;
	new_desc->has_sticky_lb = 0;

	new_desc->alignsize = 0;
	new_desc->element_size = 0;
	new_desc->eltype = 0;

	new_desc->lb = 0;
	new_desc->ub = 0;
	new_desc->true_lb = 0;
	new_desc->true_ub = 0;
	new_desc->extent = 0;

	new_desc->is_contig = 1;

	errno = CCSI_Dataloop_create_blockindexed(0,
						   0,
						   NULL,
						   0,
						   CCS_DATA8, /* dummy type */
						   &(new_desc->dataloop),
						   &(new_desc->dataloop_size),
						   &(new_desc->dataloop_depth),
						   0);
	if (errno)
	    return CCS_FAILURE;
	
	*newdesc = new_desc->handle;
	return CCS_SUCCESS;
    }
    else if (is_builtin)
    {
	el_sz   = CCSI_datadesc_builtin_size_macro (olddesc);
	el_type = olddesc;

	old_lb = 0;
	old_true_lb = 0;
	old_ub = el_sz;
	old_true_ub = el_sz;
	old_extent = el_sz;
	old_is_contig = 1;

	new_desc->size = count * blocklength * el_sz;
	new_desc->has_sticky_lb = 0;
	new_desc->has_sticky_ub = 0;

	new_desc->alignsize = el_sz; /* ??? */
	new_desc->element_size = el_sz;
	new_desc->eltype = el_type;
    }
    else
    {
	/* user-defined base type (olddesc) */
	CCSI_datadesc_t *old_desc;

	old_desc = CCSI_datadesc_handle_ptr (olddesc);
	el_sz = old_desc->element_size;
	el_type = old_desc->eltype;

	old_lb = old_desc->lb;
	old_true_lb = old_desc->true_lb;
	old_ub = old_desc->ub;
	old_true_ub = old_desc->true_ub;
	old_extent = old_desc->extent;
	old_is_contig = old_desc->is_contig;

	new_desc->size = count * blocklength * old_desc->size;
	new_desc->has_sticky_lb = old_desc->has_sticky_lb;
	new_desc->has_sticky_ub = old_desc->has_sticky_ub;

	new_desc->alignsize = old_desc->alignsize;
	new_desc->element_size = el_sz;
	new_desc->eltype = el_type;
    }
    
    /* priming for loop */
    CCSI_DATADESC_BLOCK_LB_UB((CCS_aint_t) blocklength,
			       array_of_displacements[0],
			       old_lb,
			       old_ub,
			       old_extent,
			       min_lb,
			       max_ub);

    /* determine new min lb and max ub */
    for (i = 1; i < count; i++)
    {
	CCS_aint_t tmp_lb, tmp_ub;

	CCSI_DATADESC_BLOCK_LB_UB((CCS_aint_t) blocklength,
				   array_of_displacements[i],
				   old_lb,
				   old_ub,
				   old_extent,
				   tmp_lb,
				   tmp_ub);

	if (tmp_lb < min_lb)
	    min_lb = tmp_lb;
	if (tmp_ub > max_ub)
	    max_ub = tmp_ub;
    }

    new_desc->lb = min_lb;
    new_desc->ub = max_ub;
    new_desc->true_lb = min_lb + (old_true_lb - old_lb);
    new_desc->true_ub = max_ub + (old_true_ub - old_ub);
    new_desc->extent = max_ub - min_lb;

    /* new type is contig for N types if it is all one big block,
     * its size and extent are the same, and the old type was also
     * contiguous.
     */
    if (old_is_contig && (new_desc->size == new_desc->extent))
    {
	contig_count = CCSI_datadesc_blockindexed_count_contig(count,
								blocklength,
								array_of_displacements,
								old_extent);
	new_desc->is_contig = (contig_count == 1) ? 1 : 0;
    }
    else
    {
	new_desc->is_contig = 0;
    }

    errno = CCSI_Dataloop_create_blockindexed(count,
					       blocklength,
					       array_of_displacements,
					       1,
					       olddesc,
					       &(new_desc->dataloop),
					       &(new_desc->dataloop_size),
					       &(new_desc->dataloop_depth),
					       0);
    if (errno)
	return CCS_FAILURE;

    *newdesc = new_desc->handle;
    return CCS_SUCCESS;
}

void
CCS_datadesc_free (CCS_datadesc_t *d)
{
    CCSI_datadesc_t *desc = CCSI_datadesc_handle_ptr (*d);

    if (CCSI_datadesc_release_ref (desc))
	*d = CCS_DATA_NULL;
}

void
CCSI_datadesc_add_ref (CCSI_datadesc_t *desc)
{
    ++desc->ref_count;
}

int
CCSI_datadesc_release_ref (CCSI_datadesc_t *desc)
{
    --desc->ref_count;
    if (desc->ref_count == 0)
    {
	CCSI_Dataloop_free (&(desc->dataloop));
	CCS_free (desc);
	return 1;
    }

    return 0;
}
