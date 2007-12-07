/*
 *  (C) 2006 by Argonne National Laboratory.
 *   Contact:  Darius Buntinas
 */ 

#include <ccsi.h>
#include <ccsi_dev.h>
#include <stdarg.h>
#include <segment.h>
#include <ccs_dataloop.h>

int CCSI_num_contexts;
CCSI_context_t *CCSI_context_table[CCS_MAX_CONTEXTS];
int CCSI_initialized = 0;
unsigned CCSI_size;
unsigned CCSI_rank;

int *CCSI_barrier_flags[2];
int CCSI_barrier_sense;

static int CCSI_atomics_lock;

/* Internal handlers */
CCS_context_t CCSI_private_context;

static void CCSI_barrier_handler (CCS_token_t token, void *buffer, unsigned buf_len, unsigned num_args, ...);
#define CCSI_BARRIER_HANDLER_ID 1
static void CCSI_atomic_handler (CCS_token_t token, void *buffer, unsigned buf_len, unsigned num_args, ...);
#define CCSI_ATOMIC_HANDLER_ID 2
static void CCSI_atomic_reply_handler (CCS_token_t token, void *buffer, unsigned buf_len, unsigned num_args, ...);
#define CCSI_ATOMIC_REPLY_HANDLER_ID 3

static void
CCSI_null_handler (CCS_token_t token, void *buffer, unsigned buf_len, unsigned num_args, ...)
{
    CCSI_ERROR ("Uninitialized handler called on process %d by process %d", CCSI_rank, token->source);
    CCSI_abort();
}



int
CCS_init ()
{
    int errno = CCS_SUCCESS;
    int i;

    CCSI_num_contexts = 0;

    errno = CCSI_dev_init ();
    if (errno != CCS_SUCCESS)
	goto end_l;

    /* initialize structures for barrier op */
    CCSI_barrier_flags[0] = CCS_malloc (sizeof (int) * CCSI_size);
    if (!CCSI_barrier_flags[0])
    {
	errno = CCS_NOMEM;
	goto end_l;
    }
    CCSI_barrier_flags[1] = CCS_malloc (sizeof (int) * CCSI_size);
    if (!CCSI_barrier_flags[1])
    {
	errno = CCS_NOMEM;
	goto end_l;
    }

    for (i = 0; i < CCSI_size; ++i)
	CCSI_barrier_flags[0][i] = CCSI_barrier_flags[1][i] = 0;
	    CCSI_barrier_sense = 0;

    errno = CCS_new_context (&CCSI_private_context);
    if (errno != CCS_SUCCESS)
	goto end_l;

    errno = CCS_register_handler (CCSI_private_context, CCSI_BARRIER_HANDLER_ID, CCSI_barrier_handler);
    if (errno != CCS_SUCCESS)
	goto end_l;

    /* initialize structures for atomic ops */

    CCSI_atomics_lock = 0;

    errno = CCS_register_handler (CCSI_private_context, CCSI_ATOMIC_HANDLER_ID, CCSI_atomic_handler);
    if (errno != CCS_SUCCESS)
	goto end_l;
    errno = CCS_register_handler (CCSI_private_context, CCSI_ATOMIC_REPLY_HANDLER_ID, CCSI_atomic_reply_handler);
    if (errno != CCS_SUCCESS)
	goto end_l;


    CCSI_initialized = 1;

end_l:
    return errno;
}

int
CCS_finalize ()
{
    int errno = CCS_SUCCESS;

    errno = CCSI_dev_finalize ();

    return errno;
}

int
CCS_new_context (CCS_context_t *context)
{
    CCSI_context_t *c;
    int i;

    if (CCSI_num_contexts == CCS_MAX_CONTEXTS)
	return CCS_FAILURE;

    c = malloc (sizeof (*c));
    if (!c)
	return CCS_NOMEM;

    for (i = 0; i < CCS_MAX_HANDLERS; ++i)
	c->handler[i] = CCSI_null_handler;

    CCSI_context_table[CCSI_num_contexts] = c;
    *context = CCSI_num_contexts;
    ++CCSI_num_contexts;

    return CCS_SUCCESS;
}

int
CCS_get_size (CCS_node_t *size)
{
    *size = CCSI_size;
    return CCS_SUCCESS;
}

