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
#include <assert.h>

#include "helper.h"

#define PRINT_CONTENT 0

#define SIZE  (64 * 1024)

const char *msg1 = "hello";
const char *msg2 = ", ";
const char *msg3 = "world";
const char *plop = "plop";

struct nm_so_interface *sr_if;
nm_gate_id_t gate_id;

void large_contigu_vers_large_contigu(void){
  struct iovec iov[1];
  char * message = NULL;
  char *src, *dst;
  char *recv_plop = NULL;

  /* Build the message to be sent */
  message = malloc(SIZE);
  memset(message, '\0', SIZE);
  memset(message, ' ' , SIZE-1);
  dst = message;
  src = (char *) msg1;
  while(*src)
    *dst++ = *src++;
  src = (char *)msg2;
  while(*src)
    *dst++ = *src++;

  dst = message + SIZE - strlen(msg3) - 1;
  src = (char *) msg3;
  while(*src)
    *dst++ = *src++;

  dst = message + SIZE - 1;
  *dst = '\0';

  /* init recv buffers */
  iov[0].iov_len  = SIZE;
  iov[0].iov_base = malloc(SIZE);
  memset(iov[0].iov_base, 0, SIZE);

  recv_plop = malloc(strlen(plop)+1);
  memset(recv_plop, 0, strlen(plop)+1);

  if (is_server()) {
    nm_so_request request0;
    nm_so_request request1;

    /* server */
    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 1, &request1);

    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    nm_so_request request;
    /* client */

    nm_so_sr_isend(sr_if, gate_id, 0, message, SIZE, &request);
    nm_so_sr_swait(sr_if, request);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request);
    nm_so_sr_swait(sr_if, request);

    sleep(2);
  }

  if (is_server()) {
    assert (strcmp ((char *)iov[0].iov_base, message) == 0);
    assert (strcmp (recv_plop, plop) == 0);

    printf("large_contigu_vers_large_contigu OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s]\n", (char *)iov[0].iov_base);
#endif
  }

  free(message);

  free(iov[0].iov_base);
  free(recv_plop);
}

void court_contigu_vers_court_contigu(void){
  char *buf  = "hello, world";
  struct iovec iov[1];
  char *recv_plop = NULL;
  nm_so_request request0, request1;

  /* init recv buffers */
  iov[0].iov_len  = strlen(buf) + 1;
  iov[0].iov_base = malloc(strlen(buf)+1);
  memset(iov[0].iov_base, 0, strlen(buf)+1);

  recv_plop = malloc(strlen(plop)+1);
  memset(recv_plop, 0, strlen(plop)+1);


  if (is_server()) {
    /* server */

    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);


    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 1, &request1);
    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    /* client */
    nm_so_sr_isend(sr_if, gate_id, 0, buf, 1+strlen(buf), &request0);
    nm_so_sr_swait(sr_if, request0);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request1);
    nm_so_sr_swait(sr_if, request1);

    sleep(2);
  }

  if (is_server()) {
    assert (strcmp ((char *)iov[0].iov_base, buf) == 0);
    assert (strcmp (recv_plop, plop) == 0);

    printf("court_contigu_vers_court_contigu OK\n");
#if PRINT_CONTENT
    printf("buffers contents: [%s]\n", (char *)iov[0].iov_base);
#endif
  }
  free(iov[0].iov_base);
  free(recv_plop);
}

