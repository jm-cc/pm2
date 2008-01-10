/*
 *  (C) 2006 by Argonne National Laboratory.
 *   Contact:  Darius Buntinas
 */ 

#include <ccsi.h>
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

static void
CCSI_null_handler (CCS_token_t token, void *buffer, unsigned buf_len, unsigned num_args, ...)
{
    CCSI_ERROR ("Uninitialized handler called on process %d by process %d", CCSI_rank, token->source);
    CCSI_abort();
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

