/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <pm2_common.h>

#include "ping_optimized.h"
#include "vector_optimized.h"

static
int get_vector_size(int size,
                  int blocks) {
  int vector_size = size;
  while (vector_size % blocks != 0) vector_size ++;
  return vector_size;
}

static
float get_value(int x,
                int y,
                int size) {
  return (y+1)+(size*x);
}


void pingpong_datatype_vector(nm_so_pack_interface interface,
                              nm_gate_id_t         gate_id,
                              int                  number_of_elements,
                              int                  number_of_blocks,
                              int                  client) {
  struct MPIR_DATATYPE *datatype_vector = NULL;
  float                *buffer;
  float                *r_buffer = NULL;
  int                   i, j, k;
  int                   vector_size;

  /*
    Initialise data
  */
  vector_size = get_vector_size(sqrt(number_of_elements), number_of_blocks);
  buffer = malloc(vector_size * vector_size * sizeof(float*));
  for(i=0 ; i<vector_size ; i++) {
    for(j=0 ; j<vector_size ; j++) {
      buffer[i*vector_size+j] = get_value(i, j, vector_size);
      DEBUG_PRINTF("%3.1f ", buffer[i*vector_size+j]);
    }
    DEBUG_PRINTF("\n");
  }

  datatype_vector = malloc(sizeof(struct MPIR_DATATYPE));
  init_datatype_vector(datatype_vector, number_of_elements, number_of_blocks);
  r_buffer = malloc(number_of_elements * sizeof(float));

  if (client) {
    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
    for(k = 0 ; k<LOOPS ; k++) {
      pack_datatype_vector(interface, gate_id, datatype_vector, buffer);
      unpack_datatype_vector(interface, gate_id, r_buffer);
    }
    TBX_GET_TICK(t2);

    printf("%s\t%s\t%d\t%d\t%lf\n", "vector", STRATEGY, number_of_elements, number_of_blocks, TBX_TIMING_DELAY(t1, t2) / (2 * LOOPS));
    PRINTF("Received value: ");
    for(i=0 ; i<number_of_elements ; i++) PRINTF("%3.2f ", r_buffer[i]);
    PRINTF("\n");
  }
  else { /* server */
    for(k = 0 ; k<LOOPS ; k++) {
      unpack_datatype_vector(interface, gate_id, r_buffer);
      pack_datatype_vector(interface, gate_id, datatype_vector, buffer);
    }
  }

  free(buffer);
  free(r_buffer);
  free(datatype_vector);
}

void init_datatype_vector(struct MPIR_DATATYPE *datatype,
                          int                   number_of_elements,
                          int                   number_of_blocks) {
  int vector_size;
  vector_size = get_vector_size(sqrt(number_of_elements), number_of_blocks);

  DEBUG_PRINTF("Creating datatype vector with %d elements and %d blocks with %d vector size\n", number_of_elements, number_of_blocks, vector_size);
  datatype->dte_type = MPIR_VECTOR;
  datatype->count = number_of_blocks;
  datatype->blocklen = number_of_elements / datatype->count;
  datatype->stride = datatype->blocklen + ((vector_size * vector_size - number_of_elements) / number_of_blocks);

  datatype->size =  datatype->blocklen * datatype->count * sizeof(float);
}

void pack_datatype_vector(nm_so_pack_interface  interface,
                          nm_gate_id_t          gate_id,
                          struct MPIR_DATATYPE *datatype,
                          float                *s_ptr) {
  struct nm_so_cnx cnx;
  int              size, numberOfElements, i, j;
  float           *tmp_buf;

  DEBUG_PRINTF("Sending (h)vector type: stride %d - blocklen %d - count %d - size %d\n", datatype->stride, datatype->blocklen,
        datatype->count, datatype->size);
  numberOfElements = datatype->blocklen * datatype->count;
  size = datatype->size / numberOfElements;

  /*  Pack the needed information to unpack on the other side (number of blocks, number of elements, size of each element) */
  nm_so_begin_packing(interface, gate_id, 0, &cnx);
  nm_so_pack(&cnx, &datatype->count, sizeof(int));
  nm_so_pack(&cnx, &size, sizeof(int));
  nm_so_pack(&cnx, &datatype->blocklen, sizeof(int));
#if defined(NO_RWAIT)
  nm_so_end_packing(&cnx);
#endif /* NO_RWAIT */

  /*  for each block pack the elements of this block */
#if defined(NO_RWAIT)
  nm_so_begin_packing(interface, gate_id, 0, &cnx);
#endif /* NO_RWAIT */
  tmp_buf = s_ptr;
  for(i=0 ; i<datatype->count ; i++) {
    DEBUG_PRINTF("Packing block %d at address %p\n", i, tmp_buf);
    DEBUG_PRINTF("Values: ");
    for(j=0 ; j<datatype->blocklen ; j++) DEBUG_PRINTF("%3.2f ", tmp_buf[j]);
    DEBUG_PRINTF("\n");
    nm_so_pack(&cnx, tmp_buf, datatype->blocklen*size);
    tmp_buf += datatype->stride;
  }
  nm_so_end_packing(&cnx);
}

void unpack_datatype_vector(nm_so_pack_interface interface,
                            nm_gate_id_t         gate_id,
                            float               *r_ptr) {
  struct nm_so_cnx cnx;
  int              numberOfBlocks, size, blockLength, i;
  float          **tmp_buf;

  DEBUG_PRINTF("Receiving (h)vector type\n");

  /*  Unpack the following informations: numberOfBlocks, size of each element, block length */
  nm_so_begin_unpacking(interface, gate_id, 0, &cnx);
  nm_so_unpack(&cnx, &numberOfBlocks, sizeof(int));
  nm_so_unpack(&cnx, &size, sizeof(int));
  nm_so_unpack(&cnx, &blockLength, sizeof(int));
#if defined(NO_RWAIT)
  nm_so_end_unpacking(&cnx);
#else
  nm_so_flush_unpacks(&cnx);
#endif /* NO_RWAIT */
  DEBUG_PRINTF("Number of blocks %d Size %d Block length %d\n", numberOfBlocks, size, blockLength);

  /*  unpack the elements for each block */
  tmp_buf = malloc((numberOfBlocks+1) * sizeof(float *));
  tmp_buf[0] = r_ptr;
#if defined(NO_RWAIT)
  nm_so_begin_unpacking(interface, gate_id, 0, &cnx);
#endif /* NO_RWAIT */
  for(i=0 ; i<numberOfBlocks ; i++) {
    DEBUG_PRINTF("Going to unpack block %d at address %p\n", i, tmp_buf[i]);
    nm_so_unpack(&cnx, tmp_buf[i], blockLength*size);
    tmp_buf[i+1] = tmp_buf[i] + blockLength;
  }
  nm_so_end_unpacking(&cnx);
  free(tmp_buf);
}
