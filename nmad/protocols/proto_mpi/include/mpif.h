!
! NewMadeleine
! Copyright (C) 2006 (see AUTHORS file)
!
! This program is free software; you can redistribute it and/or modify
! it under the terms of the GNU General Public License as published by
! the Free Software Foundation; either version 2 of the License, or (at
! your option) any later version.
!
! This program is distributed in the hope that it will be useful, but
! WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
! General Public License for more details.
!

!
! mpif.h
! =====
!

!
! Types
      INTEGER MPI_CHARACTER
      INTEGER MPI_COMPLEX
      INTEGER MPI_DOUBLE_COMPLEX
      INTEGER MPI_LOGICAL
      INTEGER MPI_REAL
      INTEGER MPI_DOUBLE_PRECISION
      INTEGER MPI_INTEGER

      PARAMETER (MPI_CHARACTER=1)
      PARAMETER (MPI_COMPLEX=23)
      PARAMETER (MPI_DOUBLE_COMPLEX=24)
      PARAMETER (MPI_LOGICAL=25)
      PARAMETER (MPI_REAL=26)
      PARAMETER (MPI_DOUBLE_PRECISION=27)
      PARAMETER (MPI_INTEGER=28)

!
! Abstract types
      INTEGER MPI_STATUS_SIZE

      PARAMETER (MPI_STATUS_SIZE=4)

!
! Communicators
      INTEGER MPI_COMM_WORLD

      PARAMETER (MPI_COMM_WORLD=91)

!
! Error codes
      INTEGER MPI_SUCCESS
      INTEGER MPI_ERR_OTHER

      PARAMETER (MPI_SUCCESS=0)
      PARAMETER (MPI_ERR_OTHER=2)

!
! Communications
      INTEGER MPI_ANY_SOURCE
      PARAMETER (MPI_ANY_SOURCE=-2)

!
! Operators
      INTEGER MPI_MAX
      INTEGER MPI_MIN
      INTEGER MPI_SUM

      PARAMETER (MPI_MAX=100)
      PARAMETER (MPI_MIN=101)
      PARAMETER (MPI_SUM=102)

!
! Timing routines
      DOUBLE PRECISION MPI_WTIME
      DOUBLE PRECISION MPI_WTICK

      EXTERNAL MPI_WTIME
      EXTERNAL MPI_WTICK

!     There are additional MPI-2 constants 
      INTEGER MPI_ADDRESS_KIND, MPI_OFFSET_KIND
      PARAMETER (MPI_ADDRESS_KIND=4)
      PARAMETER (MPI_OFFSET_KIND=8)
