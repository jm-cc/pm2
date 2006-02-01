# auto generated from generic/config/options/*
if [ "${PM2_ARCH}" = IA64_ARCH -o "${PM2_ARCH}" = X86_64_ARCH ]; then
  PM2_@LIB@_CFLAGS_KERNEL="$PM2_@LIB@_CFLAGS_KERNEL -ggdb2 -fno-omit-frame-pointer"
else
  PM2_@LIB@_CFLAGS_KERNEL="$PM2_@LIB@_CFLAGS_KERNEL -ggdb2 -fno-omit-frame-pointer"
fi
if [ "$PM2_@LIB@_BUILD_DYNAMIC" = yes ]; then
	PM2_@LIB@_EARLY_LDFLAGS_KERNEL="$PM2_@LIB@_EARLY_LDFLAGS_KERNEL -rdynamic"
else
	PM2_@LIB@_EARLY_LDFLAGS="$PM2_@LIB@_EARLY_LDFLAGS -rdynamic"
fi
