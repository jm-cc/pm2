PM2_DSM_LIBNAME=dsm
if [ "${PM2_ARCH}" = RS6K_ARCH ]; then
  PM2_DSM_CFLAGS="$PM2_DSM_CFLAGS -mno-powerpc"
fi
