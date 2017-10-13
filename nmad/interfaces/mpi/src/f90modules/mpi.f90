        MODULE MPI
!       This module was created by the script CreateChoiceSrc
        USE MPI_CONSTANTS,                                               &
     &      BASE_MPI_WTIME => MPI_WTIME, BASE_MPI_WTICK => MPI_WTICK
!
        USE MPI1
        USE MPI2
        USE ISO_C_BINDING
        INTEGER(C_INT),BIND(C,NAME="f90_mpi_status_ignore") :: MPI_STATUS_IGNORE(MPI_STATUS_SIZE)=0
        INTEGER(C_INT),BIND(C,NAME="f90_mpi_statuses_ignore") :: MPI_STATUSES_IGNORE(MPI_STATUS_SIZE,1)=0
        INTEGER(C_INT),BIND(C,NAME="f90_mpi_in_place") :: MPI_IN_PLACE
        INTEGER(C_INT),BIND(C,NAME="f90_mpi_bottom") :: MPI_BOTTOM=0
        END MODULE MPI