void large_contigu_vers_large_contigu_plus_long(void){
  struct iovec iov[1];
  char * message = NULL;
  char *recv_plop = NULL;
  char *src, *dst;

  /* Build the message to be sent */
  message = malloc(SIZE);
  if(!message){
    perror("malloc failed\n");
    exit(0);
  }

  memset(message, '\0', SIZE);
  memset(message, ' ' , SIZE-1);

  dst = message;
  src = (char *) msg1;
  while(*src)
    *dst++ = *src++;
  src = (char *)msg2;
  while(*src)
    *dst++ = *src++;

  dst = message + SIZE - (strlen(msg3)+1);
  src = (char *) msg3;
  while(*src)
    *dst++ = *src++;


  /* init recv buffers */
  iov[0].iov_len  = SIZE + 10;
  iov[0].iov_base = malloc(SIZE + 10);
  if(!iov[0].iov_base){
    perror("malloc failed\n");
    exit(0);
  }
  memset(iov[0].iov_base, 0, SIZE + 10);

  recv_plop = malloc(strlen(plop)+1);
  memset(recv_plop, 0, strlen(plop)+1);

  if (is_server()) {
    nm_so_request request0;
    nm_so_request request1;

    /* server */
    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 1, &request1);

    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    nm_so_request request;
    /* client */

    nm_so_sr_isend(sr_if, gate_id, 0, message, SIZE, &request);
    nm_so_sr_swait(sr_if, request);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request);
    nm_so_sr_swait(sr_if, request);

    sleep(2);
  }

  if (is_server()) {
    assert (strcmp ((char *)iov[0].iov_base, message) == 0);
    assert (strcmp (recv_plop, plop) == 0);

    printf("large_contigu_vers_large_contigu_plus_long OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s]\n", (char *)iov[0].iov_base);
#endif
  }
  free(message);
  free(iov[0].iov_base);
  free(recv_plop);
}

void court_contigu_vers_court_contigu_plus_long(void){
  char *buf  = "hello, world";
  char *recv_plop = NULL;
  struct iovec iov[1];
  nm_so_request request0, request1;

  /* init recv buffers */
  iov[0].iov_len  = strlen(buf) + 10;
  iov[0].iov_base = malloc(strlen(buf)+10);
  memset(iov[0].iov_base, 0, strlen(buf)+10);

  recv_plop = malloc(strlen(plop)+1);
  memset(recv_plop, 0, strlen(plop)+1);

  if (is_server()) {
    /* server */
    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);


    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 1, &request1);
    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    /* client */
    nm_so_sr_isend(sr_if, gate_id, 0, buf, 1+strlen(buf), &request0);
    nm_so_sr_swait(sr_if, request0);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request1);
    nm_so_sr_swait(sr_if, request1);

    sleep(2);
  }

  if (is_server()) {
    assert (strcmp ((char *)iov[0].iov_base, buf) == 0);
    assert (strcmp (recv_plop, plop) == 0);

    printf("court_contigu_vers_court_contigu_plus_long OK\n");
#if PRINT_CONTENT
    printf("buffers contents: [%s]\n", (char *)iov[0].iov_base);
#endif
  }

  free(iov[0].iov_base);
  free(recv_plop);
}

void large_contigu_vers_large_disperse(void){
  struct iovec iov[3];
  char *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  char *message = NULL;
  char *recv_plop = NULL;
  char *src, *dst;
  int msg3_len = SIZE - strlen(msg1) - strlen(msg2);
  nm_so_request request0, request1;

  if (is_server()) {
    /* server */
    buf1 = malloc(strlen(msg1));
    buf2 = malloc(strlen(msg2));
    buf3 = malloc(msg3_len);

    iov[0].iov_len  = strlen(msg1);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg2);
    iov[1].iov_base = buf2;
    iov[2].iov_len  = msg3_len;
    iov[2].iov_base = buf3;

    memset(buf1, 0, strlen(msg1));
    memset(buf2, 0, strlen(msg2));
    memset(buf3, 0, msg3_len);

    recv_plop = malloc(strlen(plop)+1);
    memset(recv_plop, 0, strlen(plop)+1);

    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 3, &request1);
    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    /* client */
    /* Build the message to be sent */
    message = malloc(SIZE);
    memset(message, '\0', SIZE);
    memset(message, ' ' , SIZE-1);
    dst = message;
    src = (char *) msg1;
    while(*src)
      *dst++ = *src++;
    src = (char *)msg2;
    while(*src)
      *dst++ = *src++;

    dst = message + SIZE - strlen(msg3) - 1;
    src = (char *) msg3;
    while(*src)
      *dst++ = *src++;

    nm_so_sr_isend(sr_if, gate_id, 0, message, SIZE, &request0);
    nm_so_sr_swait(sr_if, request0);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request1);
    nm_so_sr_swait(sr_if, request1);

    sleep(2);
  }

  if (is_server()) {
    char *entree0 = malloc(iov[0].iov_len + 1);
    strncpy(entree0, (char *)iov[0].iov_base, iov[0].iov_len);
    entree0[iov[0].iov_len] = '\0';
    assert (strcmp (entree0, msg1) == 0);

    char *entree1 = malloc(iov[1].iov_len + 1);
    strncpy(entree1, (char *)iov[1].iov_base, iov[1].iov_len);
    entree1[iov[1].iov_len] = '\0';
    assert (strcmp (entree1, msg2) == 0);

    char *ooo_msg3 = malloc(msg3_len);
    if(!ooo_msg3){
      perror("malloc a echoué!\n");
      exit(0);
    }

    memset(ooo_msg3, '\0', msg3_len);
    memset(ooo_msg3, ' ' , msg3_len-1);

    dst = ooo_msg3 + msg3_len - (strlen(msg3) + 1);
    src = (char *) msg3;
    while(*src)
      *dst++ = *src++;
    assert (strcmp ((char *)iov[2].iov_base, ooo_msg3) == 0);

    assert (strcmp (recv_plop, plop) == 0);

    printf("large_contigu_vers_large_disperse OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s][%s]\n", entree0, entree1, (char *)iov[2].iov_base);
#endif

    free(buf1);
    free(buf2);
    free(buf3);
    free(recv_plop);

    free(entree0);
    free(entree1);
    free(ooo_msg3);

  } else {
    free(message);
  }
}