int
CCS_get_rank (CCS_node_t *rank)
{
    *rank = CCSI_rank;
    return CCS_SUCCESS;
}


int
CCS_register_handler (CCS_context_t context, unsigned handler_id, CCS_amhandler_t handler_ptr)
{
    int errno = CCS_SUCCESS;

    if (context >= CCSI_num_contexts)
    {
	CCSI_ERROR ("Invalid context id");
	errno = CCS_FAILURE;
	goto exit_l;
    }
    if (handler_id >= CCS_MAX_HANDLERS)
    {
	CCSI_ERROR ("Invalid handler id");
	errno = CCS_FAILURE;
	goto exit_l;
    }

    CCSI_context_table[context]->handler[handler_id] = handler_ptr;

exit_l:
    return errno;
}

int
CCS_sender_rank (CCS_token_t token, CCS_node_t *sender)
{
    struct CCS_token *t = token;

    if (t->source < CCSI_size)
    {
	*sender = t->source;
	return CCS_SUCCESS;
    }
    else
	return CCS_FAILURE;
}


int
CCS_amrequest (CCS_context_t context, CCS_node_t node, void *buf, unsigned count, CCS_datadesc_t datadesc,
		unsigned handler_id, unsigned num_args, ...)
{
    int errno = CCS_SUCCESS;
    va_list ap;

    /* check the arguments */
    CCSI_assert (context < CCSI_num_contexts);
    CCSI_assert (node < CCSI_size);
    CCSI_assert (count == 0 || buf);
    CCSI_assert (num_args <= CCS_MAX_HANDLER_ARGS);

    va_start (ap, num_args);

    errno = CCSI_dev_amrequest (context, node, buf, count, datadesc, handler_id, num_args, ap);

    va_end (ap);

    return errno;
}


int
CCS_amreply (CCS_token_t token, void *buf, unsigned count, CCS_datadesc_t datadesc, unsigned handler_id, unsigned num_args, ...)
{
    int errno = CCS_SUCCESS;
    va_list ap;

    /* check the arguments */
    CCSI_assert (token);
    CCSI_assert (token->context_id < CCSI_num_contexts);
    CCSI_assert (token->source < CCSI_size);
    CCSI_assert (count == 0 || buf);
    CCSI_assert (num_args <= CCS_MAX_HANDLER_ARGS);

    if (!token->is_request)
    {
	CCSI_ERROR ("CCSI_amreply called from inside a reply handler");
	errno = CCS_FAILURE;
	goto exit_l;
    }

    va_start (ap, num_args);

    errno = CCSI_dev_amreply (token, buf, count, datadesc, handler_id, num_args, ap);

    va_end (ap);

exit_l:
    return errno;
}

void
CCS_poll ()
{
    CCSI_dev_poll ();
}

void
CCSI_error_ (const char *file, unsigned line, const char *format, ...)
{
#define CCSI_ERROR_STRING_LEN_ 256
    char s[CCSI_ERROR_STRING_LEN_];
    va_list ap;

    if (CCSI_initialized)
	snprintf (s, CCSI_ERROR_STRING_LEN_, "CCSI ERROR: %d %s:%d %s\n", CCSI_rank, file, line, format);
    else
	snprintf (s, CCSI_ERROR_STRING_LEN_, "CCSI ERROR: %s:%d %s\n", file, line, format);

    s[CCSI_ERROR_STRING_LEN_-1] = 0;

    va_start (ap, format);
    vprintf (s, ap);
    va_end (ap);
}


static void
CCSI_barrier_handler (CCS_token_t token, void *buffer, unsigned buf_len, unsigned num_args, ...)
{
    va_list ap;
    CCS_node_t sender;
    unsigned sense;

    va_start(ap, num_args);

    sense = va_arg (ap, unsigned);
    CCS_sender_rank (token, &sender);
    CCSI_barrier_flags[sense][sender] = 1;

    va_end(ap);
}

