
if [ x${PM2_PUK_ENABLE_PUKABI} = xyes ]; then
    PM2_PUK_CONFIGURE_EXTRA_FLAGS="$PM2_PUK_CONFIGURE_EXTRA_FLAGS --enable-pukabi"
    PM2_PUK_LIBS="${PM2_PUK_LIBS} -lPukABI"
else
    PM2_PUK_CONFIGURE_EXTRA_FLAGS="$PM2_PUK_CONFIGURE_EXTRA_FLAGS --disable-pukabi"
fi

PM2_COMMON_PM2_STLIBS="$PM2_COMMON_PM2_STLIBS PUK"
