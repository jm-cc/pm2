# auto generated from generic/config/options/*
PM2_MARCEL_BUILD_STATIC=yes
PM2_MARCEL_BUILD_DYNAMIC=yes
PM2_COMMON_PM2_SHLIBS="$PM2_COMMON_PM2_SHLIBS MARCEL"
if [ "$PM2_SYS" = DARWIN_SYS ]; then
    PM2_MARCEL_EARLY_OBJECT_FILES="_marcel_link.o"
    PM2_MARCEL_EARLY_OBJECT_FILES_KERNEL="_marcel_link.o"
else
    PM2_MARCEL_EARLY_LDFLAGS="${PM2_ROOT}/marcel/marcel.lds"
    PM2_MARCEL_EARLY_LDFLAGS_KERNEL="${PM2_ROOT}/marcel/marcel.lds"
fi