int CCS_barrier ()
{
    int errno = CCS_SUCCESS;
    int mask;
    int dst, src;

#ifdef CCSI_DEV_BARRIER_FN
    return CCSI_DEV_BARRIER_FN ();
#endif

    mask = 0x1;

    while(mask < CCSI_size)
    {
	dst = (CCSI_rank + mask) % CCSI_size;
	src = (CCSI_rank - mask + CCSI_size) % CCSI_size;

	errno = CCS_amrequest (CCSI_private_context, dst, NULL, 0, CCS_DATA_NULL, CCSI_BARRIER_HANDLER_ID, 1,
				CCSI_barrier_sense);
	if (errno != CCS_SUCCESS)
	{
	    CCSI_ERROR ("CCS_amrequest() failed in CCS_barrier()");
	    goto end_l;
	}

	while(!CCSI_barrier_flags[CCSI_barrier_sense][src])
	    CCS_poll ();
	CCSI_barrier_flags[CCSI_barrier_sense][src] = 0;

	mask <<= 1;
    }

    /* flip sense bit */
    CCSI_barrier_sense ^= 1;

end_l:
    return errno;
}

int
CCS_pack (void *dest, void *src, int count, CCS_datadesc_t desc, unsigned bytes)
{
    struct CCSI_Segment segment;
    int errno = CCS_SUCCESS;
    int last = bytes;

    errno = CCSI_Segment_init (src, count, desc, &segment, 0);
    if (errno != 0)
    {
	errno = CCS_FAILURE;
	goto end_l;
    }

    CCSI_Segment_pack (&segment, 0, &last, dest);

end_l:
    return errno;
}

int
CCS_unpack (void *dest, void *src, int count, CCS_datadesc_t desc, unsigned bytes)
{
    struct CCSI_Segment segment;
    int errno = CCS_SUCCESS;
    int last = bytes;

    errno = CCSI_Segment_init (dest, count, desc, &segment, 0);
    if (errno != 0)
    {
	errno = CCS_FAILURE;
	goto end_l;
    }

    CCSI_Segment_unpack (&segment, 0, &last, src);

end_l:
    return errno;
}

int
CCS_register_mem (void *addr, unsigned len)
{
    /* eventually use registration cache */
    return CCSI_dev_register_mem (addr, len);
}

int
CCS_deregister_mem (void *addr, unsigned len)
{
    return CCSI_dev_deregister_mem (addr, len);
}

int
CCS_put (CCS_node_t node, void *src_buf, unsigned src_count, CCS_datadesc_t src_datadesc,
	  void *dest_buf, unsigned dest_count, CCS_datadesc_t dest_datadesc, CCS_callback_t callback, void *callback_arg)
{
    return CCSI_dev_put (node, src_buf, src_count, src_datadesc, dest_buf, dest_count, dest_datadesc, callback, callback_arg);
}

int
CCS_get (CCS_node_t node, void *src_buf, unsigned src_count, CCS_datadesc_t src_datadesc,
	  void *dest_buf, unsigned dest_count, CCS_datadesc_t dest_datadesc, CCS_callback_t callback, void *callback_arg)
{
    return CCSI_dev_get (node, src_buf, src_count, src_datadesc, dest_buf, dest_count, dest_datadesc, callback, callback_arg);
}

int
CCS_datadesc_extent (CCS_datadesc_t datadesc)
{
    int extent;

    DLOOP_Handle_get_extent_macro (datadesc, extent);
    return extent;
}

int
CCS_datadesc_size (CCS_datadesc_t datadesc)
{
    int size;

    DLOOP_Handle_get_size_macro(datadesc, size);
    return size;
}

/* Since the active message args are only 32 bits, if CCS_atomic_type_t is 64 bits we need to be able to split it */
union CCSI_split_atomic_type
{
    CCS_atomic_type_t whole;
    struct
    {
	unsigned one;
	unsigned two;
    } half;
};


static void
CCSI_do_atomic (CCS_atomic_op_t op, CCS_atomic_type_t *var, CCS_atomic_type_t val, CCS_atomic_type_t cmp, CCS_atomic_type_t *result)
{
    CCSI_assert (var);
    CCSI_assert (op == CCS_ATOMIC_WRITE || result);

    CCSI_local_lock (&CCSI_atomics_lock);

    switch (op)
    {
    case CCS_ATOMIC_SWAP :
	*result = *var;
	*var = val;
	break;
    case CCS_ATOMIC_COMPARESWAP :
	*result = *var;
	if (*var == cmp)
	    *var = val;
	break;
    case CCS_ATOMIC_FETCHADD :
	*result = *var;
	*var += val;
	break;
    case CCS_ATOMIC_READ :
	*result = *var;
	break;
    case CCS_ATOMIC_WRITE :
	*var = val;
	break;
    default:
	CCSI_ERROR ("Internal error: Invalid atomic op type %d", op);
	CCSI_abort ();
    }

    CCSI_local_unlock (&CCSI_atomics_lock);
}

