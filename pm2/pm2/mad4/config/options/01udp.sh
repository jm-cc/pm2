PM2_PROTOCOLS="$PM2_PROTOCOLS udp"
if [ "${PM2_SYS}" = SOLARIS_SYS ]; then
    PM2_MAD4_LIBS="$PM2_MAD4_LIBS -lnsl -lsocket"
fi
PM2_MAD4_CFLAGS="$PM2_MAD4_CFLAGS -DDRV_UDP=mad_UDP"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD4_MAD1_MAIN_PROTO=mad_UDP"
PM2_PM2_CFLAGS="$PM2_PM2_CFLAGS -DMAD4_MAD1_MAIN_PROTO_PARAM=NULL"
