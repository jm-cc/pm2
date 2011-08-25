
if defined_in GTK1 PM2_EZFLAVOR_CFLAGS ; then
    PM2_EZFLAVOR_CFLAGS="$PM2_EZFLAVOR_CFLAGS `gtk-config --cflags`"
    PM2_EZFLAVOR_LIBS="$PM2_EZFLAVOR_LIBS `gtk-config --libs`"
else
    PM2_EZFLAVOR_CFLAGS="$PM2_EZFLAVOR_CFLAGS `pkg-config --cflags gtk+-2.0`"
    PM2_EZFLAVOR_LD_PATH="$PM2_EZFLAVOR_LD_PATH `pkg-config --libs-only-L gtk+-2.0`"
    PM2_EZFLAVOR_LIBS="$PM2_EZFLAVOR_LIBS `pkg-config --libs-only-l gtk+-2.0`"
fi

if defined_in WIN98_SYS_WINNT_SYS_WIN2K_SYS PM2_SYS ; then
    PM2_EZFLAVOR_CFLAGS="$PM2_EZFLAVOR_CFLAGS -fnative-struct"
    PM2_EZFLAVOR_LIBS="$PM2_EZFLAVOR_LIBS -e _mainCRTStartup"
fi    

