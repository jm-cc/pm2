PM2_MEMORY_LIBNAME=mami
PM2_DEFAULT_LOADER=${PM2_SRCROOT}/marcel/bin/marcelload_conf_not_needed

case " $PM2_LIBS " in
    *\ marcel\ *) ;;
    *) PM2_MEMORY_LIBS="$PM2_MEMORY_LIBS -lpthread" ;;
esac

PM2_MEMORY_CFLAGS="$PM2_MEMORY_CFLAGS -DMM_MAMI_ENABLED"
if [ "${PM2_ARCH}" = X86_64_ARCH -o "${PM2_ARCH}" = X86_ARCH ]; then
    PM2_MEMORY_CFLAGS="$PM2_MEMORY_CFLAGS -msse2"
fi
