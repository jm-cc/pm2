# auto generated from generic/config/options/*

# As of GCC 4.2, `-gdwarf-2 -g3' is the recommended way to get macro
# expansion debugging information from GCC (info "(gdb) Compilation").

if [ "${PM2_ARCH}" = IA64_ARCH -o "${PM2_ARCH}" = X86_64_ARCH ]; then
  PM2_@LIB@_CFLAGS_KERNEL="$PM2_@LIB@_CFLAGS_KERNEL -gdwarf-2 -g3 -fno-omit-frame-pointer"
else
  PM2_@LIB@_CFLAGS_KERNEL="$PM2_@LIB@_CFLAGS_KERNEL -gdwarf-2 -g3 -fno-omit-frame-pointer"
fi
if [ "$PM2_SYS" = LINUX_SYS -o "$PM2_SYS" = GNU_SYS ]; then
  if [ "$PM2_@LIB@_BUILD_DYNAMIC" = yes ]; then
    PM2_@LIB@_EARLY_LDFLAGS_KERNEL="$PM2_@LIB@_EARLY_LDFLAGS_KERNEL -rdynamic"
  else
    PM2_@LIB@_EARLY_LDFLAGS="$PM2_@LIB@_EARLY_LDFLAGS -rdynamic"
  fi
fi
