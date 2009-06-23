# auto generated from generic/config/options/*
if [ "${PM2_ARCH}" = IA64_ARCH -o "${PM2_ARCH}" = X86_64_ARCH ]; then
  PM2_NMAD_CFLAGS_KERNEL="$PM2_NMAD_CFLAGS_KERNEL -pg -fno-omit-frame-pointer"
else
  PM2_NMAD_CFLAGS_KERNEL="$PM2_NMAD_CFLAGS_KERNEL -pg -fno-omit-frame-pointer"
fi
if [ "$PM2_SYS" = LINUX_SYS -o "$PM2_SYS" = GNU_SYS ]; then
  if [ "$PM2_NMAD_BUILD_DYNAMIC" = yes ]; then
    PM2_NMAD_EARLY_LDFLAGS_KERNEL="$PM2_NMAD_EARLY_LDFLAGS_KERNEL -rdynamic"
  else
    PM2_NMAD_EARLY_LDFLAGS="$PM2_NMAD_EARLY_LDFLAGS -rdynamic"
  fi
fi