void court_contigu_vers_court_disperse(void){
  char *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  char *recv_plop = NULL;
  struct iovec iov[3];
  nm_so_request request0, request1;

  if (is_server()) {
    /* server */
    buf1 = malloc(strlen(msg1));
    buf2 = malloc(strlen(msg2));
    buf3 = malloc(1+strlen(msg3));

    iov[0].iov_len  = strlen(msg1);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg2);
    iov[1].iov_base = buf2;
    iov[2].iov_len  = strlen(msg3)+1;
    iov[2].iov_base = buf3;

    memset(buf1, 0, strlen(msg1));
    memset(buf2, 0, strlen(msg2));
    memset(buf3, 0, strlen(msg3)+1);

    recv_plop = malloc(strlen(plop)+1);
    memset(recv_plop, 0, strlen(plop)+1);

    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 3, &request1);
    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    /* client */
    char *buf  = "hello, world";

    nm_so_sr_isend(sr_if, gate_id, 0, buf, 1+strlen(buf), &request0);
    nm_so_sr_swait(sr_if, request0);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request1);
    nm_so_sr_swait(sr_if, request1);

    sleep(2);
  }

  if (is_server()) {
    char *entree0 = malloc(iov[0].iov_len + 1);
    strncpy(entree0, (char *)iov[0].iov_base, iov[0].iov_len);
    entree0[iov[0].iov_len] = '\0';
    assert (strcmp (entree0, msg1) == 0);

    char *entree1 = malloc(iov[1].iov_len + 1);
    strncpy(entree1, (char *)iov[1].iov_base, iov[1].iov_len);
    entree1[iov[1].iov_len] = '\0';
    assert (strcmp (entree1, msg2) == 0);

    assert (strcmp ((char *)iov[2].iov_base, msg3) == 0);

    assert (strcmp (recv_plop, plop) == 0);

    printf("court_contigu_vers_court_disperse OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s][%s]\n", entree0, entree1, (char *)iov[2].iov_base);
#endif

    free(buf1);
    free(buf2);
    free(buf3);
    free(recv_plop);

    free(entree0);
    free(entree1);
  }
}

