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
#include <sys/uio.h>

#include "helper.h"

#define ANY_SRC 1

static nm_gate_t r_gate_id = NM_ANY_GATE;

const char *msg	 = "h ell o!";
const char *msg2 = "hello!";

#define SIZE  (64 * 1024)
const char *msg_beg	= "hello", *msg_end = "world!";


static void index_vers_contig(void){
  CCS_datadesc_t my_type;
  struct CCSI_Segment seg;

  int count = 1;
  int blocklengths[3] = {1, 3, 2};
  int strides[3] = {0, 2, 6};

  char *buf = malloc(count * strlen(msg) + 1);

  CCS_datadesc_create_indexed (3, blocklengths, strides, CCS_DATA8, &my_type);

  CCSI_Segment_init (buf, count, my_type, &seg, 0);

  if (is_server) {
    nm_sr_request_t request;
    /* server
     */
    memset(buf, ' ', count * strlen(msg) + 1);

    nm_sr_irecv(p_core, r_gate_id, 0, buf, strlen(msg2), &request);
    nm_sr_rwait(p_core, &request);

    buf[strlen(msg2)] = '\0';

  } else {
    nm_sr_request_t request;
    /* client
     */
    strcpy(buf, msg);

    nm_sr_isend_datatype(p_core, gate_id, 0, &seg, &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    if(strcmp(buf, msg2) == 0){
      printf("**************************************\n");
      printf("********* index_vers_contig OK *******\n");
      printf("**************************************\n");
      //printf("buffer contents: %s\n", buf);
    } else {
      printf("buf = [%s] - msg2 = [%s]\n", buf, msg2);

      TBX_FAILURE("Echec sur index_vers_contig\n");
    }
  }
  free(buf);
}


static void index_vers_index(void){
  CCS_datadesc_t my_type;
  struct CCSI_Segment seg;

  int count = 1;
  int blocklengths[3] = {1, 3, 2};
  int strides[3] = {0, 2, 6};

  char *buf = malloc(count * strlen(msg) + 1);

  CCS_datadesc_create_indexed (3, blocklengths, strides, CCS_DATA8, &my_type);

  CCSI_Segment_init (buf, count, my_type, &seg, 0);

  if (is_server) {
    nm_sr_request_t request;
    /* server
     */
    memset(buf, ' ', count * strlen(msg) + 1);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

    buf[strlen(msg)] = '\0';

  } else {
    nm_sr_request_t request;
    /* client
     */
    strcpy(buf, msg);

    nm_sr_isend_datatype(p_core, gate_id, 0, &seg, &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    if(strcmp(buf, msg) == 0){
      printf("**************************************\n");
      printf("********* index_vers_index OK ********\n");
      printf("**************************************\n");
      //printf("buffer contents: %s\n", buf);
    } else {
      TBX_FAILURE("Echec sur index_vers_index\n");
    }
  }

  free(buf);
}


static void index_vers_index_different(void){
  CCS_datadesc_t my_type;
  struct CCSI_Segment seg;
  char *buf = NULL;
  char * res = "hello world";

  if (is_server) {
    int count = 1;
    int blocklengths[2] = {5, 5};
    int strides[2] = {0, 6};

    nm_sr_request_t request;
    /* server
     */

    buf = malloc(strlen(res));
    memset(buf, ' ', strlen(res));

    CCS_datadesc_create_indexed (2, blocklengths, strides, CCS_DATA8, &my_type);
    CCSI_Segment_init (buf, count, my_type, &seg, 0);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

    buf[11] = '\0';

  } else {
    int count = 1;
    int blocklengths[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    int strides[10] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18};

    char *hello_world = "h e l l o w o r l d";
    nm_sr_request_t request;

    /* client
     */

    buf = malloc(strlen(hello_world));
    memset(buf, ' ', strlen(hello_world));

    memcpy(buf, hello_world, strlen(hello_world));

    //{
    //  int i;
    //  printf("J'envoie =[");
    //  for(i = 0; i < strlen(hello_world); i++)
    //    printf("%c", buf[i]);
    //  printf("]\n");
    //}

    CCS_datadesc_create_indexed (10, blocklengths, strides, CCS_DATA8, &my_type);
    CCSI_Segment_init (buf, count, my_type, &seg, 0);


    nm_sr_isend_datatype(p_core, gate_id, 0, &seg, &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    if(strcmp(buf, res) == 0){
      printf("**************************************\n");
      printf("*** index_vers_index_different OK ****\n");
      printf("**************************************\n");
      //printf("buffer contents: %s\n", buf);
    } else {
      TBX_FAILURE("Echec sur index_vers_index_different\n");
    }
  }

  free(buf);
}


static void index_vers_iov(void){
  char *hello = "hello";
  char *world = "world";
  //struct iovec iov[2];
  struct iovec *iov = NULL;

  if (is_server) {
    nm_sr_request_t request;
    /* server
     */

    iov = malloc ( 2 * sizeof(struct iovec));
    iov[0].iov_base = malloc(strlen(hello) + 1);
    memset(iov[0].iov_base, ' ', strlen(hello) + 1);
    iov[0].iov_len  = strlen(hello);

    iov[1].iov_base = malloc(strlen(world) + 1);
    memset(iov[1].iov_base, ' ', strlen(world) + 1);
    iov[1].iov_len  = strlen(world);

    nm_sr_irecv_iov(p_core, r_gate_id, 0, iov, 2, &request);
    nm_sr_rwait(p_core, &request);

  } else {
    int count = 1;
    int blocklengths[2] = {5, 5};
    int strides[2] = {0, 6};
    CCS_datadesc_t my_type;
    struct CCSI_Segment seg;

    char *buf = NULL;
    char *hello_world = "hello world";
    nm_sr_request_t request;

    /* client
     */
    buf = malloc(strlen(hello_world));
    memcpy(buf, hello_world, strlen(hello_world));

    CCS_datadesc_create_indexed (2, blocklengths, strides, CCS_DATA8, &my_type);

    CCSI_Segment_init (buf, count, my_type, &seg, 0);

    nm_sr_isend_datatype(p_core, gate_id, 0, &seg, &request);
    nm_sr_swait(p_core, &request);

    free(buf);
  }

  if (is_server) {
    int i;

    for(i = 0; i < strlen(hello); i++){
      if(((char *)iov[0].iov_base)[i] != hello[i]){
        printf("%c - %c\n", ((char *)iov[0].iov_base)[i], hello[i]);
        TBX_FAILURE("Echec sur index_vers_iov\n");
      }
    }

    for(i = 0; i < strlen(world); i++){
      if(((char *)iov[1].iov_base)[i] != world[i]){
        printf("%c - %c\n", ((char *)iov[0].iov_base)[i], world[i]);
        TBX_FAILURE("Echec sur index_vers_iov\n");
      }
    }

    printf("**************************************\n");
    printf("********* index_vers_iov OK **********\n");
    printf("**************************************\n");

    free(iov[0].iov_base);
    free(iov[1].iov_base);
    free(iov);
  }
}


static void contig_vers_index(void){
  CCS_datadesc_t my_type;
  struct CCSI_Segment seg;

  int count = 1;
  int blocklengths[3] = {1, 3, 2};
  int strides[3] = {0, 2, 6};

  char *buf = malloc(count * strlen(msg) + 1);

  CCS_datadesc_create_indexed (3, blocklengths, strides, CCS_DATA8, &my_type);

  CCSI_Segment_init (buf, count, my_type, &seg, 0);

  if (is_server) {
    nm_sr_request_t request;
    /* server
     */
    memset(buf, ' ', count * strlen(msg) + 1);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

    buf[strlen(msg)] = '\0';

  } else {
    nm_sr_request_t request;
    /* client
     */
    strcpy(buf, msg2);

    nm_sr_isend(p_core, gate_id, 0, buf, strlen(buf), &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    if(strcmp(buf, msg) == 0){
      printf("**************************************\n");
      printf("******** contig_vers_index OK ********\n");
      printf("**************************************\n");
      //printf("buffer contents: %s\n", buf);
    } else {
      TBX_FAILURE("Echec sur contig_vers_index\n");
    }
  }

  free(buf);
}


static void iov_vers_index(void){
  char *hello_world = "hello world";
  char *buf = NULL;

  if (is_server) {
    CCS_datadesc_t my_type;
    struct CCSI_Segment seg;
    int count = 1;
    int blocklengths[2] = {5, 5};
    int strides[2] = {0, 6};

    nm_sr_request_t request;
    /* server
     */

    buf = malloc(count * strlen(hello_world) + 1);
    memset(buf, ' ', count * strlen(hello_world) + 1);

    CCS_datadesc_create_indexed (2, blocklengths, strides, CCS_DATA8, &my_type);
    CCSI_Segment_init (buf, count, my_type, &seg, 0);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

    buf[strlen(hello_world)] = '\0';

  } else {
    struct iovec *iov = NULL;
    nm_sr_request_t request;
    /* client
     */

    iov = malloc ( 2 * sizeof(struct iovec));
    iov[0].iov_base = "hello";
    iov[0].iov_len  = strlen("hello");
    iov[1].iov_base = "world";
    iov[1].iov_len  = strlen("world");

    nm_sr_isend_iov(p_core, gate_id, 0, iov, 2, &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    if(strcmp(buf, hello_world) == 0){
      printf("**************************************\n");
      printf("******** iov_vers_index OK ********\n");
      printf("**************************************\n");
      //printf("buffer contents: %s\n", buf);
    } else {
      TBX_FAILURE("Echec sur iov_vers_index\n");
    }
  }

  free(buf);
}


static void large_index_vers_contig(void){
  CCS_datadesc_t my_type;
  struct CCSI_Segment seg;

  int count = 1;
  int blocklengths[2] = {SIZE/2 - 10, SIZE/2};
  int strides[2] = {0, SIZE/2};

  char *buf = malloc(count * SIZE);

  CCS_datadesc_create_indexed (2, blocklengths, strides, CCS_DATA8, &my_type);

  CCSI_Segment_init (buf, count, my_type, &seg, 0);

  if (is_server) {
    nm_sr_request_t request;
    /* server
     */
    memset(buf, ' ', SIZE);

    nm_sr_irecv(p_core, r_gate_id, 0, buf, SIZE, &request);
    nm_sr_rwait(p_core, &request);

  } else {
    nm_sr_request_t request;
    /* client
     */

    {
      char *src, *dst;

      memset(buf, ' ', SIZE);
      dst = buf;
      src = (char *) msg_beg;
      while(*src)
        *dst++ = *src++;

      dst = buf + SIZE - strlen(msg_end) - 1;
      src = (char *) msg_end;
      while(*src)
        *dst++ = *src++;

      dst = buf + SIZE - 1;
      *dst = '\0';

      //printf("Here's the message we're going to send : [%s]\n", buf);
    }

    nm_sr_isend_datatype(p_core, gate_id, 0, &seg, &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    char *hello000000world = malloc(SIZE);
    {
      char *src, *dst;

      memset(hello000000world, ' ', SIZE);
      dst = hello000000world;
      src = (char *) msg_beg;
      while(*src)
        *dst++ = *src++;

      dst = hello000000world + SIZE - 10 - strlen(msg_end) - 1;
      src = (char *) msg_end;
      while(*src)
        *dst++ = *src++;

      dst = hello000000world + SIZE - 1 -10;
      *dst = '\0';
    }

    if(strcmp(buf, hello000000world) == 0){
      printf("**************************************\n");
      printf("******** large_index_vers_contig OK **\n");
      printf("**************************************\n");
      //printf("buffer contents: %s\n", buf);
    } else {
      TBX_FAILURE("Echec sur large_index_vers_contig\n");
    }
    free(hello000000world);
  }
  free(buf);
}

static void large_index_vers_index(void){
  CCS_datadesc_t my_type;
  struct CCSI_Segment seg;

  int count = 1;
  int blocklengths[2] = {SIZE/2 - 10, SIZE/2};
  int strides[2] = {0, SIZE/2};

  char *buf = malloc(count * SIZE);

  CCS_datadesc_create_indexed (2, blocklengths, strides, CCS_DATA8, &my_type);

  CCSI_Segment_init (buf, count, my_type, &seg, 0);

  if (is_server) {
    nm_sr_request_t request;
    /* server
     */
    memset(buf, ' ', SIZE);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

  } else {
    nm_sr_request_t request;
    /* client
     */

    {
      char *src, *dst;

      memset(buf, ' ', SIZE);
      dst = buf;
      src = (char *) msg_beg;
      while(*src)
        *dst++ = *src++;

      dst = buf + SIZE - strlen(msg_end) - 1;
      src = (char *) msg_end;
      while(*src)
        *dst++ = *src++;

      dst = buf + SIZE - 1;
      *dst = '\0';

      //printf("Here's the message we're going to send : [%s]\n", buf);
    }

    nm_sr_isend_datatype(p_core, gate_id, 0, &seg, &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    char *hello000000world = malloc(SIZE);
    {
      char *src, *dst;

      memset(hello000000world, ' ', SIZE);
      dst = hello000000world;
      src = (char *) msg_beg;
      while(*src)
        *dst++ = *src++;

      dst = hello000000world + SIZE - strlen(msg_end) - 1;
      src = (char *) msg_end;
      while(*src)
        *dst++ = *src++;

      dst = hello000000world + SIZE - 1;
      *dst = '\0';
    }

    {
      int i;
      for(i = 0; i < SIZE; i++){
        if (buf[i] != hello000000world[i])
          printf("[%d] %c != %c\n", i, buf[i], hello000000world[i]);
      }
    }

    if(strcmp(buf, hello000000world) == 0){
      printf("**************************************\n");
      printf("**** large_index_vers_index OK *******\n");
      printf("**************************************\n");
      //printf("buffer contents: %s\n", buf);
    } else {
      TBX_FAILURE("Echec sur large_index_vers_index\n");
    }
    free(hello000000world);
  }
  free(buf);
}

static void large_index_vers_index_different(void){
  CCS_datadesc_t my_type;
  struct CCSI_Segment seg;
  int count = 1;

  int nb_1KB_block = SIZE/1000;
  int first_offset = SIZE - nb_1KB_block * 1000;
  int first_block_len = 1;

  char *buf = malloc(count * SIZE);

  if (is_server) {

    nm_sr_request_t request;
    /* server
     */

    memset(buf, ' ', SIZE);

    CCS_datadesc_create_contiguous(SIZE, CCS_DATA8, &my_type);

    CCSI_Segment_init (buf, count, my_type, &seg, 0);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

  } else {
    int blocklengths[1+ nb_1KB_block];
    int strides[1+ nb_1KB_block];

    nm_sr_request_t request;
    int i;
    /* client
     */

    for(i = 0; i < SIZE; i++)
      buf[i] = 'a' + (i%26);

    blocklengths[0] = first_block_len;
    strides[0] = 0;

    blocklengths[1] = 900;
    strides[1] = first_offset;

    for(i = 2; i <= nb_1KB_block; i++){
      blocklengths[i] = 900;
      strides[i] = strides[i-1]+1000;
    }

    CCS_datadesc_create_indexed (1+ nb_1KB_block, blocklengths, strides, CCS_DATA8, &my_type);

    CCSI_Segment_init (buf, count, my_type, &seg, 0);

    nm_sr_isend_datatype(p_core, gate_id, 0, &seg, &request);
    nm_sr_swait(p_core, &request);
  }


  if(is_server){
    char check_char = 'a';
    int  ajout = 0;
    char *cur_char = buf;
    int i, j;

    i = 0;
    do{
      if(*cur_char != check_char){
        printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
        TBX_FAILURE("Echec sur large_index_avec_bcp_blocks_vers_contig\n");
      }
      cur_char++;
      ajout++;
      check_char = 'a' + (ajout % 26);
      i++;
    }while(i < first_block_len);

    ajout=first_offset;
    check_char = 'a' + (ajout % 26);

    for(i = 1; i <= nb_1KB_block; i++){
      for(j = 0; j < 900; j++){

        if(*cur_char != check_char){
          printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
          TBX_FAILURE("Echec sur large_index_avec_bcp_blocks_vers_contig\n");
        }

        cur_char++;
        ajout++;
        check_char = 'a' + (ajout % 26);
      }
      ajout += 100;
      check_char = 'a' + (ajout % 26);
    }

    printf("**********************************************\n");
    printf("* large_index_avec_bcp_blocks_vers_contig OK *\n");
    printf("**********************************************\n");
  }
}

static void large_index_vers_iov(void){

  int nb_1KB_block = SIZE/1000;
  int first_offset = SIZE - nb_1KB_block * 1000;
  int first_block_len = 1;

  struct iovec * iov= NULL;

  if (is_server) {
    nm_sr_request_t request;
    int i;
    /* server
     */

    int total_len = first_block_len + nb_1KB_block * 900;

    int next_entry_len = total_len / 10;
    int first_entry_len = (total_len - 10 * next_entry_len) + next_entry_len;

    iov = malloc(10 * sizeof(struct iovec));
    iov[0].iov_base = malloc(first_entry_len);
    memset(iov[0].iov_base, ' ', first_entry_len);
    iov[0].iov_len = first_entry_len;

    for(i = 1; i < 10; i++){
      iov[i].iov_base = malloc(next_entry_len);
      memset(iov[i].iov_base, ' ', next_entry_len);
      iov[i].iov_len = next_entry_len;
    }

    nm_sr_irecv_iov(p_core, r_gate_id, 0, iov, 10, &request);
    nm_sr_rwait(p_core, &request);

  } else {
    CCS_datadesc_t my_type;
    struct CCSI_Segment seg;
    int count = 1;

    int blocklengths[1+ nb_1KB_block];
    int strides[1+ nb_1KB_block];

    nm_sr_request_t request;
    int i;
    /* server
     */

    char *buf = malloc(SIZE);

    for(i = 0; i < SIZE; i++)
      buf[i] = 'a' + (i%26);

    blocklengths[0] = first_block_len;
    strides[0] = 0;

    blocklengths[1] = 900;
    strides[1] = first_offset;

    for(i = 2; i <= nb_1KB_block; i++){
      blocklengths[i] = 900;
      strides[i] = strides[i-1]+1000;
    }

    CCS_datadesc_create_indexed (1+ nb_1KB_block, blocklengths, strides, CCS_DATA8, &my_type);

    CCSI_Segment_init (buf, count, my_type, &seg, 0);

    nm_sr_isend_datatype(p_core, gate_id, 0, &seg, &request);
    nm_sr_swait(p_core, &request);
  }

  if(is_server){
    char check_char = 'a';
    int  ajout = 0;
    char *cur_char = iov[0].iov_base;
    int i = 0, j = 0;
    int idx = 0;

    if(*cur_char != check_char){
      printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
      TBX_FAILURE("Echec sur large_index_vers_iov\n");
    }

    cur_char++;
    ajout=first_offset;
    check_char = 'a' + (ajout % 26);
    i++;

    while(1){
      while(i < iov[idx].iov_len && j < 900){
        if(*cur_char != check_char){
          printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
          TBX_FAILURE("Echec sur large_index_vers_iov\n");
        }
        i++;
        j++;
        ajout++;
        check_char = 'a' + (ajout % 26);
        cur_char++;
      }

      if(j>=900){
        ajout+= 99;
        check_char = 'a' + (ajout % 26);
        j=0;
      }
      if(i < 9 && i >= iov[idx].iov_len){
        idx++;
        i = 0;
      } else {
        break;
      }
    }

    printf("**********************************************\n");
    printf("********* large_index_vers_iov OK ************\n");
    printf("**********************************************\n");
  }
}

static void large_contig_vers_index(void){
  CCS_datadesc_t my_type;
  struct CCSI_Segment seg;

  int count = 1;
  int blocklengths[2] = {SIZE/2 - 10, SIZE/2};
  int strides[2] = {0, SIZE/2};

  char *buf = malloc(count * SIZE);

  CCS_datadesc_create_indexed (2, blocklengths, strides, CCS_DATA8, &my_type);

  CCSI_Segment_init (buf, count, my_type, &seg, 0);

  if (is_server) {
    nm_sr_request_t request;
    /* server
     */
    memset(buf, ' ', SIZE);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

  } else {
    nm_sr_request_t request;
    /* client
     */

    {
      char *src, *dst;

      memset(buf, ' ', SIZE);
      dst = buf;
      src = (char *) msg_beg;
      while(*src)
        *dst++ = *src++;

      dst = buf + SIZE - strlen(msg_end) - 1 - 10;
      src = (char *) msg_end;
      while(*src)
        *dst++ = *src++;

      dst = buf + SIZE - 1 - 10;
      *dst = '\0';

      //printf("Here's the message we're going to send : [%s]\n", buf);
    }

    nm_sr_isend(p_core, gate_id, 0, buf, SIZE-10, &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    char *hello000000world = malloc(SIZE);
    {
      char *src, *dst;

      memset(hello000000world, ' ', SIZE);
      dst = hello000000world;
      src = (char *) msg_beg;
      while(*src)
        *dst++ = *src++;

      dst = hello000000world + SIZE - strlen(msg_end) - 1;
      src = (char *) msg_end;
      while(*src)
        *dst++ = *src++;

      dst = hello000000world + SIZE - 1;
      *dst = '\0';
    }

    {
      int i;
      for(i = 0; i < SIZE; i++){
        if (buf[i] != hello000000world[i])
          printf("[%d] %c != %c\n", i, buf[i], hello000000world[i]);
      }
    }

    if(strcmp(buf, hello000000world) == 0){
      printf("**************************************\n");
      printf("****** large_contig_vers_index OK ****\n");
      printf("**************************************\n");
      //printf("buffer contents: %s\n", buf);
    } else {
      TBX_FAILURE("Echec sur large_contig_vers_index\n");
    }
    free(hello000000world);
  }

  free(buf);
}


static void large_index_avec_bcp_blocs_vers_contig(void){
  CCS_datadesc_t my_type;
  struct CCSI_Segment seg;

  int count = 1;
  int i;

  // avec SIZE = 64K, on a 1 bloc de 536o + 65 blocs de 900o (on les espace de 100o)
  int nb_1KB_block = SIZE/1000;

  int first_block_len = 1;
  int first_offset = SIZE - nb_1KB_block * 1000;

  int blocklengths[1+ nb_1KB_block];
  int strides[1+ nb_1KB_block];

  blocklengths[0] = first_block_len;
  strides[0] = 0;

  blocklengths[1] = 900;
  strides[1] = first_offset;


  for(i = 2; i <= nb_1KB_block; i++){
    blocklengths[i] = 900;
    strides[i] = strides[i-1]+1000;
  }

  char *buf = malloc(count * SIZE);

  CCS_datadesc_create_indexed (1+ nb_1KB_block, blocklengths, strides, CCS_DATA8, &my_type);

  CCSI_Segment_init (buf, count, my_type, &seg, 0);

  if (is_server) {
    nm_sr_request_t request;
    /* server
     */
    memset(buf, ' ', SIZE);

    nm_sr_irecv(p_core, r_gate_id, 0, buf, SIZE, &request);
    nm_sr_rwait(p_core, &request);

  } else {
    nm_sr_request_t request;
    /* client
     */
    for(i = 0; i < SIZE; i++)
      buf[i] = 'a' + (i%26);

    nm_sr_isend_datatype(p_core, gate_id, 0, &seg, &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    char check_char = 'a';
    int  ajout = 0;
    char *cur_char = buf;
    int i, j;

    i = 0;
    do{
      if(*cur_char != check_char){
        printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
        TBX_FAILURE("Echec sur large_index_avec_bcp_blocs_vers_contig\n");
      }
      cur_char++;
      ajout++;
      check_char = 'a' + (ajout % 26);
      i++;
    }while(i < first_block_len);

    ajout = first_offset;
    check_char = 'a' + (ajout % 26);

    for(i = 1; i <= nb_1KB_block; i++){
      for(j = 0; j < 900; j++){

        if(*cur_char != check_char){
          printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
          TBX_FAILURE("Echec sur large_index_avec_bcp_blocs_vers_contig\n");
        }

        cur_char++;
        ajout++;
        check_char = 'a' + (ajout % 26);
      }
      ajout += 100;
      check_char = 'a' + (ajout % 26);
    }

    printf("**********************************************\n");
    printf("* large_index_avec_bcp_blocs_vers_contig OK *\n");
    printf("**********************************************\n");
  }
  free(buf);
}

static void large_contig_vers_index_avec_bcp_blocs(void){
  CCS_datadesc_t my_type;
  struct CCSI_Segment seg;

  int count = 1;
  int i;

  // avec SIZE = 64K, on a 1 bloc de 536o + 65 blocs de 900o (on les espace de 100o)
  int nb_1KB_block = SIZE/1000;

  int first_block_len = 1;
  int first_offset = SIZE - nb_1KB_block * 1000;

  int blocklengths[1+ nb_1KB_block];
  int strides[1+ nb_1KB_block];

  blocklengths[0] = first_block_len;
  strides[0] = 0;

  blocklengths[1] = 900;
  strides[1] = first_offset;


  for(i = 2; i <= nb_1KB_block; i++){
    blocklengths[i] = 900;
    strides[i] = strides[i-1]+1000;
  }

  char *buf = malloc(count * SIZE);

  CCS_datadesc_create_indexed (1+ nb_1KB_block, blocklengths, strides, CCS_DATA8, &my_type);

  CCSI_Segment_init (buf, count, my_type, &seg, 0);

  if (is_server) {
    nm_sr_request_t request;
    /* server
     */
    memset(buf, ' ', SIZE);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

  } else {
    nm_sr_request_t request;
    /* client
     */
    for(i = 0; i < SIZE; i++)
      buf[i] = 'a' + (i%26);

    nm_sr_isend(p_core, gate_id, 0, buf, SIZE - (first_offset - 1) - (100 * nb_1KB_block), &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    char check_char = 'a';
    char *cur_char = buf;
    int ajout = 0;
    int i, j;

    if(*cur_char != check_char){
      printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
      TBX_FAILURE("Echec sur large_contig_vers_index_avec_bcp_blocs\n");
    }

    cur_char += first_offset;

    ajout++;
    check_char = 'a' + (ajout % 26);

    for(i = 1; i <= nb_1KB_block; i++){
      for(j = 0; j < 900; j++){

        if(*cur_char != check_char){
          printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
          TBX_FAILURE("Echec sur large_contig_vers_index_avec_bcp_blocs\n");
        }

        cur_char++;
        ajout++;
        check_char = 'a' + (ajout % 26);
      }

      cur_char += 100;
    }

    printf("**********************************************\n");
    printf("* large_contig_vers_index_avec_bcp_blocs OK *\n");
    printf("**********************************************\n");
  }
  free(buf);
}

static void iov_vers_index_avec_bcp_blocs(void){
  int nb_1KB_block = SIZE/1000;
  int first_offset = SIZE - nb_1KB_block * 1000;
  int first_block_len = 1;

  char * buf = NULL;

  if (is_server) {
    CCS_datadesc_t my_type;
    struct CCSI_Segment seg;
    int count = 1;

    int blocklengths[1+ nb_1KB_block];
    int strides[1+ nb_1KB_block];

    nm_sr_request_t request;
    int i;
    /* server
     */

    buf = malloc(SIZE);
    memset(buf, ' ', SIZE);

    blocklengths[0] = first_block_len;
    strides[0] = 0;

    blocklengths[1] = 900;
    strides[1] = first_offset;

    for(i = 2; i <= nb_1KB_block; i++){
      blocklengths[i] = 900;
      strides[i] = strides[i-1]+1000;
    }

    CCS_datadesc_create_indexed (1+ nb_1KB_block, blocklengths, strides, CCS_DATA8, &my_type);
    CCSI_Segment_init (buf, count, my_type, &seg, 0);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);

    
  } else {
    int total_len = first_block_len + nb_1KB_block * 900;
    int next_entry_len  = total_len / 10;
    int first_entry_len = (total_len - 10 * next_entry_len) + next_entry_len;
    int cur = 0;

    struct iovec *iov = NULL;

    nm_sr_request_t request;
    int i, j;
    /* client
     */

    iov = malloc(10 * sizeof(struct iovec));
    iov[0].iov_base = malloc(first_entry_len);
    iov[0].iov_len = first_entry_len;

    for(i = 1; i < 10; i++){
      iov[i].iov_base = malloc(next_entry_len);
      iov[i].iov_len = next_entry_len;
    }

    for(i=0; i<10; i++){
      for(j=0; j<iov[i].iov_len; j++){
        ((char*)iov[i].iov_base)[j] = 'a' + (cur%26);
        cur++;
      }
    }

    nm_sr_isend_iov(p_core, gate_id, 0, iov, 10, &request);
    nm_sr_swait(p_core, &request);
  }

  if(is_server){
    char check_char = 'a';
    int  ajout = 0;
    char *cur_char = buf;
    int i=0, j;

    do{
      if(*cur_char != check_char){
        printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
        TBX_FAILURE("Echec sur iov_vers_index_avec_bcp_blocs\n");
      }
      cur_char++;
      ajout++;
      check_char = 'a' + (ajout % 26);
      i++;
    }while(i < first_block_len);

    cur_char = buf + first_offset;

    for(i = 1; i <= nb_1KB_block; i++){
      for(j = 0; j < 900; j++){

        if(*cur_char != check_char){
          printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
          TBX_FAILURE("Echec sur iov_vers_index_avec_bcp_blocs\n");
        }

        cur_char++;
        ajout++;
        check_char = 'a' + (ajout % 26);
      }
      cur_char += 100;
    }

    printf("**********************************************\n");
    printf("***** iov_vers_index_avec_bcp_blocs OK *******\n");
    printf("**********************************************\n");
  }
}

static void iov_vers_large_index(void){
  char *buf = NULL;


  if(is_server){
    CCS_datadesc_t my_type;
    struct CCSI_Segment seg;

    int count = 1;
    int blocklengths[2] = {SIZE/2 - 10, SIZE/2};
    int strides[2] = {0, SIZE/2};

    nm_sr_request_t request;
    /* server
     */

    buf = malloc(count * SIZE);
    memset(buf, ' ', SIZE);

    CCS_datadesc_create_indexed (2, blocklengths, strides, CCS_DATA8, &my_type);
    CCSI_Segment_init (buf, count, my_type, &seg, 0);

    nm_sr_irecv_datatype(p_core, r_gate_id, 0, &seg, &request);
    nm_sr_rwait(p_core, &request);



  } else {
    int total_len = SIZE - 10;

    int next_entry_len  = total_len / 10;
    int first_entry_len = (total_len - 10 * next_entry_len) + next_entry_len;
    int cur = 0;

    struct iovec *iov = NULL;

    nm_sr_request_t request;
    int i, j;
    /* client
     */

    iov = malloc(10 * sizeof(struct iovec));
    iov[0].iov_base = malloc(first_entry_len);
    iov[0].iov_len = first_entry_len;

    for(i = 1; i < 10; i++){
      iov[i].iov_base = malloc(next_entry_len);
      iov[i].iov_len = next_entry_len;
    }

    for(i=0; i<10; i++){
      for(j=0; j<iov[i].iov_len; j++){
        ((char*)iov[i].iov_base)[j] = 'a' + (cur%26);
        cur++;
      }
    }

    nm_sr_isend_iov(p_core, gate_id, 0, iov, 10, &request);
    nm_sr_swait(p_core, &request);
  }

  if(is_server){
    char check_char = 'a';
    int  ajout = 0;
    char *cur_char = buf;
    int j;

    for(j = 0; j < SIZE/2 - 10; j++){
      if(*cur_char != check_char){
        printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
        TBX_FAILURE("Echec sur iov_vers_large_index\n");
      }

      cur_char++;
      ajout++;
      check_char = 'a' + (ajout % 26);
    }

    cur_char += 10;

    for(j = 0; j < SIZE/2; j++){
      if(*cur_char != check_char){
        printf("[%d] cur_char = %c - check_char = %c\n", ajout, *cur_char, check_char);
        TBX_FAILURE("Echec sur iov_vers_large_index\n");
      }

      cur_char++;
      ajout++;
      check_char = 'a' + (ajout % 26);
    }


    printf("**********************************************\n");
    printf("*********** iov_vers_large_index OK **********\n");
    printf("**********************************************\n");
  }
}



int
main(int	  argc,
     char	**argv) {

  init(&argc, argv);

#ifdef ANY_SRC
  r_gate_id = NM_ANY_GATE;
#else
  r_gate_id = gate_id;
#endif

  // datatype court vers ... - RQ : dans tous les cas, les données sont copiées donc pas besoin de jouer sur la densité
  index_vers_contig();
  index_vers_index();
  index_vers_index_different();
  index_vers_iov();

  // ... vers un datatype court
  contig_vers_index();
  iov_vers_index();

  /*************************/

  // datatype long vers ...
  //1) avec peu de longs blocs
  large_index_vers_contig();
  large_index_vers_index();
  large_index_vers_index_different();
  large_index_vers_iov();

  //2) avec bcp de petits blocs
  large_index_avec_bcp_blocs_vers_contig();
  //-> large_index_avec_bcp_blocs_vers_index_identique();
  //-> large_index_avec_bcp_blocs_vers_index_different_avec_bcp_blocs();
  //-> large_index_avec_bcp_blocs_vers_index_avec_gros_blocs();
  //-> large_index_avec_bcp_blocs_vers_iov();

  // ... vers un datatype long
  large_contig_vers_index();
  large_contig_vers_index_avec_bcp_blocs();
  iov_vers_index_avec_bcp_blocs();
  iov_vers_large_index();

  nmad_exit();
  exit(0);
}
