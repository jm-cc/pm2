PM2_MEMORY_LIBNAME=mami
PM2_DEFAULT_LOADER=${PM2_SRCROOT}/marcel/bin/marcelload_conf_not_needed

case " $PM2_LIBS " in
    *\ marcel\ *) ;;
    *) PM2_MEMORY_LIBS="$PM2_MEMORY_LIBS -lpthread" ;;
esac
