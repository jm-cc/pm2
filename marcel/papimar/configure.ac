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
			    [papi_dir=$withval; enable_papi=yes], papi_dir=/usr/local/papi/)

if test -d "$papi_dir/include/"; then
	CPPFLAGS="${CPPFLAGS} -I$papi_dir/include/ "
fi
if test -d "$papi_dir/lib/"; then
	LDFLAGS="${LDFLAGS} -L$papi_dir/lib/ "
fi

AC_CHECK_HEADER([papi.h])
AC_SEARCH_LIBS([PAPI_library_init],[papi])

# Check availability of Marcel
PKG_CHECK_MODULES(MARCEL, marcel)
CFLAGS="$CFLAGS $MARCEL_CFLAGS"
LIBS="$LIBS $MARCEL_LIBS"
AC_CHECK_FUNCS([marcel_attr_setfpreswitch marcel_attr_setfpostswitch],[:],
	       [AC_MSG_ERROR([marcel was not built with papi support])])
AC_CHECK_FUNCS([marcel_gettid])

# Generate configuration-dependent files
AC_CONFIG_HEADERS([include/papimar_config.h])
AC_CONFIG_FILES([Makefile
		 src/Makefile
		 examples/Makefile
		 libpapimar.pc])

# Finalize
AC_OUTPUT

