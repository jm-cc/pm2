# auto generated from generic/config/options/*
if [ "${PM2_ARCH}" = IA64_ARCH ]; then
PM2_@LIB@_CFLAGS_KERNEL="$PM2_@LIB@_CFLAGS_KERNEL -ggdb2"
else
PM2_@LIB@_CFLAGS_KERNEL="$PM2_@LIB@_CFLAGS_KERNEL -ggdb3"
fi
