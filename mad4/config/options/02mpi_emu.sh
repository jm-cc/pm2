PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -DCONFIG_PROTO_MPI -DMAD_MPI"
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -I${PM2_ROOT}/nmad/protocols/proto_mpi/include"

if [ "$PM2_COMMON_FORTRAN_TARGET" != "none" ] ; then
    PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -I${PM2_ROOT}/nmad/protocols/proto_mpi/include/f90base"
fi

PM2_NMAD_PROTO="$PM2_NMAD_PROTO proto_mpi"

PM2_NMAD_LAUNCHER="leonie"

