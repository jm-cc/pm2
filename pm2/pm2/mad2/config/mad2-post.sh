
case " $PM2_MAD2_CFLAGS " in
    *\ -DAPPLICATION_SPAWN\ *)
	PM2_DEFAULT_LOADER=$PM2_ROOT/mad2/bin/mad2_app_spawn_load
	;;
    *)
	PM2_DEFAULT_LOADER=$PM2_ROOT/mad2/bin/madload
	;;
esac