void large_disperse_vers_large_disperse_identique(void){
  struct iovec iov[3];
  char *recv_plop = NULL;
  char *src, *dst;
  nm_so_request request0, request1;

  int msg3_len = SIZE - strlen(msg1) - strlen(msg2);

  char *buf1 = malloc(strlen(msg1));
  char *buf2 = malloc(strlen(msg2));
  char *buf3 = malloc(msg3_len);

  if (is_server()) {
    /* server */
    memset(buf1, 0, strlen(msg1));
    memset(buf2, 0, strlen(msg2));
    memset(buf3, 0, msg3_len);

    iov[0].iov_len  = strlen(msg1);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg2);
    iov[1].iov_base = buf2;
    iov[2].iov_len  = msg3_len;
    iov[2].iov_base = buf3;

    recv_plop = malloc(strlen(plop)+1);
    memset(recv_plop, 0, strlen(plop)+1);

    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 3, &request1);
    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    /* client */
    strncpy(buf1, msg1, strlen(msg1));
    strncpy(buf2, msg2, strlen(msg1));

    memset(buf3, '\0', msg3_len);
    memset(buf3, ' ' , msg3_len - 1);
    dst = buf3 + msg3_len - strlen(msg3) - 1;
    src = (char *) msg3;
    while(*src)
      *dst++ = *src++;

    iov[0].iov_len  = strlen(msg1);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg2);
    iov[1].iov_base = buf2;
    iov[2].iov_len  = msg3_len;
    iov[2].iov_base = buf3;

    nm_so_sr_isend_iov(sr_if, gate_id, 0, iov, 3, &request0);
    nm_so_sr_swait(sr_if, request0);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request1);
    nm_so_sr_swait(sr_if, request1);

    sleep(2);
  }

  if (is_server()) {
    char *entree0 = NULL, *entree1 = NULL, *ooo_msg3 = NULL;

    entree0 = malloc(iov[0].iov_len + 1);
    strncpy(entree0, (char *)iov[0].iov_base, iov[0].iov_len);
    entree0[iov[0].iov_len] = '\0';
    assert (strcmp (entree0, msg1) == 0);

    entree1 = malloc(iov[1].iov_len + 1);
    strncpy(entree1, (char *)iov[1].iov_base, iov[1].iov_len);
    entree1[iov[1].iov_len] = '\0';
    assert (strcmp (entree1, msg2) == 0);

    ooo_msg3 = malloc(msg3_len * sizeof(char));
    if(!ooo_msg3){
      perror("le malloc a échoue\n");
      exit(0);
    }

    memset(ooo_msg3, '\0', msg3_len);
    memset(ooo_msg3, ' ' , msg3_len-1);
    dst = ooo_msg3 + msg3_len - (strlen(msg3) + 1);
    src = (char *) msg3;
    while(*src)
      *dst++ = *src++;

    assert (strcmp (ooo_msg3, (char *)iov[2].iov_base) == 0);

    assert (strcmp (recv_plop, plop) == 0);

    printf("large_disperse_vers_large_disperse_identique OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s][%s]\n", entree0, entree1, (char *)iov[2].iov_base);
#endif

    free(recv_plop);
    free(entree0);
    free(entree1);
    free(ooo_msg3);
  }

  free(buf1);
  free(buf2);
  free(buf3);
}

void court_disperse_vers_court_disperse_identique(void){
  struct iovec iov[3];
  char *recv_plop = NULL;
  nm_so_request request0, request1;

  char *buf1 = malloc(strlen(msg1));
  char *buf2 = malloc(strlen(msg2));
  char *buf3 = malloc(1+strlen(msg3));

  if (is_server()) {
    /* server */
    memset(buf1, 0, strlen(msg1));
    memset(buf2, 0, strlen(msg2));
    memset(buf3, 0, strlen(msg3)+1);

    iov[0].iov_len  = strlen(msg1);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg2);
    iov[1].iov_base = buf2;
    iov[2].iov_len  = strlen(msg3) + 1;
    iov[2].iov_base = buf3;

    recv_plop = malloc(strlen(plop)+1);
    memset(recv_plop, 0, strlen(plop)+1);

    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 3, &request1);
    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    /* client */
    strncpy(buf1, msg1,  strlen(msg1));
    strncpy(buf2, msg2,  strlen(msg2));
    strcpy (buf3, msg3);

    iov[0].iov_len  = strlen(msg1);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg2);
    iov[1].iov_base = buf2;
    iov[2].iov_len  = strlen(msg3) + 1;
    iov[2].iov_base = buf3;

    nm_so_sr_isend_iov(sr_if, gate_id, 0, iov, 3, &request0);
    nm_so_sr_swait(sr_if, request0);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request1);
    nm_so_sr_swait(sr_if, request1);

    sleep(2);
  }

  if (is_server()) {
    char *entree0 = malloc(iov[0].iov_len + 1);
    strncpy(entree0, (char *)iov[0].iov_base, iov[0].iov_len);
    entree0[iov[0].iov_len] = '\0';
    assert (strcmp (entree0, msg1) == 0);

    char *entree1 = malloc(iov[1].iov_len + 1);
    strncpy(entree1, (char *)iov[1].iov_base, iov[1].iov_len);
    entree1[iov[1].iov_len] = '\0';
    assert (strcmp (entree1, msg2) == 0);

    assert (strcmp ((char *)iov[2].iov_base, msg3) == 0);

    assert (strcmp (recv_plop, plop) == 0);

    printf("court_disperse_vers_court_disperse_identique OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s][%s]\n", entree0, entree1, (char *)iov[2].iov_base);
#endif

    free(recv_plop);

    free(entree0);
    free(entree1);
  }

  free(buf1);
  free(buf2);
  free(buf3);
}

