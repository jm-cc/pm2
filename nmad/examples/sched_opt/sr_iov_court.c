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

const char *msg1 = "hello";
const char *msg2 = ", ";
const char *msg3 = "world";


void court_contigu_vers_court_contigu(void){
  char *buf  = "hello, world";
  struct iovec iov[1];
  nm_so_request request;

  if (is_server) {
    /* server */
    iov[0].iov_len  = strlen(buf)+1;
    iov[0].iov_base = malloc(strlen(buf)+1);

    memset(iov[0].iov_base, 0, strlen(buf)+1);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 1, &request);
    nm_so_sr_rwait(sr_if, request);

  } else {
    /* client */
    nm_so_sr_isend(sr_if, gate_id, 0, buf, 1+strlen(buf), &request);
    nm_so_sr_swait(sr_if, request);
  }

  if (is_server) {
    assert (strcmp ((char *)iov[0].iov_base, buf) == 0);

    printf("court_contigu_vers_court_contigu OK\n");
#if PRINT_CONTENT
    printf("buffers contents: [%s]\n", (char *)iov[0].iov_base);
#endif

    free(iov[0].iov_base);
  }
}

void court_contigu_vers_court_contigu_plus_long(void){
  char *buf  = "hello, world";
  struct iovec iov[1];
  nm_so_request request;

  if (is_server) {
    /* server */
    iov[0].iov_len  = strlen(buf) + 10;
    iov[0].iov_base = malloc(strlen(buf)+10);

    memset(iov[0].iov_base, 0, strlen(buf)+10);

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 1, &request);
    nm_so_sr_rwait(sr_if, request);

  } else {
    /* client */
    nm_so_sr_isend(sr_if, gate_id, 0, buf, 1+strlen(buf), &request);
    nm_so_sr_swait(sr_if, request);
  }

  if (is_server) {
    assert (strcmp ((char *)iov[0].iov_base, buf) == 0);

    printf("court_contigu_vers_court_contigu_plus_long OK\n");
#if PRINT_CONTENT
    printf("buffers contents: [%s]\n", (char *)iov[0].iov_base);
#endif

    free(iov[0].iov_base);
  }
}

void court_contigu_vers_court_disperse(void){
  char *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  struct iovec iov[3];
  nm_so_request request;

  if (is_server) {
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

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 3, &request);
    nm_so_sr_rwait(sr_if, request);

  } else {
    /* client */
    char *buf  = "hello, world";

    nm_so_sr_isend(sr_if, gate_id, 0, buf, 1+strlen(buf), &request);
    nm_so_sr_swait(sr_if, request);
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

    assert (strcmp ((char *)iov[2].iov_base, msg3) == 0);

    printf("court_contigu_vers_court_disperse OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s][%s]\n", entree0, entree1, (char *)iov[2].iov_base);
#endif

    free(buf1);
    free(buf2);
    free(buf3);

    free(entree0);
    free(entree1);
  }
}

void court_disperse_vers_court_disperse_identique(void){
  struct iovec iov[3];
  nm_so_request request;

  char *buf1 = malloc(strlen(msg1));
  char *buf2 = malloc(strlen(msg2));
  char *buf3 = malloc(1+strlen(msg3));

  if (is_server) {
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

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 3, &request);
    nm_so_sr_rwait(sr_if, request);

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

    nm_so_sr_isend_iov(sr_if, gate_id, 0, iov, 3, &request);
    nm_so_sr_swait(sr_if, request);
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

    assert (strcmp ((char *)iov[2].iov_base, msg3) == 0);

    printf("court_disperse_vers_court_disperse_identique OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s][%s]\n", entree0, entree1, (char *)iov[2].iov_base);
#endif

    free(entree0);
    free(entree1);
  }

  free(buf1);
  free(buf2);
  free(buf3);
}

void court_disperse_vers_court_disperse_disymetrique(void){
  char *buf1 = NULL, *buf2 = NULL, *buf3 = NULL;
  struct iovec iov[3];
  nm_so_request request;

  if (is_server) {
    /* server */
    buf1 = malloc(strlen(msg1) + strlen(msg2));
    buf2 = malloc(1+strlen(msg3));

    memset(buf1, 0, strlen(msg1) + strlen(msg2));
    memset(buf2, 0, strlen(msg3)+1);

    iov[0].iov_len  = strlen(msg1) + strlen(msg2);
    iov[0].iov_base = buf1;
    iov[1].iov_len  = strlen(msg3) + 1;
    iov[1].iov_base = buf2;

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 2, &request);
    nm_so_sr_rwait(sr_if, request);

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

    nm_so_sr_isend_iov(sr_if, gate_id, 0, iov, 3, &request);
    nm_so_sr_swait(sr_if, request);
  }

  if (is_server) {
    char *test = malloc(strlen(msg1) + strlen(msg2)+1);
    strcpy(test, msg1);
    strcat(test, msg2);

    char *entree0 = malloc(iov[0].iov_len + 1);
    strncpy(entree0, (char *)iov[0].iov_base, iov[0].iov_len);
    entree0[iov[0].iov_len] = '\0';

    assert (strcmp (entree0, test) == 0);
    assert (strcmp ((char *)iov[1].iov_base, msg3) == 0);

    printf("court_disperse_vers_court_disperse_disymetrique OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s][%s]\n", entree0, (char *)iov[1].iov_base);
#endif

    free(buf1);
    free(buf2);

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
  struct iovec iov[3];
  nm_so_request request;

  if (is_server) {
    /* server */
    buf = malloc(strlen(msg1) + strlen(msg2) + strlen(msg3) + 1);

    memset(buf, 0, strlen(msg1) + strlen(msg2) + strlen(msg3) + 1);

    iov[0].iov_len  = strlen(msg1) + strlen(msg2) + strlen(msg3) + 1;
    iov[0].iov_base = buf;

    nm_so_sr_irecv_iov(sr_if, gate_id, 0, iov, 1, &request);
    nm_so_sr_rwait(sr_if, request);

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

    nm_so_sr_isend_iov(sr_if, gate_id, 0, iov, 3, &request);
    nm_so_sr_swait(sr_if, request);
  }

  if (is_server) {
    char *test = malloc(strlen(msg1) + strlen(msg2) + strlen(msg3) + 1);

    strcpy(test, msg1);
    strcat(test, msg2);
    strcat(test, msg3);

    assert (strcmp ((char *)iov[0].iov_base, test) == 0);

    printf("court_disperse_vers_court_contigu OK\n");
#if PRINT_CONTENT
    printf("buffers content: [%s]\n", (char *)iov[0].iov_base);
#endif

    free(buf);
    free(test);

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

  while(i++ < 100){
    court_contigu_vers_court_contigu();
    court_contigu_vers_court_contigu_plus_long();
    court_contigu_vers_court_disperse();
    court_disperse_vers_court_disperse_identique();
    court_disperse_vers_court_disperse_disymetrique();
    court_disperse_vers_court_contigu();
  }

  nmad_exit();
  exit(0);
}
