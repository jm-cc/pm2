PM2_BUBBLELIB_LIBNAME=bubblelib
PM2_BUBBLELIB_LIBS="$PM2_BUBBLELIB_LIBS -lming -lm"
PM2_BUBBLELIB_GENERATE_INCLUDE=true

PM2_BUBBLELIB_CFLAGS="$PM2_BUBBLELIB_CFLAGS `pkg-config --static --cflags fxt`"
PM2_BUBBLELIB_LIBS="$PM2_BUBBLELIB_LIBS `pkg-config --static --libs fxt`"