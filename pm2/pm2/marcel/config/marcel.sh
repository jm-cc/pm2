if [ "${PM2_ARCH}" = RS6K_ARCH ]; then
  PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -mno-powerpc"
fi
PM2_MARCEL_LIBNAME=marcel
