
if defined_in APPLICATION_SPAWN PM2_MAD2_CFLAGS ; then
    PM2_DEFAULT_LOADER=$PM2_ROOT/mad2/bin/mad2_app_spawn_load
else
    PM2_DEFAULT_LOADER=$PM2_ROOT/mad2/bin/madload
fi
