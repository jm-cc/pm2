if [ "$PM2_ARCH" != IA64_ARCH ]; then
    PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -DMARCEL_DEBUG_SPINLOCK"
fi
