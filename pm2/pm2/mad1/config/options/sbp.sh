PM2_PROTOCOLS=sbp
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DCANT_SEND_TO_SELF"
PM2_MAD1_LIBS="$PM2_MAD1_LIBS -lded"
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNET_ARCH="'\"'sbp'\"'
PM2_MAD1_CFLAGS="$PM2_MAD1_CFLAGS -DNETINTERF_INIT=mad_sbp_netinterf_init"
