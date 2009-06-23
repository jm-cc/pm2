/*! \file tbx_fortran_argument.c
 *  \brief TBX argument management routines for Fortran
 *
 *  This file implements routines to manage arguments in Fortran
 *  applications.
 *
 */

/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/*
 * tbx_fortran_argument.c
 * -----------------------
 */

#include "tbx.h"

#define MAX_ARG_LEN 128

#if defined PM2_FORTRAN_TARGET_GFORTRAN
/* GFortran iargc/getargc bindings
 */
void _gfortran_getarg_i4(int32_t *num, char *value, int val_len)	__attribute__ ((weak));

void _gfortran_getarg_i8(int64_t *num, char *value, int val_len)	__attribute__ ((weak));

int32_t _gfortran_iargc(void)	__attribute__ ((weak));

/** Initialisation by Fortran code.
 */
void tbx_fortran_init(int *argc, char ***argv) {
  int i;

  *argc = 1+_gfortran_iargc();
  //  fprintf(stderr,"argc = %d\n", *argc);
  *argv = malloc(*argc * sizeof(char *));

  for (i = 0; i < *argc; i++) {
    int j;

    (*argv)[i] = malloc(MAX_ARG_LEN+1);
    if (sizeof(char*) == 4) {
      int32_t ii = i;
      _gfortran_getarg_i4(&ii, (*argv)[i], MAX_ARG_LEN);
    }
    else {
      int64_t ii = i;
      _gfortran_getarg_i8(&ii, (*argv)[i], MAX_ARG_LEN);
    }
    j = MAX_ARG_LEN;
    while (j > 1 && ((*argv)[i])[j-1] == ' ') {
      j--;
    }
    ((*argv)[i])[j] = '\0';
  }
//  for (i = 0; i < *argc; i++) {
//    fprintf(stderr,"argv[%d] = [%s]\n", i, (*argv)[i]);
//  }
}

#elif defined PM2_FORTRAN_TARGET_IFORT
/* Ifort iargc/getargc bindings
 */
extern int  iargc_();
extern void getarg_(int*, char*, int);

void tbx_fortran_init(int *argc, char ***argv) {
  int i;

  *argc = 1+iargc_();
  fprintf(stderr,"argc = %d\n", *argc);
  *argv = malloc(*argc * sizeof(char *));
  for (i = 0; i < *argc; i++) {
    int j;

    (*argv)[i] = malloc(MAX_ARG_LEN+1);
    getarg_((int32_t *)&i, (*argv)[i], MAX_ARG_LEN);

    j = MAX_ARG_LEN;
    while (j > 1 && ((*argv)[i])[j-1] == ' ') {
      j--;
    }
    ((*argv)[i])[j] = '\0';
  }
  for (i = 0; i < *argc; i++) {
    fprintf(stderr,"argv[%d] = [%s]\n", i, (*argv)[i]);
  }
}
//#elif defined PM2_FORTRAN_TARGET_NONE
///* Nothing */
#else
void tbx_fortran_init(int *argc TBX_UNUSED, char ***argv TBX_UNUSED) {
  abort();
}
//#  error unknown FORTRAN TARGET
#endif
