#ifndef CCSI_H
#define CCSI_H
#include <stdlib.h>
#include <string.h>

#ifdef __STRICT_ANSI__
#define inline
#endif

#include <stdint.h>

#include "ccs.h"
#include "ccsi_datadesc.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define CCSI_memset(x...) memset(x)
#define CCSI_memcpy(x...) memcpy(x)

#define CCSI_abort() abort()

#define CCSI_Assert CCSI_assert
#define CCSI_assert(x) do {							\
    if (!(x))									\
    {										\
	printf ("Assert failed at %s:%d \"%s\"\n", __FILE__, __LINE__, #x);	\
	abort ();								\
    }										\
} while (0)

#if defined(CCSI_DEBUG) && (CCSI_DEBUG > 0)
#define CCSI_dbg_printf(x...) printf (x)
#else
#define CCSI_dbg_printf(x...) do { } while(0)
#endif

#define CCSI_ERROR(x...) CCSI_error_ (__FILE__, __LINE__, x)
void CCSI_error_ (const char *file, unsigned line, const char *format, ...);

#define CCSI_malloc(x) malloc(x)
#define CCSI_free(x) free(x)

extern unsigned CCSI_size;
extern unsigned CCSI_rank;

enum CCSI_pkt_type
{
    CCSI_PKT_AM_REQUEST = 1,
    CCSI_PKT_AM_REPLY
};

typedef struct CCSI_pkt_am
{
    uint8_t type;
    uint8_t context_id;
    uint8_t handler_id;
    uint8_t num_args;
    uint16_t src_rank;
    uint16_t dest_rank;
    uint32_t data_size;
    /* CCS_handler_arg_t args[num_args]; */
    /* uint8_t data[data_size]; */
}
CCSI_pkt_am_t;

#if (CCS_MAX_CONTEXTS > 256)
#error CCS_MAX_CONTEXTS can be no greater than 256
#endif
#if (CCS_MAX_HANDLERS > 256)
#error CCS_MAX_HANDLERS can be no greater than 256
#endif
#if (CCS_MAX_HANDLER_ARGS > 256)
#error CCS_MAX_HANDLER_ARGS can be no greater than 256
#endif

#define CCSI_PKT_AM_GET_ARG_PTR(pkt_p) ((CCS_handler_arg_t *) (&((CCSI_pkt_am_t *)(pkt_p))[1]))
#define CCSI_PKT_AM_GET_DATA_PTR(pkt_p) ((uint8_t *) (&(CCSI_PKT_AM_GET_ARG_PTR (pkt_p))[((CCSI_pkt_am_t *)(pkt_p))->num_args]))
#define CCSI_PKT_AM_GET_PKT_LEN(pkt_p) (sizeof (CCSI_pkt_am_t) +						\
				         sizeof(CCS_handler_arg_t) * ((CCSI_pkt_am_t *)(pkt_p))->num_args +	\
				         ((CCSI_pkt_am_t *)(pkt_p))->data_size)

typedef union CCSI_pkt
{
    uint8_t type;
    CCSI_pkt_am_t am;
}
CCSI_pkt_t;

int CCSI_dev_init ();
int CCSI_dev_finalize ();

typedef struct CCS_context
{
    CCS_amhandler_t handler[CCS_MAX_HANDLERS];
}
CCSI_context_t;

extern int CCSI_num_contexts;
extern CCSI_context_t *CCSI_context_table[CCS_MAX_CONTEXTS];

struct CCS_token
{
    unsigned context_id;
    unsigned source;
    int is_request;
};

static inline void
CCSI_local_lock (int *lock)
{
    int val;
    int data = 1;

    do 
    {
	asm volatile ("xchgl %0,%1"
		      :"=r" (val)
		      :"m" (*lock), "0" (data)
		      :"memory");
    } while (val);
}

static inline void
CCSI_local_unlock (int *lock)
{
    *lock = 0;
}


#endif
