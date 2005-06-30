# auto generated from generic/config/options/*
if [ "${PM2_ARCH}" = IA64_ARCH -o "${PM2_ARCH}" = X86_64_ARCH ]; then
  PM2_@LIB@_CFLAGS_KERNEL="$PM2_@LIB@_CFLAGS_KERNEL -ggdb2"
else
  PM2_@LIB@_CFLAGS_KERNEL="$PM2_@LIB@_CFLAGS_KERNEL -ggdb3"
fi
