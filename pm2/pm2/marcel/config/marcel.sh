PM2_MARCEL_CFLAGS_KERNEL="$PM2_MARCEL_CFLAGS_KERNEL -fomit-frame-pointer"
#PM2_MARCEL_LIBS_SCRIPTS="$PM2_MARCEL_LIBS_SCRIPTS ${PM2_ROOT}/marcel/marcel.lds"
PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -I${PM2_ROOT}/marcel/autogen-include"

PM2_MARCEL_STAMP_FILES="_marcel_link.o"

case " $PM2_LIBS " in
    *\ mad?\ *) ;;
    *) PM2_DEFAULT_LOADER=${PM2_ROOT}/marcel/bin/marcelload_conf_not_needed ;;
esac

if [ "$PM2_SYS" = SOLARIS_SYS ]; then
    if [ "$PM2_ARCH" = X86_ARCH ]; then
	PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lrt"
    else
	PM2_MARCEL_LIBS="$PM2_MARCEL_LIBS -lposix4"
    fi
fi

PM2_MARCEL_GENERATE_INCLUDE=true

