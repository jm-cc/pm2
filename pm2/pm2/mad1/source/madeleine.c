/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

/*#define DEBUG*/

#include <madeleine.h>
#include <mad_types.h>
#include <mad_timing.h>

#include <sys/netinterf.h>

#include <string.h>
#include <stdio.h>
#include <sys/uio.h>

#define NOCHECK

#ifdef PM2

exception ALIGN_ERROR = "ALIGNMENT ERROR";
#define CUR_BUF ((struct s_buffer *)marcel_getspecific(buffer_key))
static marcel_key_t buffer_key;

#else

char *ALIGN_ERROR = "ALIGNMENT ERROR", *CONSTRAINT_ERROR = "CONSTRAINT_ERROR";
#include <safe_malloc.h>
#define CUR_BUF buffer_pool
#define tmalloc MALLOC
#define tfree   FREE

#define RAISE(e) fprintf(stderr, "%s\n", e), exit(1)

#endif

/* Pour les données dans le header : */
#define ALIGN_FRONT      sizeof(long)
#define ALIGN(X)         ((((long)X)+(ALIGN_FRONT-1)) & ~(ALIGN_FRONT-1))

/* Pour les données "IN_PLACE" : */
#define ALIGNMASK        (MAD_ALIGNMENT-1)
#define NOT_ALIGNED(X)   ((long)X & (MAD_ALIGNMENT - 1))

#define IS_SMALL(x)      ((x) <= sizeof(long))

#define MAX_BUFFER                 10

static char headers[MAX_BUFFER][MAX_HEADER] __MAD_ALIGNED__;

static struct s_buffer {
  size_t       s_vec_size;
  struct iovec s_vector[MAX_IOVECS];
  boolean dyn_allocated[MAX_IOVECS];
  char *s_header;
  size_t s_head_size;
  int dest_node;
} buffer_pool[MAX_BUFFER];

static struct s_buffer *s_buffer_cache[MAX_BUFFER];
static int last_buffer_free;

static struct iovec r_vector[MAX_IOVECS];
static size_t       r_vec_size;
static char        *r_header;
static size_t       r_head_size;

#ifdef PM2
static marcel_sem_t sem;
static marcel_mutex_t mutex;

/* semaphore used to block sends during slot negociations: */
_PRIVATE_ extern marcel_key_t _pm2_isomalloc_nego_key;
marcel_sem_t sem_nego;
#endif

static netinterf_t *cur_netinterf;

extern netinterf_t* NETINTERF_INIT();

char *mad_aligned_malloc(int size)
{
    char *ptr, *ini;

    ini = ptr = tmalloc(size + 2 * MAD_ALIGNMENT - 1);
    if(ptr != NULL && ((unsigned)ptr & ALIGNMASK) != 0) {
	ptr = (char *) (((unsigned)ptr + ALIGNMASK) & ~ALIGNMASK);
    }

    *(char **)ptr = ini;
    ptr += MAD_ALIGNMENT;

    return ptr;
}

void mad_aligned_free(void *ptr)
{
  tfree(*(char **)((char *)ptr - MAD_ALIGNMENT));
}


void mad_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
#if !defined(PM2)
  timing_init();
#endif

#if !defined(PM2) && defined(USE_SAFE_MALLOC)
  safe_malloc_init();
#endif

  cur_netinterf = NETINTERF_INIT();

  (*cur_netinterf->network_init)(argc, argv, nb_proc, tids, nb, whoami);
}

#ifdef MAD_VIA
extern void mad_via_register_send_buffers(char *area, unsigned len);
#endif

void mad_buffers_init(void)
{
  int i;

#ifdef PM2
  marcel_sem_init(&sem_nego, 1);
  marcel_sem_init(&sem, MAX_BUFFER);
  marcel_mutex_init(&mutex, NULL);
  marcel_key_create(&buffer_key,NULL);
#endif

  for(i=0; i<MAX_BUFFER; i++) {
    buffer_pool[i].s_header = (char *)headers[i];
    s_buffer_cache[i]=&buffer_pool[i];
  }
  last_buffer_free=MAX_BUFFER-1;

#ifdef MAD_VIA
  mad_via_register_send_buffers((char *)headers, MAX_HEADER * MAX_BUFFER);
#endif
}

void mad_exit(void)
{
  (*cur_netinterf->network_exit)();
}

char *mad_arch_name(void)
{
  return NET_ARCH;
}

boolean mad_can_send_to_self(void)
{
#ifdef CANT_SEND_TO_SELF
  return FALSE;
#else
  return TRUE;
#endif
}

