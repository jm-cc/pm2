
# Just for people which do not upgrade their flavors often! ;-)
if [ "$PM2_COMMON_LINK_MODE_STATIC" != yes \
      -a "$PM2_COMMON_LINK_MODE_DYNAMIC" != yes ] ; then
    echo "Warning, you should update your flavor to choose" \
        "STATIC or DYNAMIC linking mode" 1>&2
    echo "******** Default set to STATIC for now" 1>&2
    PM2_COMMON_LINK_MODE_STATIC=yes
fi