void large_disperse_vers_large_disperse_disymetrique(void){
  struct iovec iov[3];
  nm_so_request request0, request1;
  char *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  char *recv_plop = NULL;
  char *src, *dst;

  int msg3_len = SIZE - strlen(msg1) - strlen(msg2);

  if (is_server()) {
    /* server */
    buf1 = malloc(strlen(msg1) + strlen(msg2));
    buf2 = malloc(msg3_len);

    memset(buf1, 0, strlen(msg1) + strlen(msg2));
    memset(buf2, 0, msg3_len);

    iov[0].iov_len  = strlen(msg1) + strlen(msg2);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = msg3_len;
    iov[1].iov_base = buf2;

    recv_plop = malloc(strlen(plop)+1);
    memset(recv_plop, 0, strlen(plop)+1);

    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 2, &request1);
    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    /* client */
    buf1 = malloc(strlen(msg1));
    buf2 = malloc(strlen(msg2));
    buf3 = malloc(msg3_len);

    strcpy(buf1, msg1);
    strcpy(buf2, msg2);

    memset(buf3, '\0', msg3_len);
    memset(buf3, ' ' , msg3_len - 1);
    dst = buf3 + msg3_len - strlen(msg3) - 1;
    src = (char *) msg3;
    while(*src)
      *dst++ = *src++;

    iov[0].iov_len  = strlen(msg1);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg2);
    iov[1].iov_base = buf2;
    iov[2].iov_len  = msg3_len;
    iov[2].iov_base = buf3;

    nm_so_sr_isend_iov(sr_if, gate_id, 0, iov, 3, &request0);
    nm_so_sr_swait(sr_if, request0);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request1);
    nm_so_sr_swait(sr_if, request1);

    sleep(2);
  }

  if (is_server()) {
    char *test = malloc(strlen(msg1) + strlen(msg2)+1);
    strcpy(test, msg1);
    strcat(test, msg2);

    char *entree0 = malloc(iov[0].iov_len + 1);
    strncpy(entree0, (char *)iov[0].iov_base, iov[0].iov_len);
    entree0[iov[0].iov_len] = '\0';
    assert (strcmp (entree0, test) == 0);

    char *ooo_msg3 = malloc(msg3_len);
    memset(ooo_msg3, '\0', msg3_len);
    memset(ooo_msg3, ' ' , msg3_len-1);

    dst = ooo_msg3 + msg3_len - (strlen(msg3) + 1);
    src = (char *) msg3;
    while(*src)
      *dst++ = *src++;
    assert (strcmp ((char *)iov[1].iov_base, ooo_msg3) == 0);

    assert (strcmp (recv_plop, plop) == 0);

    printf("large_disperse_vers_large_disperse_disymetrique OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s]\n", entree0, (char *)iov[1].iov_base);
#endif

    free(buf1);
    free(buf2);
    free(recv_plop);

    free(test);
    free(entree0);
    free(ooo_msg3);

  } else {
    free(buf1);
    free(buf2);
    free(buf3);
  }
}
void court_disperse_vers_court_disperse_disymetrique(void){
  char *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  char *recv_plop = NULL;
  struct iovec iov[3];
  nm_so_request request0, request1;

  if (is_server()) {
    /* server */
    buf1 = malloc(strlen(msg1) + strlen(msg2));
    buf2 = malloc(1+strlen(msg3));

    memset(buf1, 0, strlen(msg1) + strlen(msg2));
    memset(buf2, 0, strlen(msg3)+1);

    iov[0].iov_len  = strlen(msg1) + strlen(msg2);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg3) + 1;
    iov[1].iov_base = buf2;

    recv_plop = malloc(strlen(plop)+1);
    memset(recv_plop, 0, strlen(plop)+1);

    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 2, &request1);
    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    /* client */
    buf1 = malloc(strlen(msg1));
    buf2 = malloc(strlen(msg2));
    buf3 = malloc(1+strlen(msg3));

    strncpy(buf1, msg1, strlen(msg1));
    strncpy(buf2, msg2, strlen(msg2));
    strcpy (buf3, msg3);

    iov[0].iov_len  = strlen(msg1);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg2);
    iov[1].iov_base = buf2;
    iov[2].iov_len  = strlen(msg3) + 1;
    iov[2].iov_base = buf3;

    nm_so_sr_isend_iov(sr_if, gate_id, 0, iov, 3, &request0);
    nm_so_sr_swait(sr_if, request0);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request1);
    nm_so_sr_swait(sr_if, request1);

    sleep(2);
  }

  if (is_server()) {
    char *test = malloc(strlen(msg1) + strlen(msg2)+1);
    strcpy(test, msg1);
    strcat(test, msg2);

    char *entree0 = malloc(iov[0].iov_len + 1);
    strncpy(entree0, (char *)iov[0].iov_base, iov[0].iov_len);
    entree0[iov[0].iov_len] = '\0';

    assert (strcmp (entree0, test) == 0);
    assert (strcmp ((char *)iov[1].iov_base, msg3) == 0);

    assert (strcmp (recv_plop, plop) == 0);

    printf("court_disperse_vers_court_disperse_disymetrique OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s]\n", entree0, (char *)iov[1].iov_base);
#endif

    free(buf1);
    free(buf2);
    free(recv_plop);

    free(test);
    free(entree0);

  } else {
    free(buf1);
    free(buf2);
    free(buf3);
  }
}

