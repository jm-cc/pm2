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

#include "ping_optimized.h"
#include "struct_optimized.h"

void pingpong_datatype_struct(nm_so_pack_interface interface,
                              gate_id_t            gate_id,
                              int                  number_of_elements,
                              int                  client) {
  struct MPIR_DATATYPE *datatype_struct = NULL;
  particle_t           *buffer = NULL;
  particle_t           *r_buffer = NULL;
  int                  i, k;

  // Initialise data to send
  buffer = malloc(number_of_elements * sizeof(particle_t));
  for (i=0; i < number_of_elements; i++) {
    buffer[i].x = (i+1) * 2.0;
    buffer[i].y = (i+1) * -2.0;
    buffer[i].c = (i+1) * 4;
    buffer[i].z = (i+1) * 4.0;
    PRINTF("%3.2f %3.2f %3d %3.2f\n", buffer[i].x, buffer[i].y, buffer[i].c, buffer[i].z);
  }

  datatype_struct = malloc(sizeof(struct MPIR_DATATYPE));
  init_datatype_struct(datatype_struct, number_of_elements);
  r_buffer = malloc(number_of_elements * sizeof(particle_t));

  if (client) {
    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
    for(k = 0 ; k<LOOPS ; k++) {
      pack_datatype_struct(interface, gate_id, datatype_struct, buffer);
      unpack_datatype_struct(interface, gate_id, r_buffer);
    }
    TBX_GET_TICK(t2);

    printf("%s\t%s\t%d\t%d\t%lf\n", "struct", STRATEGY, number_of_elements, 3, TBX_TIMING_DELAY(t1, t2) / (2 * LOOPS));
    PRINTF("Received values: ");
    for(i=0 ; i<number_of_elements ; i++) PRINTF("%3.2f %3.2f %3d %3.2f\n", r_buffer[i].x, r_buffer[i].y, r_buffer[i].c, r_buffer[i].z);
  }
  else { /* server */
    for(k = 0 ; k<LOOPS ; k++) {
      unpack_datatype_struct(interface, gate_id, r_buffer);
      pack_datatype_struct(interface, gate_id, datatype_struct, buffer);
    }
  }

  free(buffer);
  free(r_buffer);
  free(datatype_struct);
}

void init_datatype_struct(struct MPIR_DATATYPE *datatype,
                          int                   number_of_elements) {
  int i;

  datatype->dte_type = MPIR_STRUCT;
  datatype->nb_elements = number_of_elements;
  datatype->elements = 4;

  datatype->blocklens = malloc(3 * sizeof(int));
  datatype->blocklens[0] = 2;
  datatype->blocklens[1] = 1;
  datatype->blocklens[2] = 1;

  datatype->old_types = malloc(3 * sizeof(struct MPIR_DATATYPE *));
  for(i=0 ; i<3 ; i++) datatype->old_types[i] = malloc(sizeof(struct MPIR_DATATYPE));
  datatype->old_types[0]->size = sizeof(float);
  datatype->old_types[1]->size = sizeof(int);
  datatype->old_types[2]->size = sizeof(float);

  datatype->indices = malloc(3 * sizeof(int));
  datatype->indices[0] = 0;
  for(i=1 ; i<3 ; i++) datatype->indices[i] = datatype->indices[i-1] + datatype->blocklens[i-1] * datatype->old_types[i-1]->size;
}

void pack_datatype_struct(nm_so_pack_interface  interface,
                          gate_id_t             gate_id,
                          struct MPIR_DATATYPE *datatype,
                          particle_t           *s_ptr) {
  struct nm_so_cnx cnx;
  int              numberOfBlocks, numberOfElements, i, j, size, pack=0;
  void           **tmp_buf;

  DEBUG_PRINTF("Sending struct datatype, count=%d indices[0]=%d blocklen[0]=%d oldtype[0].size=%d elements=%d at address %p\n",
        datatype->nb_elements, datatype->indices[0], datatype->blocklens[0], datatype->old_types[0]->size, datatype->elements, s_ptr);

  numberOfBlocks = 1;
  numberOfElements = datatype->blocklens[0];
  while(numberOfElements != datatype->elements) {
    numberOfBlocks ++;
    numberOfElements += datatype->blocklens[numberOfBlocks-1];
    DEBUG_PRINTF("Adding %d elements for %d blocks\n", datatype->blocklens[numberOfBlocks-1], numberOfBlocks);
  }
  DEBUG_PRINTF("Number of blocks %d\n", numberOfBlocks);

  /*  Pack the needed information to unpack on the other side (number of elements, number of blocks in an element) */
  nm_so_begin_packing(interface, gate_id, 0, &cnx);
  nm_so_pack(&cnx, &datatype->nb_elements, sizeof(int));
  nm_so_pack(&cnx, &numberOfBlocks, sizeof(int));
  nm_so_end_packing(&cnx);

  /*  for each block, pack information about the block data (number of data, offset for the block, size of an individual data) */
  nm_so_begin_packing(interface, gate_id, 0, &cnx);
  nm_so_pack(&cnx, datatype->indices, numberOfBlocks*sizeof(int));
  nm_so_pack(&cnx, datatype->blocklens, numberOfBlocks*sizeof(int));
  nm_so_end_packing(&cnx);

  nm_so_begin_packing(interface, gate_id, 0, &cnx);
  for(i=0 ; i<numberOfBlocks ; i++) {
    size = datatype->old_types[i]->size;
    DEBUG_PRINTF("Send data for block %d : %d elements from indice %d with size %d\n", i, datatype->blocklens[i], datatype->indices[i], size);
    nm_so_pack(&cnx, &size, sizeof(int));
  }
  nm_so_end_packing(&cnx);

  /*  for each element and each block in the element, pack the block data */
  tmp_buf = malloc((datatype->nb_elements * numberOfBlocks + 1) * sizeof(void *));
  nm_so_begin_packing(interface, gate_id, 0, &cnx);
  pack = 0;
  tmp_buf[pack] = s_ptr;
  for(j=0 ; j<datatype->nb_elements ; j++) {
    for(i=0 ; i<numberOfBlocks ; i++) {
      size = datatype->old_types[i]->size;
      DEBUG_PRINTF("Sending element %d Block %d Size %d at address %p\n", j, i, datatype->blocklens[i]*size, tmp_buf[pack]);
      nm_so_pack(&cnx, tmp_buf[pack], datatype->blocklens[i]*size);
      pack++;
      if (pack % 256 == 0) {
        DEBUG_PRINTF("closing connection element %d Block %d Size %d at address %p\n", j, i, datatype->blocklens[i]*size, tmp_buf[pack-1]);
        nm_so_end_packing(&cnx);
        nm_so_begin_packing(interface, gate_id, 0, &cnx);
      }
      tmp_buf[pack] = tmp_buf[pack-1] + datatype->blocklens[i]*size;
    }
  }
  nm_so_end_packing(&cnx);
  free(tmp_buf);
}

