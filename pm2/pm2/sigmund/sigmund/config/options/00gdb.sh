# auto generated from generic/config/options/*
if [ "${PM2_ARCH}" = IA64_ARCH ]; then
PM2_SIGMUND_CFLAGS_KERNEL="$PM2_SIGMUND_CFLAGS_KERNEL -ggdb2"
else
PM2_SIGMUND_CFLAGS_KERNEL="$PM2_SIGMUND_CFLAGS_KERNEL -ggdb3"
fi
