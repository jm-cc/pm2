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


void large_contigu_vers_large_contigu(void){
  struct iovec iov[1];
  char * message = NULL;
  char *src, *dst;
  nm_sr_request_t request;

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

  if (is_server) {
    /* server */
    iov[0].iov_len  = SIZE;
    iov[0].iov_base = malloc(SIZE);
    if(!iov[0].iov_base){
      perror("malloc failed\n");
      exit(0);
    }
    memset(iov[0].iov_base, 0, SIZE);

    nm_sr_irecv_iov(p_core, gate_id, 0, iov, 1, &request);
    nm_sr_rwait(p_core, &request);

  } else {
    /* client */
    nm_sr_isend(p_core, gate_id, 0, message, SIZE, &request);
    nm_sr_swait(p_core, &request);

    usleep(50*1000);
  }

  if (is_server) {
    assert (strcmp ((char *)iov[0].iov_base, message) == 0);

    printf("large_contigu_vers_large_contigu OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s]\n", (char *)iov[0].iov_base);
#endif

    free(iov[0].iov_base);
  }
  free(message);
}

void large_contigu_vers_large_contigu_plus_long(void){
  struct iovec iov[1];
  char * message = NULL;
  char *src, *dst;
  nm_sr_request_t request;

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

  if (is_server) {
    /* server */
    iov[0].iov_len  = SIZE + 10;
    iov[0].iov_base = malloc(SIZE + 10);
    if(!iov[0].iov_base){
      perror("malloc failed\n");
      exit(0);
    }
    memset(iov[0].iov_base, 0, SIZE + 10);

    nm_sr_irecv_iov(p_core, gate_id, 0, iov, 1, &request);
    nm_sr_rwait(p_core, &request);

  } else {
    /* client */
    nm_sr_isend(p_core, gate_id, 0, message, SIZE, &request);
    nm_sr_swait(p_core, &request);

    usleep(50*1000);
  }

  if (is_server) {
    assert (strcmp ((char *)iov[0].iov_base, message) == 0);

    printf("large_contigu_vers_large_contigu_plus_long OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s]\n", (char *)iov[0].iov_base);
#endif

    free(iov[0].iov_base);
  }
  free(message);
}

void large_contigu_vers_large_disperse(void){
  struct iovec iov[3];
  char *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  char *message = NULL;
  char *src, *dst;
  int msg3_len = SIZE - strlen(msg1) - strlen(msg2);
  nm_sr_request_t request;

  if (is_server) {
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

    nm_sr_irecv_iov(p_core, gate_id, 0, iov, 3, &request);
    nm_sr_rwait(p_core, &request);

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

    nm_sr_isend(p_core, gate_id, 0, message, SIZE, &request);
    nm_sr_swait(p_core, &request);

    usleep(50*1000);
  }

  if (is_server) {
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

    printf("large_contigu_vers_large_disperse OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s][%s]\n", entree0, entree1, (char *)iov[2].iov_base);
#endif

    free(buf1);
    free(buf2);
    free(buf3);
    free(entree0);
    free(entree1);
    free(ooo_msg3);

  } else {
    free(message);
  }
}

void large_disperse_vers_large_disperse_identique(void){
  struct iovec iov[3];
  char *src, *dst;
  nm_sr_request_t request;

  int msg3_len = SIZE - strlen(msg1) - strlen(msg2);

  char *buf1 = malloc(strlen(msg1));
  char *buf2 = malloc(strlen(msg2));
  char *buf3 = malloc(msg3_len);

  if (is_server) {
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

    nm_sr_irecv_iov(p_core, gate_id, 0, iov, 3, &request);
    nm_sr_rwait(p_core, &request);

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

    nm_sr_isend_iov(p_core, gate_id, 0, iov, 3, &request);
    nm_sr_swait(p_core, &request);

    usleep(50*1000);
  }

  if (is_server) {
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

    printf("large_disperse_vers_large_disperse_identique OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s][%s]\n", entree0, entree1, (char *)iov[2].iov_base);
#endif

    free(entree0);
    free(entree1);
    free(ooo_msg3);
  }

  free(buf1);
  free(buf2);
  free(buf3);
}

void large_disperse_vers_large_disperse_disymetrique(void){
  struct iovec iov[3];
  nm_sr_request_t request;
  char *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  char *src, *dst;

  int msg3_len = SIZE - strlen(msg1) - strlen(msg2);

  if (is_server) {
    /* server */
    buf1 = malloc(strlen(msg1) + strlen(msg2));
    buf2 = malloc(msg3_len);

    memset(buf1, 0, strlen(msg1) + strlen(msg2));
    memset(buf2, 0, msg3_len);

    iov[0].iov_len  = strlen(msg1) + strlen(msg2);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = msg3_len;
    iov[1].iov_base = buf2;

    nm_sr_irecv_iov(p_core, gate_id, 0, iov, 2, &request);
    nm_sr_rwait(p_core, &request);

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

    nm_sr_isend_iov(p_core, gate_id, 0, iov, 3, &request);
    nm_sr_swait(p_core, &request);

    usleep(50*1000);
  }

  if (is_server) {
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

    printf("large_disperse_vers_large_disperse_disymetrique OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s]\n", entree0, (char *)iov[1].iov_base);
#endif

    free(buf1);
    free(buf2);

    free(test);
    free(entree0);
    free(ooo_msg3);

  } else {
    free(buf1);
    free(buf2);
    free(buf3);
  }
}

void large_disperse_vers_large_contigu(void){
  struct iovec iov[3];
  nm_sr_request_t request;
  char *buf = NULL, *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  char *src, *dst;

  int msg3_len = SIZE - strlen(msg1) - strlen(msg2);

  if (is_server) {
    /* server */
    buf = malloc(strlen(msg1) + strlen(msg2) + msg3_len);

    memset(buf, 0, strlen(msg1) + strlen(msg2) + msg3_len);

    iov[0].iov_len  = strlen(msg1) + strlen(msg2) + msg3_len;
    iov[0].iov_base = buf;

    nm_sr_irecv_iov(p_core, gate_id, 0, iov, 1, &request);
    nm_sr_rwait(p_core, &request);

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

    nm_sr_isend_iov(p_core, gate_id, 0, iov, 3, &request);
    nm_sr_swait(p_core, &request);

    usleep(50*1000);
  }

  if (is_server) {
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

    printf("large_disperse_vers_large_contigu OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s]\n", (char *)iov[0].iov_base);
#endif

    free(buf);

    free(test);
    free(entree0);

  } else {
    free(buf1);
    free(buf2);
    free(buf3);
  }
}

int
main(int argc, char **argv) {
  int i = 0;

  init(&argc, argv);

  while(i++ < 3){
    large_contigu_vers_large_contigu();
    large_contigu_vers_large_contigu_plus_long();
    large_contigu_vers_large_disperse();
    large_disperse_vers_large_disperse_identique();
    large_disperse_vers_large_disperse_disymetrique();
    large_disperse_vers_large_contigu();
  }

  nmad_exit();
  exit(0);
}
