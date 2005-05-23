MPI_QS_DIR="/usr/lib/mpi"
PM2_CC=${MPI_QS_DIR}/bin/mpicc
PM2_PROTOCOLS="$PM2_PROTOCOLS mpi"
PM2_MAD3_LIBS="$PM2_MAD3_LIBS -L${MPI_QS_DIR}/lib"
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -I${MPI_QS_DIR}/include -DDRV_MPI=mad_MPI"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO=mad_MPI"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO_PARAM=NULL"
