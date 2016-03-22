! -*-Fortran-*-
!     
! NewMadeleine
! Copyright (C) 2006-2014 (see AUTHORS file)
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
! MPI_Datatype
! for consistency, C and FORTRAN types are defined
      INTEGER MPI_CHAR
      PARAMETER (MPI_CHAR=1)
      INTEGER MPI_UNSIGNED_CHAR
      PARAMETER (MPI_UNSIGNED_CHAR=2)
      INTEGER MPI_SIGNED_CHAR
      PARAMETER (MPI_SIGNED_CHAR=3)
      INTEGER MPI_BYTE
      PARAMETER (MPI_BYTE=4)
      INTEGER MPI_SHORT
      PARAMETER (MPI_SHORT=5)
      INTEGER MPI_UNSIGNED_SHORT
      PARAMETER (MPI_UNSIGNED_SHORT=6)
      INTEGER MPI_INT
      PARAMETER (MPI_INT=7)
      INTEGER MPI_UNSIGNED
      PARAMETER (MPI_UNSIGNED=8)
      INTEGER MPI_LONG
      PARAMETER (MPI_LONG=9)
      INTEGER MPI_UNSIGNED_LONG
      PARAMETER (MPI_UNSIGNED_LONG=10)
      INTEGER MPI_FLOAT
      PARAMETER (MPI_FLOAT=11)
      INTEGER MPI_DOUBLE
      PARAMETER (MPI_DOUBLE=12)
      INTEGER MPI_LONG_DOUBLE
      PARAMETER (MPI_LONG_DOUBLE=13)
      INTEGER MPI_LONG_LONG_INT
      PARAMETER (MPI_LONG_LONG_INT=14)
      INTEGER MPI_UNSIGNED_LONG_LONG
      PARAMETER (MPI_UNSIGNED_LONG_LONG=15)

      INTEGER MPI_CHARACTER
      PARAMETER (MPI_CHARACTER=16)
      INTEGER MPI_LOGICAL
      PARAMETER (MPI_LOGICAL=17)
      INTEGER MPI_REAL
      PARAMETER (MPI_REAL=18)
      INTEGER MPI_REAL4
      PARAMETER (MPI_REAL4=19)
      INTEGER MPI_REAL8
      PARAMETER (MPI_REAL8=20)
      INTEGER MPI_REAL16
      PARAMETER (MPI_REAL16=21)
      INTEGER MPI_DOUBLE_PRECISION
      PARAMETER (MPI_DOUBLE_PRECISION=22)
      INTEGER MPI_INTEGER
      PARAMETER (MPI_INTEGER=23)
      INTEGER MPI_INTEGER1
      PARAMETER (MPI_INTEGER1=24)
      INTEGER MPI_INTEGER2
      PARAMETER (MPI_INTEGER2=25)
      INTEGER MPI_INTEGER4
      PARAMETER (MPI_INTEGER4=26)
      INTEGER MPI_INTEGER8
      PARAMETER (MPI_INTEGER8=27)
      INTEGER MPI_COMPLEX
      PARAMETER (MPI_COMPLEX=28)
      INTEGER MPI_DOUBLE_COMPLEX
      PARAMETER (MPI_DOUBLE_COMPLEX=29)
      INTEGER MPI_PACKED
      PARAMETER (MPI_PACKED=30)

      INTEGER MPI_LONG_INT
      PARAMETER (MPI_LONG_INT=31)
      INTEGER MPI_SHORT_INT
      PARAMETER (MPI_SHORT_INT=32)
      INTEGER MPI_FLOAT_INT
      PARAMETER (MPI_FLOAT_INT=33)
      INTEGER MPI_DOUBLE_INT
      PARAMETER (MPI_DOUBLE_INT=34)
      INTEGER MPI_LONG_DOUBLE_INT
      PARAMETER (MPI_LONG_DOUBLE_INT=35)

      INTEGER MPI_2INT
      PARAMETER (MPI_2INT=36)
      INTEGER MPI_2INTEGER
      PARAMETER (MPI_2INTEGER=37)
      INTEGER MPI_2REAL
      PARAMETER (MPI_2REAL=38)
      INTEGER MPI_2DOUBLE_PRECISION
      PARAMETER (MPI_2DOUBLE_PRECISION=39)
      INTEGER MPI_UB
      PARAMETER (MPI_UB=40)
      INTEGER MPI_LB
      PARAMETER (MPI_LB=41)



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

      INTEGER MPI_STATUS_IGNORE
      PARAMETER (MPI_STATUS_IGNORE=0)

      INTEGER MPI_REQUEST_SIZE
      PARAMETER (MPI_REQUEST_SIZE=4)