void large_disperse_vers_large_contigu(void){
  struct iovec iov[3];
  nm_so_request request0, request1;
  char *buf = NULL, *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  char *recv_plop = NULL;
  char *src, *dst;

  int msg3_len = SIZE - strlen(msg1) - strlen(msg2);

  if (is_server()) {
    /* server */
    buf = malloc(strlen(msg1) + strlen(msg2) + msg3_len);

    memset(buf, 0, strlen(msg1) + strlen(msg2) + msg3_len);

    iov[0].iov_len  = strlen(msg1) + strlen(msg2) + msg3_len;
    iov[0].iov_base = buf;

    recv_plop = malloc(strlen(plop)+1);
    memset(recv_plop, 0, strlen(plop)+1);

    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 1, &request1);
    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    /* client */
    buf1 = malloc(strlen(msg1));
    buf2 = malloc(strlen(msg2));
    buf3 = malloc(msg3_len);

    strcpy(buf1, msg1);
    strcpy(buf2, msg2);

    memset(buf3, '\0', msg3_len);
    memset(buf3, ' ' , msg3_len - 1);
    dst = buf3 + msg3_len - strlen(msg3) - 1;
    src = (char *) msg3;
    while(*src)
      *dst++ = *src++;

    iov[0].iov_len  = strlen(msg1);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg2);
    iov[1].iov_base = buf2;
    iov[2].iov_len  = msg3_len;
    iov[2].iov_base = buf3;

    nm_so_sr_isend_iov(sr_if, gate_id, 0, iov, 3, &request0);
    nm_so_sr_swait(sr_if, request0);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request1);
    nm_so_sr_swait(sr_if, request1);

    sleep(2);
  }

  if (is_server()) {
    char *test = malloc(strlen(msg1) + strlen(msg2) + msg3_len);

    memset(test, '\0', strlen(msg1) + strlen(msg2) + msg3_len);
    memset(test, ' ' , strlen(msg1) + strlen(msg2) + msg3_len - 1);

    dst = test;
    src = (char *) msg1;
    while(*src)
      *dst++ = *src++;

    dst = test+strlen(msg1);
    src = (char *)msg2;
    while(*src)
      *dst++ = *src++;

    dst = test + strlen(msg1) + strlen(msg2) + msg3_len - strlen(msg3) - 1;
    src = (char *) msg3;
    while(*src)
      *dst++ = *src++;

    char *entree0 = malloc(iov[0].iov_len + 1);
    strncpy(entree0, (char *)iov[0].iov_base, iov[0].iov_len);
    entree0[iov[0].iov_len] = '\0';
    assert (strcmp (entree0, test) == 0);

    assert (strcmp (recv_plop, plop) == 0);

    printf("large_disperse_vers_large_contigu OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s]\n", (char *)iov[0].iov_base);
#endif

    free(buf);
    free(recv_plop);

    free(test);
    free(entree0);

  } else {
    free(buf1);
    free(buf2);
    free(buf3);
  }
}

