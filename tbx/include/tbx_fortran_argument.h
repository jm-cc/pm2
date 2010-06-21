/*! \file tbx_fortran_argument.h
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
 * Tbx_fortran_argument.h :
 * ============-------------------------------
 */

#ifndef TBX_FORTRAN_ARGUMENT_H
#define TBX_FORTRAN_ARGUMENT_H

/** \defgroup fortran_interface compiler dependent Fortran interface
 *
 * This is the compiler dependent Fortran interface
 *
 * @{
 */

void tbx_fortran_init(int *argc, char ***argv);

/* @} */

#endif				/* TBX_FORTRAN_ARGUMENT_H */