!
! Communicators
      INTEGER MPI_COMM_NULL
      INTEGER MPI_COMM_WORLD
      INTEGER MPI_COMM_SELF
      
      PARAMETER (MPI_COMM_NULL=0)
      PARAMETER (MPI_COMM_WORLD=1)
      PARAMETER (MPI_COMM_SELF=2)

!
! Communications
      INTEGER MPI_ANY_SOURCE
      PARAMETER (MPI_ANY_SOURCE=-2)

      INTEGER MPI_ANY_TAG
      PARAMETER (MPI_ANY_TAG=-1)

!
! Comparison
      INTEGER MPI_IDENT, MPI_SIMILAR, MPI_CONGRUENT, MPI_UNEQUAL
      PARAMETER (MPI_IDENT=1)
      PARAMETER (MPI_SIMILAR=2)
      PARAMETER (MPI_CONGRUENT=3)
      PARAMETER (MPI_UNEQUAL=4)

!
! Operators
      INTEGER MPI_MAX
      INTEGER MPI_MIN
      INTEGER MPI_SUM
      INTEGER MPI_PROD
      INTEGER MPI_LAND
      INTEGER MPI_BAND
      INTEGER MPI_LOR
      INTEGER MPI_BOR
      INTEGER MPI_LXOR
      INTEGER MPI_BXOR
      INTEGER MPI_MINLOC
      INTEGER MPI_MAXLOC

      PARAMETER (MPI_MAX=1)
      PARAMETER (MPI_MIN=2)
      PARAMETER (MPI_SUM=3)
      PARAMETER (MPI_PROD=4)
      PARAMETER (MPI_LAND=5)
      PARAMETER (MPI_BAND=6)
      PARAMETER (MPI_LOR=7)
      PARAMETER (MPI_BOR=8)
      PARAMETER (MPI_LXOR=9)
      PARAMETER (MPI_BXOR=10)
      PARAMETER (MPI_MINLOC=11)
      PARAMETER (MPI_MAXLOC=12)

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
      INTEGER MPI_ERR_INFO
      INTEGER MPI_ERR_REQUEST
      INTEGER MPI_ERR_KEYVAL
      INTEGER MPI_ERR_DATATYPE_ACTIVE
      INTEGER MPI_ERR_INFO_NOKEY
      INTEGER MPI_ERR_FILE
      INTEGER MPI_ERR_IO
      INTEGER MPI_ERR_AMODE
      INTEGER MPI_ERR_UNSUPPORTED_OPERATION
      INTEGER MPI_ERR_UNSUPPORTED_DATAREP
      INTEGER MPI_ERR_READ_ONLY
      INTEGER MPI_ERR_ACCESS
      INTEGER MPI_ERR_DUP_DATAREP
      INTEGER MPI_ERR_NO_SUCH_FILE
      INTEGER MPI_ERR_NOT_SAME
      INTEGER MPI_ERR_BAD_FILE
      INTEGER MPI_ERR_LASTCODE

      PARAMETER (MPI_SUCCESS             =   0)
      PARAMETER (MPI_ERR_BUFFER          =   1)
      PARAMETER (MPI_ERR_COUNT           =   2)
      PARAMETER (MPI_ERR_TYPE            =   3)
      PARAMETER (MPI_ERR_TAG             =   4)
      PARAMETER (MPI_ERR_COMM            =   5)
      PARAMETER (MPI_ERR_RANK            =   6)
      PARAMETER (MPI_ERR_ROOT            =   7)
      PARAMETER (MPI_ERR_GROUP           =   8)
      PARAMETER (MPI_ERR_OP              =   9)
      PARAMETER (MPI_ERR_TOPOLOGY        =  10)
      PARAMETER (MPI_ERR_DIMS            =  11)
      PARAMETER (MPI_ERR_ARG             =  12)
      PARAMETER (MPI_ERR_UNKNOWN         =  13)
      PARAMETER (MPI_ERR_TRUNCATE        =  14)
      PARAMETER (MPI_ERR_OTHER           =  15)
      PARAMETER (MPI_ERR_INTERN          =  16)
      PARAMETER (MPI_ERR_IN_STATUS       =  17)
      PARAMETER (MPI_ERR_PENDING         =  18)
      PARAMETER (MPI_ERR_INFO            =  19)
      PARAMETER (MPI_ERR_REQUEST         =  32)
      PARAMETER (MPI_ERR_KEYVAL          =  33)
      PARAMETER (MPI_ERR_DATATYPE_ACTIVE =  34)
      PARAMETER (MPI_ERR_INFO_NOKEY      =  35)
      PARAMETER (MPI_ERR_FILE            =  64)
      PARAMETER (MPI_ERR_IO              =  65)
      PARAMETER (MPI_ERR_AMODE           =  66)
      PARAMETER (MPI_ERR_UNSUPPORTED_OPERATION = 67)
      PARAMETER (MPI_ERR_UNSUPPORTED_DATAREP = 68)
      PARAMETER (MPI_ERR_READ_ONLY       =  69)
      PARAMETER (MPI_ERR_ACCESS          =  70)
      PARAMETER (MPI_ERR_DUP_DATAREP     =  71)
      PARAMETER (MPI_ERR_NO_SUCH_FILE    =  72)
      PARAMETER (MPI_ERR_NOT_SAME        =  73)
      PARAMETER (MPI_ERR_BAD_FILE        =  74)
      PARAMETER (MPI_ERR_LASTCODE        = 128)

