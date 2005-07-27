# auto generated from generic/config/options/*
PM2_MARCEL_BUILD_STATIC=yes
if [ "$PM2_SYS" = DARWIN_SYS ]; then
    PM2_MARCEL_EARLY_OBJECT_FILES="_marcel_link.o"
else
    PM2_MARCEL_EARLY_LDFLAGS="${PM2_ROOT}/marcel/marcel.lds"
fi

