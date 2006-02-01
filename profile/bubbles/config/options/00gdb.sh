# auto generated from generic/config/options/*
if [ "${PM2_ARCH}" = IA64_ARCH -o "${PM2_ARCH}" = X86_64_ARCH ]; then
  PM2_BUBBLES_CFLAGS_KERNEL="$PM2_BUBBLES_CFLAGS_KERNEL -ggdb2 -fno-omit-frame-pointer"
else
  PM2_BUBBLES_CFLAGS_KERNEL="$PM2_BUBBLES_CFLAGS_KERNEL -ggdb2 -fno-omit-frame-pointer"
fi
if [ "$PM2_BUBBLES_BUILD_DYNAMIC" = yes ]; then
	PM2_BUBBLES_EARLY_LDFLAGS_KERNEL="$PM2_BUBBLES_EARLY_LDFLAGS_KERNEL -rdynamic"
else
	PM2_BUBBLES_EARLY_LDFLAGS="$PM2_BUBBLES_EARLY_LDFLAGS -rdynamic"
fi
