#!/bin/sh
##########

if [ ! -f ${MAD2_ROOT}/.mad2mpi_implementation ] ; then
  echo "No MPI implementation has been selected"
  exit 0
fi

IMPL=`cat ${MAD2_ROOT}/.mad2mpi_implementation`
echo "Booting MPI/${IMPL} ..."

case "$IMPL" in
  lam) 
    echo "Calling recon"
    recon   -v ${MAD2_ROOT}/.mad2_conf
    echo Calling lamboot ...
    lamboot -v ${MAD2_ROOT}/.mad2_conf
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
