PM2_PROTOCOLS="$PM2_PROTOCOLS quadrics"
PM2_MAD3_LIBS="$PM2_MAD3_LIBS -lelan"
PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -DDRV_QUADRICS=mad_QUADRICS -I/opt/qsnet/include"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO=mad_QUADRICS"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD3_MAD1_MAIN_PROTO_PARAM=NULL"