void mad_sendbuf_init(int dest_node)
{
  register struct s_buffer *cur_buf;

#ifdef PM2
  marcel_sem_P(&sem);
  marcel_mutex_lock(&mutex);
  marcel_setspecific(buffer_key,s_buffer_cache[last_buffer_free--]);
  marcel_mutex_unlock(&mutex);
#endif

  cur_buf=CUR_BUF;
  cur_buf->s_vec_size = 1;
  cur_buf->s_head_size = ALIGN(PIGGYBACK_AREA_LEN * sizeof(long));
  cur_buf->dyn_allocated[0] = FALSE;
  cur_buf->dest_node = dest_node;
}

void mad_sendbuf_send(void)
{
  register struct s_buffer *cur_buf=CUR_BUF;
  int i;

#if defined(PM2)
  if(marcel_getspecific(_pm2_isomalloc_nego_key) != (any_t)-1)
    marcel_sem_P(&sem_nego);
#endif

  cur_buf->s_vector[0].iov_base = cur_buf->s_header;
  cur_buf->s_vector[0].iov_len = cur_buf->s_head_size;

#ifdef DEBUG
  fprintf(stderr, "mad_sendbuf_send:\n");
  for(i=0; i<cur_buf->s_vec_size; i++)
    fprintf(stderr, "\tvec[%d].iov_len = %ld\n",
	    i, cur_buf->s_vector[i].iov_len);
#endif

  (*cur_netinterf->network_send)(cur_buf->dest_node,
				 cur_buf->s_vector,
				 cur_buf->s_vec_size);

#if defined(PM2)
  if(marcel_getspecific(_pm2_isomalloc_nego_key) != (any_t) -1)
    marcel_sem_V(&sem_nego);
#endif

  /* We must free all dynamically allocated memory */
  for(i=0; i< cur_buf->s_vec_size; i++) {
    if(cur_buf->dyn_allocated[i]) {
      tfree(cur_buf->s_vector[i].iov_base);
      cur_buf->dyn_allocated[i] = FALSE;
    }
  }

#ifdef PM2
  marcel_mutex_lock(&mutex);
  s_buffer_cache[++last_buffer_free]=cur_buf;
  marcel_mutex_unlock(&mutex);

  marcel_sem_V(&sem);
#endif
}

void mad_receive(void)
{
  (*cur_netinterf->network_receive)(&r_header);
  r_head_size = ALIGN(PIGGYBACK_AREA_LEN * sizeof(long));
  r_vec_size = 0;
}

void mad_recvbuf_receive(void)
{
  (*cur_netinterf->network_receive_data)(r_vector, r_vec_size);
#ifdef DEBUG
  {
    int i;

    fprintf(stderr, "mad_recvbuf_receive (body):\n");
    for(i=0; i<r_vec_size; i++)
      fprintf(stderr, "\tvec[%d].iov_len = %ld\n",
	      i, r_vector[i].iov_len);
  }
#endif
}

