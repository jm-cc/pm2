BIP_DIR=${BIP_DIR:-/usr/local/bip}
PM2_PROTOCOLS=mpi_bip
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -I${BIP_DIR}/include -I${BIP_DIR}/include/mpi"
PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${BIP_DIR}/lib -L${BIP_DIR}/lib/mpi_gnu -lmpi -lbip"
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNET_ARCH="'\"'mpi_bip'\"'
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNETINTERF_INIT=mad_mpi_netinterf_init"
