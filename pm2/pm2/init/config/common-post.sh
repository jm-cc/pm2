
# Just for people which do not upgrade their flavors often! ;-)
if [ "$PM2_COMMON_LINK_MODE_STATIC" != yes -a "$PM2_COMMON_LINK_MODE_DYNAMIC" != yes ] ; then
    PM2_COMMON_LINK_MODE_STATIC=yes
fi

