
#ifndef MADELEINE_EST_DEF
#define MADELEINE_EST_DEF

#include <sys/types.h>

#include "pointers.h"
#include "mad_types.h"
#include "mad_timing.h"
#include "sys/debug.h"

#ifdef PM2

#include "marcel.h"
extern exception ALIGN_ERROR;

#else

#ifndef FALSE
typedef enum { FALSE, TRUE } boolean;
#else
typedef int boolean;
#endif

#endif

typedef enum {
  MAD_IN_HEADER,
  MAD_IN_PLACE,
  MAD_IN_PLACE_N_FREE,
  MAD_BY_COPY
} madeleine_part;


void mad_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami);

void mad_buffers_init(void);

void mad_exit(void);

char *mad_arch_name(void);

boolean mad_can_send_to_self(void);

void mad_sendbuf_init(int dest_node);

void mad_sendbuf_send(void);

void mad_receive(void);

void mad_recvbuf_receive(void);

void old_mad_pack_byte(madeleine_part where, char *data, size_t nb);
void old_mad_unpack_byte(madeleine_part where, char *data, size_t nb);

void old_mad_pack_short(madeleine_part where, short *data, size_t nb);
void old_mad_unpack_short(madeleine_part where, short *data, size_t nb);

void old_mad_pack_int(madeleine_part where, int *data, size_t nb);
void old_mad_unpack_int(madeleine_part where, int *data, size_t nb);

void old_mad_pack_long(madeleine_part where, long *data, size_t nb);
void old_mad_unpack_long(madeleine_part where, long *data, size_t nb);

void old_mad_pack_float(madeleine_part where, float *data, size_t nb);
void old_mad_unpack_float(madeleine_part where, float *data, size_t nb);

void old_mad_pack_double(madeleine_part where, double *data, size_t nb);
void old_mad_unpack_double(madeleine_part where, double *data, size_t nb);

void old_mad_pack_pointer(madeleine_part where, pointer *data, size_t nb);
void old_mad_unpack_pointer(madeleine_part where, pointer *data, size_t nb);

void old_mad_pack_str(madeleine_part where, char *data);
void old_mad_unpack_str(madeleine_part where, char *data);

char *mad_aligned_malloc(int size);
void mad_aligned_free(void *ptr);

#endif
