#PM2_PTHREAD_LIBS="$PM2_PTHREAD_LIBS -lfl"

##PM2_MARCEL_CFLAGS_KERNEL="$PM2_MARCEL_CFLAGS_KERNEL -DMARCEL_LIBPTHREAD"
##PM2_COMMON_CFLAGS="$PM2_COMMON_CFLAGS -fPIC"
##PM2_PTHREAD_CFLAGS="$PM2_PTHREAD_CFLAGS -O2"
##PM2_PTHREAD_PROGNAME=pthread
##PM2_PTHREAD_MODULE_DEPEND_LIB="${PM2_PTHREAD_MODULE_DEPEND_LIB} init marcel tbx"

PM2_STACKALIGN_LIBNAME=stackalign
#PM2_PTHREAD_CFLAGS_KERNEL="$PM2_MARCEL_CFLAGS_KERNEL -fomit-frame-pointer"
PM2_STACKALIGN_LIBS="-ldl"

PM2_STACKALIGN_BUILD_DYNAMIC=yes
# On ne veut pas de link avec stackalign
PM2_STACKALIGN_DO_NOT_LINK_LIB_WITH_OTHERS=yes

PM2_LD_PRELOAD="${PM2_LD_PRELOAD:+${PM2_LD_PRELOAD}:}libstackalign.so"
