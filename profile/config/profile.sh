PM2_PROFILE_LIBNAME=profile
PM2_PROFILE_GENERATE_INCLUDE=true
PM2_PROFILE_LIBS="-lfxt -lbfd"
PM2_PROFILE_CFLAGS="$PM2_PROFILE_CFLAGS -I$FXT_PREFIX/include"
PM2_PROFILE_LIBS="$PM2_PROFILE_LIBS -L$FXT_PREFIX/lib"
