PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -DMARCEL_NUMA"
PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -DVERSION1_COMPATIBILITY"
[ "$PM2_SYS" = LINUX_SYS ] && PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lnuma"