void unpack_datatype_struct(nm_so_pack_interface interface,
                            gate_id_t            gate_id,
                            particle_t          *r_ptr) {
  struct nm_so_cnx cnx;
  int             *block_size, *block_elements, *block_offset, numberOfBlocks, count;
  int              i, j, unpack;
  void           **tmp_buf;

  DEBUG_PRINTF("Receiving struct type at address %p...\n", r_ptr);

  /*  Unpack the following information : count, number of blocks */
  nm_so_begin_unpacking(interface, gate_id, 0, &cnx);
  nm_so_unpack(&cnx, &count, sizeof(int));
  nm_so_unpack(&cnx, &numberOfBlocks, sizeof(int));
  nm_so_end_unpacking(&cnx);
  DEBUG_PRINTF("Count %d - Number of blocks %d\n", count, numberOfBlocks);

  /*  unpack the needed information for the blocks */
  block_size = (int *) malloc(numberOfBlocks * sizeof(int));
  block_elements = (int *) malloc(numberOfBlocks * sizeof(int));
  block_offset = (int *) malloc(numberOfBlocks * sizeof(int));
  nm_so_begin_unpacking(interface, gate_id, 0, &cnx);
  nm_so_unpack(&cnx, block_offset, numberOfBlocks*sizeof(int));
  nm_so_unpack(&cnx, block_elements, numberOfBlocks*sizeof(int));
  nm_so_end_unpacking(&cnx);

  nm_so_begin_unpacking(interface, gate_id, 0, &cnx);
  for(i=0 ; i<numberOfBlocks ; i++) {
    nm_so_unpack(&cnx, &block_size[i], sizeof(int));
  }
  nm_so_end_unpacking(&cnx);

  for(i=0 ; i<numberOfBlocks ; i++) {
    DEBUG_PRINTF("Received data for block %d : %d elements from indice %d with size %d\n", i, block_elements[i], block_offset[i], block_size[i]);
  }

  /*  for each block, unpack the elements in the block */
  tmp_buf = malloc((count*numberOfBlocks+1)*sizeof(void *));
  unpack = 0;
  tmp_buf[unpack] = r_ptr;
  nm_so_begin_unpacking(interface, gate_id, 0, &cnx);
  for(j=0 ; j<count ; j++) {
    for(i=0 ; i<numberOfBlocks ; i++) {
      DEBUG_PRINTF("Going to unpack element %d, block %d of size %d at address %p\n", j, i, block_elements[i]*block_size[i], tmp_buf[unpack]);
      nm_so_unpack(&cnx, tmp_buf[unpack], block_elements[i]*block_size[i]);
      unpack++;
      if (unpack % 256 == 0) {
        DEBUG_PRINTF("closing connection element %d, block %d of size %d at address %p\n", j, i, block_elements[i]*block_size[i], tmp_buf[unpack-1]);
        nm_so_end_unpacking(&cnx);
        nm_so_begin_unpacking(interface, gate_id, 0, &cnx);
      }
      tmp_buf[unpack] = tmp_buf[unpack-1] + block_elements[i]*block_size[i];
    }
  }
  DEBUG_PRINTF("End of unpacking ....\n");
  nm_so_end_unpacking(&cnx);
  DEBUG_PRINTF("After unpacking ....\n");

  free(tmp_buf);
  free(block_size);
  free(block_elements);
  free(block_offset);
}
