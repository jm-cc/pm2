PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -DMARCEL_ACTNUMA"
PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -I/lib/modules/`uname -r`/build/include"
PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lnuma"