static void
CCSI_atomic_handler (CCS_token_t token, void *buffer, unsigned buf_len, unsigned num_args, ...)
{
    int errno = CCS_SUCCESS;
    va_list ap;
    CCS_atomic_op_t op;
    CCS_atomic_type_t *var;
    union CCSI_split_atomic_type val_u;
    union CCSI_split_atomic_type cmp_u;
    CCS_atomic_type_t *result_p;
    CCS_callback_t callback;
    void *callback_arg;
    union CCSI_split_atomic_type res_u;

    CCSI_assert (num_args == 9);

    va_start (ap, num_args);

    op = va_arg (ap, CCS_atomic_op_t);
    var = va_arg (ap, CCS_atomic_type_t *);
    val_u.half.one = va_arg (ap, unsigned);
    val_u.half.two = va_arg (ap, unsigned);
    cmp_u.half.one = va_arg (ap, unsigned);
    cmp_u.half.two = va_arg (ap, unsigned);
    result_p = va_arg (ap, CCS_atomic_type_t *);
    callback = va_arg (ap, CCS_callback_t);
    callback_arg = va_arg (ap, void *);
    va_end (ap);

    CCSI_assert (var);
    CCSI_assert (callback);

    CCSI_do_atomic (op, var, val_u.whole, cmp_u.whole, &res_u.whole);

    errno = CCS_amreply (token, NULL, 0, CCS_DATA_NULL, CCSI_ATOMIC_REPLY_HANDLER_ID, 5, res_u.half.one, res_u.half.two, result_p,
			  callback, callback_arg);
    CCSI_assert (errno == CCS_SUCCESS);
}

static void
CCSI_atomic_reply_handler (CCS_token_t token, void *buffer, unsigned buf_len, unsigned num_args, ...)
{
    va_list ap;
    CCS_atomic_type_t *result_p;
    CCS_callback_t callback;
    void *callback_arg;
    CCS_atomic_type_t res;

    CCSI_assert (num_args == 5);

    va_start (ap, num_args);
    res = va_arg (ap, CCS_atomic_type_t);
    result_p = va_arg (ap, CCS_atomic_type_t *);
    callback = va_arg (ap, CCS_callback_t);
    callback_arg = va_arg (ap, void *);
    va_end (ap);

    CCSI_assert (callback);

    if (result_p)
	*result_p = res;

    callback (callback_arg);
}

int
CCS_atomic (CCS_node_t node, CCS_atomic_op_t op, CCS_atomic_type_t *var, CCS_atomic_type_t val, CCS_atomic_type_t cmp,
	     CCS_atomic_type_t *result, CCS_callback_t callback, void *callback_arg)
{
    int errno = CCS_SUCCESS;

    CCSI_assert (node < CCSI_size);
    CCSI_assert (op < CCS_NUM_ATOMIC_OPS);
    CCSI_assert (var);
    CCSI_assert (op == CCS_ATOMIC_WRITE || result);
    CCSI_assert (callback);

    if (node == CCSI_rank)
    {
	CCSI_do_atomic (op, var, val, cmp, result);
    }
    else
    {
	union CCSI_split_atomic_type val_u;
	union CCSI_split_atomic_type cmp_u;

	val_u.whole = val;
	cmp_u.whole = cmp;

	/* if we're doing a put, we don't want to write any result */
	if (op == CCS_ATOMIC_WRITE)
	    result = NULL;

	errno = CCS_amrequest (CCSI_private_context, node, NULL, 0, CCS_DATA_NULL, CCSI_ATOMIC_HANDLER_ID, 9, op, var,
				val_u.half.one, val_u.half.two, cmp_u.half.one, cmp_u.half.two, result, callback, callback_arg);
    }

    return errno;
}

