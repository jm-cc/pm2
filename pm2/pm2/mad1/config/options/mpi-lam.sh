PM2_PROTOCOLS=mpi-lam
LAM_DIR=${LAM_DIR:-${HOME}/lam}
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -I${LAM_DIR}/h"
PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${LAM_DIR}/lib -lmpi -largs -ltrillium -ltstdio -lt"
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNET_ARCH="'\"'mpi-lam'\"'
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNETINTERF_INIT=mad_mpi_netinterf_init"
