AC_INIT([libpapimar], [0.0], [olivier.aumage@inria.fr])

# Check we are configuring the right package
AC_CONFIG_SRCDIR(src/papimar.c)
AC_CONFIG_MACRO_DIR([m4])

# Init/check tools
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_LIBTOOL

# Check availability of various features
AC_HEADER_STDC
AC_HEADER_ASSERT

# Check availability of PAPI
AC_ARG_WITH(papi-dir, 
	    [AS_HELP_STRING([--with-papi-dir=<path>],
			    [specify PAPI installation directory (default is /usr/local/papi/)])],
			    [
			     papi_dir=$withval
			     enable_papi=yes
			     ], papi_dir=/usr/local/papi/)

if test -d "$papi_dir/include/"; then
	CPPFLAGS="${CPPFLAGS} -I$papi_dir/include/ "
fi
if test -d "$papi_dir/lib/"; then
	LDFLAGS="${LDFLAGS} -L$papi_dir/lib/ "
fi

papi_ok=yes

AC_CHECK_HEADER([papi.h],,[papi_ok=no])
AC_SEARCH_LIBS([PAPI_library_init],[papi],,[papi_ok=no])

if test x$papi_ok = xno; then
	AC_MSG_ERROR([papi lib not found])
fi

# Check availability of Marcel
marcel_ok=yes
marcel_patch_ok=yes

if test x$PM2_ROOT = x; then
	marcel_ok=no
fi

AC_PATH_PROG([PM2_CONFIG], [pm2-config], [marcel_ok=no])

if test x$PM2_CONFIG != x; then
	PM2_CFLAGS="`$PM2_CONFIG --cflags`"
	PM2_LDFLAGS="`$PM2_CONFIG --libs-only-L`"
	PM2_LDLIBS="`$PM2_CONFIG --libs-only-l`"
	PM2_LIBS="`$PM2_CONFIG --libs`"
fi

CPPFLAGS="$CPPFLAGS $PM2_CFLAGS"
CFLAGS="$CFLAGS $PM2_CFLAGS"
LDFLAGS="$LDFLAGS $PM2_LDFLAGS"
LDLIBS="$LDLIBS $PM2_LDLIBS"
LIBS="$LIBS $PM2_LIBS"

AC_CHECK_HEADER([marcel.h],,[marcel_ok=no])
AC_SEARCH_LIBS([marcel_create],[marcel],,[marcel_ok=no])
AC_CHECK_FUNCS([marcel_attr_setfpreswitch marcel_attr_setfpostswitch],
	       [:],
	       [marcel_patch_ok=no])

if test x$marcel_ok = xno; then
	AC_MSG_ERROR([marcel lib not found])
fi

if test x$marcel_patch_ok = xno; then
	AC_MSG_ERROR([marcel not patched for papi support])
fi

# Generate configuration-dependent files
AC_CONFIG_HEADERS([include/papimar_config.h])
AC_CONFIG_FILES([
		 Makefile
		 src/Makefile
		 examples/Makefile
		 libpapimar.pc
		 ])

# Finalize
AC_OUTPUT