void court_disperse_vers_court_contigu(void){
  char *buf = NULL, *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  char *recv_plop = NULL;
  struct iovec iov[3];
  nm_so_request request0, request1;

  if (is_server()) {
    /* server */
    buf = malloc(strlen(msg1) + strlen(msg2) + strlen(msg3) + 1);

    memset(buf, 0, strlen(msg1) + strlen(msg2) + strlen(msg3) + 1);

    iov[0].iov_len  = strlen(msg1) + strlen(msg2) + strlen(msg3) + 1;
    iov[0].iov_base = buf;

    recv_plop = malloc(strlen(plop)+1);
    memset(recv_plop, 0, strlen(plop)+1);

    nm_so_sr_irecv(sr_if, gate_id, 1, recv_plop, strlen(plop)+1, &request0);

    int i = 0;
    while(i++ < 100000)
      nm_so_sr_rtest(sr_if, request0);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 1, &request1);
    nm_so_sr_rwait(sr_if, request1);

    nm_so_sr_rwait(sr_if, request0);

  } else {
    /* client */
    buf1 = malloc(strlen(msg1));
    buf2 = malloc(strlen(msg2));
    buf3 = malloc(1+strlen(msg3));

    strncpy(buf1, msg1, strlen(msg1));
    strncpy(buf2, msg2, strlen(msg2));
    strcpy (buf3, msg3);

    iov[0].iov_len  = strlen(msg1);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg2);
    iov[1].iov_base = buf2;
    iov[2].iov_len  = strlen(msg3) + 1;
    iov[2].iov_base = buf3;

    nm_so_sr_isend_iov(sr_if, gate_id, 0, iov, 3, &request0);
    nm_so_sr_swait(sr_if, request0);

    sleep(5);

    nm_so_sr_isend(sr_if, gate_id, 1, (void *)plop, strlen(plop)+1, &request1);
    nm_so_sr_swait(sr_if, request1);

    sleep(2);
  }

  if (is_server()) {
    char *test = malloc(strlen(msg1) + strlen(msg2) + strlen(msg3) + 1);

    strcpy(test, msg1);
    strcat(test, msg2);
    strcat(test, msg3);

    assert (strcmp ((char *)iov[0].iov_base, test) == 0);

    assert (strcmp (recv_plop, plop) == 0);

    printf("court_disperse_vers_court_contigu OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s]\n", (char *)iov[0].iov_base);
#endif

    free(buf);
    free(test);
    free(recv_plop);

  } else {
    free(buf1);
    free(buf2);
    free(buf3);
  }
}

int
main(int argc, char **argv) {
  int i = 0;

  nm_so_init(&argc, argv);
  nm_so_get_sr_if(&sr_if);

  if (is_server()) {
    nm_so_get_gate_in_id(1, &gate_id);
  }
  else {
    nm_so_get_gate_out_id(0, &gate_id);
  }

  while(i++ < 3){
    large_contigu_vers_large_contigu();
    court_contigu_vers_court_contigu();

    large_contigu_vers_large_contigu_plus_long();
    court_contigu_vers_court_contigu_plus_long();

    large_contigu_vers_large_disperse();
    court_contigu_vers_court_disperse();

    large_disperse_vers_large_disperse_identique();
    court_disperse_vers_court_disperse_identique();

    large_disperse_vers_large_disperse_disymetrique();
    court_disperse_vers_court_disperse_disymetrique();

    large_disperse_vers_large_contigu();
    court_disperse_vers_court_contigu();
  }

  nm_so_exit();
  exit(0);
}
