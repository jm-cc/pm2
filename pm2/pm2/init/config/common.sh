if [ "${PM2_ARCH}" = RS6K_ARCH ]; then
  PM2_COMMON_CFLAGS="$PM2_COMMON_CFLAGS -mno-powerpc"
fi
