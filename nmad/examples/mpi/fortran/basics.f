      program main

      include 'mpif.h'

      double precision  PI25DT
      parameter        (PI25DT = 3.141592653589793238462643d0)

      double precision  pi
      integer myid, numprocs, rc

      call MPI_INIT( ierr )
      call MPI_COMM_RANK( MPI_COMM_WORLD, myid, ierr )
      call MPI_COMM_SIZE( MPI_COMM_WORLD, numprocs, ierr )
      print *, "Process ", myid, " of ", numprocs, " is alive"

      if (myid .eq. 0) then
         call MPI_SEND(PI25DT,1,MPI_DOUBLE_PRECISION,1,10,
     $        MPI_COMM_WORLD,ierr)
      else
         call MPI_RECV(pi,1,MPI_DOUBLE_PRECISION,0,10,
     $        MPI_COMM_WORLD,NULL,ierr)
         if (pi .eq. PI25DT) then
            write(6, 97) pi
 97         format('  received value: ', E30.20)
            write(6, 98) PI25DT
 98         format('   correct value: ', E30.20)
         else
            write(6,97) "error"
         endif
      endif

      call MPI_FINALIZE(rc)
      stop
      end




