PM2_INIT_CFLAGS="$PM2_INIT_CFLAGS -DPM2_TOPOLOGY "`pkg-config --cflags topology`
PM2_INIT_LIBS="$PM2_INIT_LIBS "`pkg-config --libs --static topology`
