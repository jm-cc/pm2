PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -DMARCEL_BIND_LWP_ON_PROCESSORS"
grep -q -e '^extern int sched_setaffinity ([^,]*,[^,]*)' /usr/include/sched.h && PM2_MARCEL_CFLAGS="$PM2_MARCEL_CFLAGS -DHAVE_OLD_SCHED_SETAFFINITY"
