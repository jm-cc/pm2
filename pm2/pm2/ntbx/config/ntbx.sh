if [ "${PM2_ARCH}" = RS6K_ARCH ]; then
  PM2_NTBX_CFLAGS="$PM2_NTBX_CFLAGS -mno-powerpc"
fi
if [ "${PM2_SYS}" = SOLARIS_SYS ]; then
    PM2_NTBX_LIBS="$PM2_NTBX_LIBS -lnsl -lsocket"
fi
PM2_NTBX_LIBNAME=ntbx
PM2_NTBX_CFLAGS="$PM2_NTBX_CFLAGS -DNTBX_TCP"