void pm2_pack_byte(mad_send_mode_t sm,
		   mad_receive_mode_t rm,
		   char *data,
		   size_t nb)
{
  switch(sm) {
  case SEND_CHEAPER : {
    switch(rm) {
    case RECV_EXPRESS : {
      old_mad_pack_byte(MAD_IN_HEADER, data, nb);
      break;
    }
    case RECV_CHEAPER : {
      old_mad_pack_byte(MAD_IN_PLACE, data, nb);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  case SEND_SAFER : {
    old_mad_pack_byte(MAD_IN_HEADER, data, nb);
    break;
  }
  case SEND_LATER : {
    switch(rm) {
    case RECV_CHEAPER : {
      old_mad_pack_byte(MAD_IN_PLACE, data, nb);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  default:
    fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
    exit(1);
  }
}

void pm2_unpack_byte(mad_send_mode_t sm,
		     mad_receive_mode_t rm,
		     char *data,
		     size_t nb)
{
  switch(sm) {
  case SEND_CHEAPER : {
    switch(rm) {
    case RECV_EXPRESS : {
      old_mad_unpack_byte(MAD_IN_HEADER, data, nb);
      break;
    }
    case RECV_CHEAPER : {
      old_mad_unpack_byte(MAD_IN_PLACE, data, nb);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  case SEND_SAFER : {
    old_mad_unpack_byte(MAD_IN_HEADER, data, nb);
    break;
  }
  case SEND_LATER : {
    switch(rm) {
    case RECV_CHEAPER : {
      old_mad_unpack_byte(MAD_IN_PLACE, data, nb);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  default:
    fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
    exit(1);
  }
}

void pm2_pack_str(mad_send_mode_t sm,
		  mad_receive_mode_t rm,
		  char *data)
{
  switch(sm) {
  case SEND_CHEAPER : {
    switch(rm) {
    case RECV_EXPRESS : {
      old_mad_pack_str(MAD_IN_HEADER, data);
      break;
    }
    case RECV_CHEAPER : {
      old_mad_pack_str(MAD_IN_PLACE, data);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  case SEND_SAFER : {
    old_mad_pack_str(MAD_IN_HEADER, data);
    break;
  }
  case SEND_LATER : {
    switch(rm) {
    case RECV_CHEAPER : {
      old_mad_pack_str(MAD_IN_PLACE, data);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  default:
    fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
    exit(1);
  }
}

void pm2_unpack_str(mad_send_mode_t sm,
		    mad_receive_mode_t rm,
		    char *data)
{
  switch(sm) {
  case SEND_CHEAPER : {
    switch(rm) {
    case RECV_EXPRESS : {
      old_mad_unpack_str(MAD_IN_HEADER, data);
      break;
    }
    case RECV_CHEAPER : {
      old_mad_unpack_str(MAD_IN_PLACE, data);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  case SEND_SAFER : {
    old_mad_unpack_str(MAD_IN_HEADER, data);
    break;
  }
  case SEND_LATER : {
    switch(rm) {
    case RECV_CHEAPER : {
      old_mad_unpack_str(MAD_IN_PLACE, data);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  default:
    fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
    exit(1);
  }
}

void old_mad_pack_byte(madeleine_part where, char *data, size_t nb)
{
  register struct s_buffer *cur_buf=CUR_BUF;

  if(nb) {
    switch(where) {
       case MAD_IN_HEADER : {

#ifndef NOCHECK
	 if(MAX_HEADER - cur_buf->s_head_size < nb)
	   RAISE(CONSTRAINT_ERROR);
#endif

	 memcpy(&cur_buf->s_header[cur_buf->s_head_size], data, nb);
	 cur_buf->s_head_size = ALIGN(cur_buf->s_head_size + nb);

	 break;
       }
       case MAD_IN_PLACE : 
       case MAD_IN_PLACE_N_FREE : {

	 struct iovec *vec = &cur_buf->s_vector[cur_buf->s_vec_size];

#ifndef NOCHECK
	 if(cur_buf->s_vec_size == MAX_IOVECS)
	   RAISE(CONSTRAINT_ERROR);

	 if(NOT_ALIGNED(data))
	   RAISE(ALIGN_ERROR);
#endif
	 vec->iov_base = data;
	 vec->iov_len  = nb;
	 cur_buf->dyn_allocated[cur_buf->s_vec_size++] = 
	   (where == MAD_IN_PLACE ? FALSE : TRUE);

	 break;
       }
       case MAD_BY_COPY : {

	 if(IS_SMALL(nb) && (nb < MAX_HEADER - cur_buf->s_head_size)) {

	   memcpy(&cur_buf->s_header[cur_buf->s_head_size], data, nb);
	   cur_buf->s_head_size = ALIGN(cur_buf->s_head_size + nb);

	 } else {
#ifndef NOCHECK
	   if(cur_buf->s_vec_size == MAX_IOVECS)
	     RAISE(CONSTRAINT_ERROR);
#endif
	   memcpy((cur_buf->s_vector[cur_buf->s_vec_size].iov_base = tmalloc(nb)), data, nb);
	   cur_buf->s_vector[cur_buf->s_vec_size].iov_len = nb;
	   cur_buf->dyn_allocated[cur_buf->s_vec_size] = TRUE;
	   cur_buf->s_vec_size++;
	 }
	 break;
       }
    }
  }
}

void old_mad_unpack_byte(madeleine_part where, char *data, size_t nb)
{
  if(nb) {
    switch(where) {
       case MAD_IN_HEADER : {

	 memcpy(data, &r_header[r_head_size], nb);
	 r_head_size = ALIGN(r_head_size + nb);

	 break;
       }
       case MAD_IN_PLACE :
       case MAD_IN_PLACE_N_FREE : {
#ifndef NOCHECK
	 if(NOT_ALIGNED(data))
	   RAISE(ALIGN_ERROR);
#endif
	 r_vector[r_vec_size].iov_base = data;
	 r_vector[r_vec_size++].iov_len = nb;

	 break;
       }
       case MAD_BY_COPY : {

	 if(IS_SMALL(nb) && (nb < MAX_HEADER - r_head_size)) {

 	   memcpy(data, &r_header[r_head_size], nb);
	   r_head_size = ALIGN(r_head_size + nb);

	 } else {
#ifndef NOCHECK
	   if(NOT_ALIGNED(data))
	     RAISE(ALIGN_ERROR);
#endif
	   r_vector[r_vec_size].iov_base = data;
	   r_vector[r_vec_size].iov_len = nb;
	   r_vec_size++;
	 }
	 break;
       }
    }
  }
}

void old_mad_pack_short(madeleine_part where, short *data, size_t nb)
{
  old_mad_pack_byte(where, (char *)data, nb*sizeof(short));
}

void old_mad_unpack_short(madeleine_part where, short *data, size_t nb)
{
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(short));
}

void old_mad_pack_int(madeleine_part where, int *data, size_t nb)
{
  old_mad_pack_byte(where, (char *)data, nb*sizeof(int));
}

void old_mad_unpack_int(madeleine_part where, int *data, size_t nb)
{
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(int));
}

void old_mad_pack_long(madeleine_part where, long *data, size_t nb)
{
  old_mad_pack_byte(where, (char *)data, nb*sizeof(long));
}

void old_mad_unpack_long(madeleine_part where, long *data, size_t nb)
{
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(long));
}

void old_mad_pack_float(madeleine_part where, float *data, size_t nb)
{
  old_mad_pack_byte(where, (char *)data, nb*sizeof(float));
}

void old_mad_unpack_float(madeleine_part where, float *data, size_t nb)
{
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(float));
}

void old_mad_pack_double(madeleine_part where, double *data, size_t nb)
{
  old_mad_pack_byte(where, (char *)data, nb*sizeof(double));
}

void old_mad_unpack_double(madeleine_part where, double *data, size_t nb)
{
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(double));
}

void old_mad_pack_pointer(madeleine_part where, pointer *data, size_t nb)
{
  old_mad_pack_byte(where, (char *)data, nb*sizeof(pointer));
}

void old_mad_unpack_pointer(madeleine_part where, pointer *data, size_t nb)
{
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(pointer));
}

void old_mad_pack_str(madeleine_part where, char *data)
{
  int nb;
  register struct s_buffer *cur_buf=CUR_BUF;

  nb = strlen(data)+1;
  switch(where) {
  case MAD_IN_HEADER : {

#ifndef NOCHECK
    if(MAX_HEADER - cur_buf->s_head_size < nb)
      RAISE(CONSTRAINT_ERROR);
#endif

    memcpy(&cur_buf->s_header[cur_buf->s_head_size], data, nb);
    cur_buf->s_head_size = ALIGN(cur_buf->s_head_size + nb);

    break;
  }
  case MAD_IN_PLACE :
  case MAD_IN_PLACE_N_FREE : {

#ifndef NOCHECK
    if(cur_buf->s_vec_size == MAX_IOVECS)
      RAISE(CONSTRAINT_ERROR);

    if(NOT_ALIGNED(data))
      RAISE(ALIGN_ERROR);
#endif

    *((int *)&cur_buf->s_header[cur_buf->s_head_size]) = nb;
    cur_buf->s_head_size = ALIGN(cur_buf->s_head_size + sizeof(int));
    cur_buf->s_vector[cur_buf->s_vec_size].iov_base = data;
    cur_buf->s_vector[cur_buf->s_vec_size].iov_len = nb;
    cur_buf->dyn_allocated[cur_buf->s_vec_size] = (where == MAD_IN_PLACE ? FALSE : TRUE);
    cur_buf->s_vec_size++;
    break;
  }
  case MAD_BY_COPY : {

#ifndef NOCHECK
    if(cur_buf->s_vec_size == MAX_IOVECS)
      RAISE(CONSTRAINT_ERROR);
#endif

    *((int *)&cur_buf->s_header[cur_buf->s_head_size]) = nb;
    cur_buf->s_head_size = ALIGN(cur_buf->s_head_size + sizeof(int));
    memcpy((cur_buf->s_vector[cur_buf->s_vec_size].iov_base = tmalloc(nb)), data, nb);
    cur_buf->s_vector[cur_buf->s_vec_size].iov_len = nb;
    cur_buf->dyn_allocated[cur_buf->s_vec_size] = TRUE;
    cur_buf->s_vec_size++;
    break;
  }
  }
}

void old_mad_unpack_str(madeleine_part where, char *data)
{ int nb;

  switch(where) {
    case MAD_IN_HEADER : {
      nb = strlen(&r_header[r_head_size])+1;
      memcpy(data, &r_header[r_head_size], nb);
      r_head_size = ALIGN(r_head_size + nb);
      break;
    }
    case MAD_IN_PLACE :
    case MAD_IN_PLACE_N_FREE :
    case MAD_BY_COPY : {
#ifndef NOCHECK
      if(NOT_ALIGNED(data))
	RAISE(ALIGN_ERROR);
#endif
      nb = *((int *)&r_header[r_head_size]);
      r_head_size = ALIGN(r_head_size + sizeof(int));
      r_vector[r_vec_size].iov_base = data;
      r_vector[r_vec_size].iov_len = nb;
      r_vec_size++;
      break;
    }
  }
}

