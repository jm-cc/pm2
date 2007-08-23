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

/*
 * arg.c
 * =====
 */

/**
 * These functions must be defined if the flavor has been set with the
 * Intel Fortran compiler. When compiling with mpif77 or mpif90, the
 * functions will be defined in the libraries of the Intel Fortran
 * compiler, when compiling with mpicc, they must be defined
 * otherwise.
 */

#if defined PM2_FORTRAN_TARGET_IFORT
int iargc_() {
  return 0;
}
void getarg_(int* x, char* y, int z) {
}
#endif /* PM2_FORTRAN_TARGET_IFORT */
