PM2_PROFILE_LIBNAME=profile
PM2_PROFILE_GENERATE_INCLUDE=true

PM2_PROFILE_CFLAGS="$PM2_PROFILE_CFLAGS `pkg-config --static --cflags fxt`"
PM2_PROFILE_LIBS="$PM2_PROFILE_LIBS `pkg-config --static --libs fxt`"
