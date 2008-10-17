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


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "helper.h"

//#define ANY_SRC 1

static nm_gate_t r_gate_id = NM_ANY_GATE;

#define SIZE  (64 * 1024)

static void short_unexpected(void){
  char *msg	= "helloworld";
  char * buf = NULL;

  if(is_server){
    CCS_datadesc_t my_type;
    struct CCSI_Segment seg;
    int count = 1;
    int blocklengths[2] = {5, 5};
    int strides[2] = {0, 6};

    nm_sr_request_t request;
    nm_sr_request_t request0;
    /* server
     */

    buf = malloc(count * strlen(msg) + 1);
    char *buf2 = malloc(strlen(msg) + 1);

    CCS_datadesc_create_indexed (2, blocklengths, strides, CCS_DATA8, &my_type);
    CCSI_Segment_init (buf, count, my_type, &seg, 0);

    nm_sr_irecv(p_core, r_gate_id, 1, buf2, strlen(msg), &request0);

    int i = 0;
    while(i++ < 10000)
      nm_sr_rtest(p_core, &request0);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

    nm_sr_rwait(p_core, &request0);

  } else {
    uint32_t len = strlen(msg);

    nm_sr_request_t request;
    /* client
     */

    buf = malloc(strlen(msg));
    strcpy(buf, msg);

    nm_sr_isend(p_core, gate_id, 0, buf, len, &request);
    nm_sr_swait(p_core, &request);

    sleep(10);
    nm_sr_isend(p_core, gate_id, 1, buf, len, &request);
    nm_sr_swait(p_core, &request);
  }

  if(is_server){
    int i, j;

    for(i = 0; i < 5; i++){
      if(buf[i] != msg[i]){
        printf("[%d] - buf[%d] = %c et msg[%d] = %c\n", i, i, buf[i], i, msg[i]);
        TBX_FAILURE("Echec sur short_unexpected");
      }
    }

    for(j = i+1; j < strlen(msg+1); j++, i++){
      if(buf[j] != msg[i]){
        printf("[%d] - buf[%d] = %c et msg[%d] = %c\n", j, j, buf[j], i, msg[i]);
        TBX_FAILURE("Echec sur short_unexpected");
      }
    }
    printf("*************** short_unexpected OK!!!\n");
  }
  free(buf);
}


static void large_unexpected(void){
  char *msg	= "helloworld";
  char *buf1 = NULL, *buf2 = NULL;

  if(is_server){
    CCS_datadesc_t my_type;
    struct CCSI_Segment seg;
    int count = 1;
    int blocklengths[2] = {SIZE/2 - 10, SIZE/2};
    int strides[2] = {0, SIZE/2};

    nm_sr_request_t request;
    nm_sr_request_t request0;
    /* server
     */

    buf1 = malloc(strlen(msg));
    memset(buf1, ' ', strlen(msg));

    buf2 = malloc(SIZE);
    memset(buf2, ' ', SIZE);

    CCS_datadesc_create_indexed (2, blocklengths, strides, CCS_DATA8, &my_type);
    CCSI_Segment_init (buf2, count, my_type, &seg, 0);

    nm_sr_irecv(p_core, r_gate_id, 1, buf1, strlen(msg), &request0);

    int i = 0;
    while(i++ < 10000)
      nm_sr_rtest(p_core, &request0);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

    nm_sr_rwait(p_core, &request0);

  } else {
    nm_sr_request_t request;
    int i;
    /* client
     */

    buf1 = malloc(strlen(msg));
    strcpy(buf1, msg);

    buf2 = malloc(SIZE-10);
    for(i = 0; i<SIZE-10; i++){
      buf2[i] = 'a' + i%26;
    }

    nm_sr_isend(p_core, gate_id, 0, buf2, SIZE-10, &request);
    nm_sr_swait(p_core, &request);

    sleep(10);
    nm_sr_isend(p_core, gate_id, 1, buf1, strlen(msg), &request);
    nm_sr_swait(p_core, &request);
  }

  if(is_server){
    char cur_char = 'a';
    int ajout = 0;
    int i, j;

    for(i = 0; i < SIZE/2-10; i++){
      if(buf2[i] != cur_char){
        printf("[%d] - buf2[%d] = %c et cur_char = %c\n", i, i, buf2[i], cur_char);
        TBX_FAILURE("Echec sur short_unexpected");
      }
      ajout++;
      cur_char = 'a' + ajout%26;
    }


    for(; i<SIZE/2; i++){
      if(buf2[i] != ' '){
        printf("[%d] - buf2[%d] = %c et cur_char = %c\n", i, i, buf2[i], cur_char);
        TBX_FAILURE("Echec sur short_unexpected");
      }
    }


    for( ; i < SIZE; j++, i++){
      if(buf2[i] != cur_char){
        printf("[%d] - buf2[%d] = %c et cur_char = %c\n", i, i, buf2[i], cur_char);
        TBX_FAILURE("Echec sur large_unexpected");
      }
      ajout++;
      cur_char = 'a' + ajout%26;
    }

    printf("*************** large_unexpected OK!!!\n");
  }
  free(buf1);
  free(buf2);
}

int
main(int	  argc,
     char	**argv) {
  init(&argc, argv);

  r_gate_id = gate_id;
  short_unexpected();
  large_unexpected();

  r_gate_id = NM_ANY_GATE;
  short_unexpected();
  large_unexpected();
  
  nmad_exit();
  exit(0);
}
