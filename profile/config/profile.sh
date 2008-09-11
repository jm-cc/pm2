PM2_PROFILE_LIBNAME=profile
PM2_PROFILE_GENERATE_INCLUDE=true

if [ -n "$FXT_PREFIX" ]
then
    PM2_PROFILE_CFLAGS="$PM2_PROFILE_CFLAGS -I$FXT_PREFIX/include"
    PM2_PROFILE_LIBS="$PM2_PROFILE_LIBS -L$FXT_PREFIX/lib"
elif pkg-config libming
then
    PM2_PROFILE_CFLAGS="$PM2_PROFILE_CFLAGS $(pkg-config libming --cflags)"
    PM2_PROFILE_LIBS="$PM2_PROFILE_LIBS $(pkg-config libming --libs-only-L)"
fi

if [ "$PM2_SYS" = DARWIN_SYS ]; then
    PM2_PROFILE_LIBS="$PM2_PROFILE_LIBS -lfxt"
else
    PM2_PROFILE_LIBS="$PM2_PROFILE_LIBS -lfxt -lbfd -liberty"
fi
