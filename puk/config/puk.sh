PM2_PUK_LIBNAME=PadicoPuk
PM2_PUK_CFLAGS="$PM2_PUK_CFLAGS -I${PADICO_ROOT}/include -DPADICO"
PM2_PUK_LIBS="-L${PADICO_ROOT}/lib -lPadicoPuk -lexpat -ldl"
