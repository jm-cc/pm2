PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -DMARCEL_MAMI_ENABLED"
if [ "${PM2_ARCH}" = X86_64_ARCH -o "${PM2_ARCH}" = X86_ARCH ]; then
    PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -msse2"
fi