!
!     Error handlers
      
      INTEGER MPI_ERRORS_ARE_FATAL
      INTEGER MPI_ERRORS_RETURN

      PARAMETER (MPI_ERRORS_ARE_FATAL=1)
      PARAMETER (MPI_ERRORS_RETURN=2)

!
!     Error handlers
      
      INTEGER MPI_THREAD_SINGLE
      INTEGER MPI_THREAD_FUNNELED
      INTEGER MPI_THREAD_SERIALIZED
      INTEGER MPI_THREAD_MULTIPLE

      PARAMETER (MPI_THREAD_SINGLE=0)
      PARAMETER (MPI_THREAD_FUNNELED=1)
      PARAMETER (MPI_THREAD_SERIALIZED=2)
      PARAMETER (MPI_THREAD_MULTIPLE=3)

!
!     Info
      INTEGER MPI_INFO_NULL
      PARAMETER (MPI_INFO_NULL=0)

!     
!     String sizes
      
      INTEGER MPI_MAX_PROCESSOR_NAME
      INTEGER MPI_MAX_ERROR_STRING
      INTEGER MPI_MAX_NAME_STRING

      PARAMETER (MPI_MAX_PROCESSOR_NAME=256)
      PARAMETER (MPI_MAX_ERROR_STRING=512)
      PARAMETER (MPI_MAX_NAME_STRING=256)

!     
!     Communicator attributes

      INTEGER MPI_TAG_UB
      INTEGER MPI_HOST
      INTEGER MPI_IO
      INTEGER MPI_WTIME_IS_GLOBAL
      INTEGER MPI_UNIVERSE_SIZE

      PARAMETER (MPI_TAG_UB=1)
      PARAMETER (MPI_HOST=2)
      PARAMETER (MPI_IO=3)
      PARAMETER (MPI_WTIME_IS_GLOBAL=4)
      PARAMETER (MPI_UNIVERSE_SIZE=5)


!     There are additional MPI-2 constants 
      INTEGER MPI_ADDRESS_KIND, MPI_OFFSET_KIND
      PARAMETER (MPI_ADDRESS_KIND=4)
      PARAMETER (MPI_OFFSET_KIND=8)

      INTEGER MPI_UNDEFINED
      PARAMETER (MPI_UNDEFINED=-32766)

!     MPI I/O

      INTEGER MPI_FILE_NULL
      PARAMETER (MPI_FILE_NULL=0)

      INTEGER MPI_MODE_RDONLY
      INTEGER MPI_MODE_RDWR
      INTEGER MPI_MODE_WRONLY
      INTEGER MPI_MODE_CREATE
      INTEGER MPI_MODE_EXCL
      INTEGER MPI_MODE_APPEND
      PARAMETER (MPI_MODE_RDONLY = Z'0001')
      PARAMETER (MPI_MODE_RDWR   = Z'0002')
      PARAMETER (MPI_MODE_WRONLY = Z'0004')
      PARAMETER (MPI_MODE_CREATE = Z'0008')
      PARAMETER (MPI_MODE_EXCL   = Z'0010')
      PARAMETER (MPI_MODE_APPEND = Z'0020')
            

      

      
