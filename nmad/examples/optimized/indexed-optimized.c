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

#include "ping-optimized.h"
#include "indexed-optimized.h"

void pingpong_datatype_indexed(struct nm_core        *p_core,
                               uint8_t                gate_id,
                               int                    number_of_elements,
                               int                    number_of_blocks,
                               int                    client) {
  struct MPIR_DATATYPE *datatype_indexed = NULL;
  float                *buffer = NULL;
  float                *r_buffer = NULL;
  int                   i, k;

  /*
    Initialise data
  */
  buffer = malloc(number_of_elements * 2 * sizeof(float));
  memset(buffer, 0, number_of_elements);
  for(i=0 ; i<number_of_elements*2 ; i++) buffer[i] = i;

  datatype_indexed = malloc(sizeof(struct MPIR_DATATYPE));
  init_datatype_indexed(datatype_indexed, number_of_elements, number_of_blocks);
  r_buffer = malloc(number_of_elements * sizeof(float));

  if (client) {
    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
    for(k = 0 ; k<LOOPS ; k++) {
      pack_datatype_indexed(p_core, gate_id, datatype_indexed, buffer);
      unpack_datatype_indexed(p_core, gate_id, r_buffer);
    }
    TBX_GET_TICK(t2);

    printf("%d\t%d\t%d\t%lf\n", MPIR_INDEXED, number_of_elements, number_of_blocks, TBX_TIMING_DELAY(t1, t2) / (2 * LOOPS));
    PRINTF("Received value: ");
    for(i=0 ; i<number_of_elements ; i++) PRINTF("%3.2f ", r_buffer[i]);
    PRINTF("\n");
  }
  else { /* server */
    for(k = 0 ; k<LOOPS ; k++) {
      unpack_datatype_indexed(p_core, gate_id, r_buffer);
      pack_datatype_indexed(p_core, gate_id, datatype_indexed, buffer);
    }
  }

  free(buffer);
  free(r_buffer);
  free(datatype_indexed);
}

void init_datatype_indexed(struct MPIR_DATATYPE *datatype,
                           int                   number_of_elements,
                           int                   number_of_blocks) {
  int i;
  datatype->dte_type = MPIR_INDEXED;
  datatype->count = number_of_blocks;
  datatype->elements = number_of_elements;

  datatype->blocklens = malloc(datatype->count * sizeof(int));
  if (datatype->elements == 0) {
    for(i=0 ; i<datatype->count ; i++) {
      datatype->blocklens[i] = 0;
    }
  }
  else {
    for(i=0 ; i<datatype->count ; i++) {
      datatype->blocklens[i] = datatype->elements / datatype->count;
    }
    datatype->blocklens[number_of_blocks-1] += datatype->elements % datatype->count;
  }

  datatype->indices = malloc(datatype->count * sizeof(int));
  datatype->indices[0] = 0;
  for(i=1 ; i<datatype->count ; i++) {
    datatype->indices[i] = datatype->blocklens[i-1] + datatype->indices[i-1] + 2;
  }

  datatype->size = datatype->elements * sizeof(float);
}

void pack_datatype_indexed(struct nm_core        *p_core,
                           uint8_t                gate_id,
                           struct MPIR_DATATYPE  *datatype,
                           float                  *s_ptr) {
  struct nm_so_cnx  *cnx = NULL;
  float             *tmp_buf;
  int                size, numberOfBlocks, numberOfElements, i, j;

  DEBUG("Sending (h)indexed datatype, indices[0]=%d blocklen[0]=%d size=%d elements=%d\n", datatype->indices[0], datatype->blocklens[0],
         datatype->size, datatype->elements);

  size = datatype->size / datatype->elements;
  numberOfBlocks = 1;
  numberOfElements = datatype->blocklens[0];
  while(numberOfElements != datatype->elements) {
    numberOfBlocks ++;
    numberOfElements += datatype->blocklens[numberOfBlocks-1];
  }
  DEBUG("Number of blocks %d\n", numberOfBlocks);

  /*  Pack the needed information to unpack on the other side (number of blocks, size of each element) */
  nm_so_begin_packing(p_core, gate_id, 0, &cnx);
  nm_so_pack(cnx, &numberOfBlocks, sizeof(int));
  nm_so_pack(cnx, &size, sizeof(int));
  nm_so_end_packing(p_core, cnx);

  /*  pack the number of elements in the blocks */
  nm_so_begin_packing(p_core, gate_id, 0, &cnx);
  nm_so_pack(cnx, datatype->blocklens, numberOfBlocks*sizeof(int));
  nm_so_end_packing(p_core, cnx);

  /*  pack the elements for each block */
  nm_so_begin_packing(p_core, gate_id, 0, &cnx);
  for(i=0 ; i<numberOfBlocks ; i++) {
    tmp_buf = s_ptr + datatype->indices[i];
    DEBUG("Packing block %d with %d elements of size %d at address %p\n", i, datatype->blocklens[i], size, tmp_buf);
    DEBUG("Values: ");
    for(j=0 ; j<datatype->blocklens[i] ; j++) DEBUG("%3.2f ", tmp_buf[j]);
    DEBUG("\n");
    nm_so_pack(cnx, tmp_buf, datatype->blocklens[i]*size);
  }
  nm_so_end_packing(p_core, cnx);
}

void unpack_datatype_indexed(struct nm_core  *p_core,
                             uint8_t          gate_id,
                             float           *r_ptr) {
  struct nm_so_cnx *cnx      = NULL;
  float           **tmp_buf;
  int               numberOfBlocks, size, *numberOfElements, i;

  DEBUG("Receiving (h)indexed datatype at address %p...\n", r_ptr);

  /*  Unpack the following information : number of blocks, size of each element */
  nm_so_begin_unpacking(p_core, gate_id, 0, &cnx);
  nm_so_unpack(cnx, &numberOfBlocks, sizeof(int));
  nm_so_unpack(cnx, &size, sizeof(int));
  nm_so_end_unpacking(p_core, cnx);
  DEBUG("Number of blocks %d Size %d\n", numberOfBlocks, size);

  /*  unpack the number of elements in the blocks */
  numberOfElements = malloc(numberOfBlocks * sizeof(int));
  nm_so_begin_unpacking(p_core, gate_id, 0, &cnx);
  nm_so_unpack(cnx, numberOfElements, numberOfBlocks*sizeof(int));
  nm_so_end_unpacking(p_core, cnx);

  /*  unpack the elements for each block */
  tmp_buf = malloc((numberOfBlocks+1) * sizeof(float *));
  tmp_buf[0] = r_ptr;
  nm_so_begin_unpacking(p_core, gate_id, 0, &cnx);
  for(i=0 ; i<numberOfBlocks ; i++) {
    DEBUG("Going to unpack block %d with %d elements of size %d at address %p\n", i, numberOfElements[i], size, tmp_buf[i]);
    nm_so_unpack(cnx, tmp_buf[i], numberOfElements[i]*size);
    tmp_buf[i+1] = tmp_buf[i] + numberOfElements[i];
  }
  nm_so_end_unpacking(p_core, cnx);
  free(tmp_buf);

  free(numberOfElements);
}
