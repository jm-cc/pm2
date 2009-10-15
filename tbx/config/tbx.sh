case " $PM2_LIBS " in
    *\ mad?\ *) ;;
    *) PM2_DEFAULT_LOADER=${PM2_SRCROOT}/tbx/bin/marcelload_conf_not_needed ;;
esac

PM2_TBX_LIBNAME=tbx
