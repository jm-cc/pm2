PM2_MARCEL_FORCE_BUILD_DYNAMIC=yes
PM2_TBX_FORCE_BUILD_DYNAMIC=yes
PM2_MARCEL_MAKEFILE="LIBPTHREAD=true"
PM2_MARCEL_LIBNAME=pthread
PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -DMARCEL_LIBPTHREAD"
PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lrt"
