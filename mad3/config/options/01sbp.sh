PM2_PROTOCOLS="$PM2_PROTOCOLS mad-sbp"
PM2_MAD3_LIBS="$PM2_MAD3_LIBS -lded"
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -DDRV_SBP=mad_SBP"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO=mad_SBP"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO_PARAM=NULL"
