# auto generated from generic/config/options/*

# As of GCC 4.2, `-gdwarf-2 -g3' is the recommended way to get macro
# expansion debugging information from GCC (info "(gdb) Compilation").

PM2_INIT_CFLAGS_KERNEL="$PM2_INIT_CFLAGS_KERNEL -gdwarf-2 -g3 -fno-omit-frame-pointer -DINIT_GDB"

if [ "$PM2_SYS" = LINUX_SYS -o "$PM2_SYS" = GNU_SYS ]; then
  if [ "$PM2_INIT_BUILD_DYNAMIC" = yes ]; then
    PM2_INIT_EARLY_LDFLAGS_KERNEL="$PM2_INIT_EARLY_LDFLAGS_KERNEL -rdynamic"
  else
    PM2_INIT_EARLY_LDFLAGS="$PM2_INIT_EARLY_LDFLAGS -rdynamic"
  fi
fi
