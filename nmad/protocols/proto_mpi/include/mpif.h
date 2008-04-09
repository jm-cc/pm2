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
      INTEGER MPI_REAL4
      INTEGER MPI_REAL8
      INTEGER MPI_DOUBLE_PRECISION
      INTEGER MPI_INTEGER
      INTEGER MPI_PACKED

      PARAMETER (MPI_CHARACTER=1)
      PARAMETER (MPI_COMPLEX=23)
      PARAMETER (MPI_DOUBLE_COMPLEX=24)
      PARAMETER (MPI_LOGICAL=25)
      PARAMETER (MPI_REAL=26)
      PARAMETER (MPI_REAL4=27)
      PARAMETER (MPI_REAL8=28)
      PARAMETER (MPI_DOUBLE_PRECISION=29)
      PARAMETER (MPI_INTEGER=30)
      PARAMETER (MPI_PACKED=31)

!
! Request accessors
      INTEGER MPI_SOURCE
      INTEGER MPI_TAG
      INTEGER MPI_ERROR

      PARAMETER (MPI_SOURCE=1)
      PARAMETER (MPI_TAG=2)
      PARAMETER (MPI_ERROR=3)

!
! Abstract types
      INTEGER MPI_STATUS_SIZE

      PARAMETER (MPI_STATUS_SIZE=4)

      INTEGER MPI_REQUEST_SIZE

      PARAMETER (MPI_REQUEST_SIZE=4)

!
! Communicators
      INTEGER MPI_COMM_WORLD

      PARAMETER (MPI_COMM_WORLD=91)

!
! Communications
      INTEGER MPI_ANY_SOURCE
      PARAMETER (MPI_ANY_SOURCE=-2)

      INTEGER MPI_ANY_TAG
      PARAMETER (MPI_ANY_TAG=-1)

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

!
! Status
      INTEGER MPI_SUCCESS
      INTEGER MPI_ERR_BUFFER
      INTEGER MPI_ERR_COUNT
      INTEGER MPI_ERR_TYPE
      INTEGER MPI_ERR_TAG
      INTEGER MPI_ERR_COMM
      INTEGER MPI_ERR_RANK
      INTEGER MPI_ERR_ROOT
      INTEGER MPI_ERR_GROUP
      INTEGER MPI_ERR_OP
      INTEGER MPI_ERR_TOPOLOGY
      INTEGER MPI_ERR_DIMS
      INTEGER MPI_ERR_ARG
      INTEGER MPI_ERR_UNKNOWN
      INTEGER MPI_ERR_TRUNCATE
      INTEGER MPI_ERR_OTHER
      INTEGER MPI_ERR_INTERN
      INTEGER MPI_ERR_IN_STATUS
      INTEGER MPI_ERR_PENDING
      INTEGER MPI_ERR_REQUEST
      INTEGER MPI_ERR_LASTCODE

      PARAMETER (MPI_SUCCESS=0)
      PARAMETER (MPI_ERR_BUFFER=1)
      PARAMETER (MPI_ERR_COUNT=2)
      PARAMETER (MPI_ERR_TYPE=3)
      PARAMETER (MPI_ERR_TAG=4)
      PARAMETER (MPI_ERR_COMM=5)
      PARAMETER (MPI_ERR_RANK=6)
      PARAMETER (MPI_ERR_ROOT=7)
      PARAMETER (MPI_ERR_GROUP=8)
      PARAMETER (MPI_ERR_OP=9)
      PARAMETER (MPI_ERR_TOPOLOGY=10)
      PARAMETER (MPI_ERR_DIMS=11)
      PARAMETER (MPI_ERR_ARG=12)
      PARAMETER (MPI_ERR_UNKNOWN=13)
      PARAMETER (MPI_ERR_TRUNCATE=14)
      PARAMETER (MPI_ERR_OTHER=15)
      PARAMETER (MPI_ERR_INTERN=16)
      PARAMETER (MPI_ERR_IN_STATUS=17)
      PARAMETER (MPI_ERR_PENDING=18)
      PARAMETER (MPI_ERR_REQUEST=19)
      PARAMETER (MPI_ERR_LASTCODE=1073741823)

!     There are additional MPI-2 constants 
      INTEGER MPI_ADDRESS_KIND, MPI_OFFSET_KIND
      PARAMETER (MPI_ADDRESS_KIND=4)
      PARAMETER (MPI_OFFSET_KIND=8)

