PM2_PROTOCOLS=sisci
PM2_LOADER=${PM2_ROOT}/mad1/bin/tcp/madload
SISCI_DIR=${SISCI_DIR:-${HOME}/DIS/src/SISCI}
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DCANT_SEND_TO_SELF -I${SISCI_DIR}/api -I${SISCI_DIR}/src"
PM2_MAD1_LIBS="$PM2_MAD1_LIBS -L${SISCI_DIR}/api -lsisci"
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNET_ARCH="'\"'sisci'\"'
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNETINTERF_INIT=mad_sisci_netinterf_init"
