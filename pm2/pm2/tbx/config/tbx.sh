if [ "${PM2_ARCH}" = RS6K_ARCH ]; then
  PM2_TBX_CFLAGS="$PM2_TBX_CFLAGS -mno-powerpc"
fi
PM2_TBX_LIBNAME=tbx
