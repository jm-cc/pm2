#!/bin/sh
##########

if [ ! -f ${MAD2_ROOT}/.mad2mpi_implementation ] ; then
  echo "No MPI implementation has been selected"
  exit 0
fi

IMPL=`cat ${MAD2_ROOT}/.mad2mpi_implementation`

case "$IMPL" in
  lam) 
    echo "Calling mpirun..."
    mpirun N $*
    ;;
  bip) 
    echo "unimplemented"
    ;;
  *)
    echo "invalid MPI implementation name"
    exit 0
    ;;
esac
##########
