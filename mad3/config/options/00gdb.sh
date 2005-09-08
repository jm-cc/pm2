# auto generated from generic/config/options/*
if [ "${PM2_ARCH}" = IA64_ARCH -o "${PM2_ARCH}" = X86_64_ARCH ]; then
  PM2_MAD3_CFLAGS_KERNEL="$PM2_MAD3_CFLAGS_KERNEL -ggdb2 -fno-omit-frame-pointer"
else
  PM2_MAD3_CFLAGS_KERNEL="$PM2_MAD3_CFLAGS_KERNEL -ggdb2 -fno-omit-frame-pointer"
fi
