PM2_PROTOCOLS=mpi_lam
LAM_DIR=${LAM_DIR:-${HOME}/lam}
PM2_LOADER=${PM2_ROOT}/mad1/bin/mpi_lam/madload
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -I${LAM_DIR}/h"
PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${LAM_DIR}/lib -lmpi -largs -ltrillium -ltstdio -lt"
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNET_ARCH="'\"'mpi_lam'\"'
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNETINTERF_INIT=mad_mpi_netinterf_init"
