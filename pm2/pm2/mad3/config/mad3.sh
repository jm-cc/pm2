PM2_MAD3_LIBNAME=mad
if [ "${PM2_ARCH}" = RS6K_ARCH ]; then
  PM2_MAD3_CFLAGS="$PM2_MAD3_CFLAGS -mno-powerpc"
fi
# PM2_DEFAULT_LOADER=$PM2_ROOT/mad3/bin/madload
PM2_MAD3_MODULE_DEPEND_LIB="${PM2_MAD3_MODULE_DEPEND_LIB} tbx ntbx marcel"
