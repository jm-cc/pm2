
#ifndef MAD_TYPES_EST_DEF
#define MAD_TYPES_EST_DEF

/* 
 * PIGGYBACK_AREA_LEN is the number of long integers that *must* be
 * reserved at the beginning of the first iovec by the user.  In other
 * words, the first iovec of a message passed to `network_send' must
 * always have a size >= PIGGYBACK_AREA_LEN and user data will begin at
 * offset PIGGYBACK_ARE_LEN*sizeof(long) in the first vector.
 */

#define PIGGYBACK_AREA_LEN    1

typedef void (*network_handler)(void *first_iov_base, size_t first_iov_len);

#define MAX_MODULES	      64
#define MAX_HEADER            (64*1024)
#define MAX_IOVECS            128

#define NETWORK_ASK_USER      -1
#define NETWORK_ONE_PER_NODE  0

#define MAD_ALIGNMENT         32

#ifdef __GNUC__
#define __MAD_ALIGNED__       __attribute__ ((aligned (MAD_ALIGNMENT)))
#else
#define __MAD_ALIGNED__
#endif

#endif



