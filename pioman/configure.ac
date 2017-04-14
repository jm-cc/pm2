dnl -*- mode: Autoconf;-*-
dnl -- configure.ac for pioman


AC_PREREQ(2.50)
AC_REVISION($Revision: 3960 $)
AC_INIT(include/pioman.h)
AC_CONFIG_HEADER(piom_config.h)

m4_include([pkg.m4])
m4_include([tools.m4])
m4_include([misc.m4])
m4_include([out.m4])
m4_include([compiler.m4])
m4_include([features.m4])

dnl -- directories
dnl --------------------------------------------------------

AC_PADICO_PACKAGE([PIOMan], [pioman])

AC_MSG_CHECKING([pioman install directory])
pioman_root=$prefix
AC_SUBST(pioman_root)
AC_MSG_RESULT(${pioman_root})

dnl -- tools
dnl --------------------------------------------------------

AC_PADICO_BASE_TOOLS
AC_PADICO_GNU_MAKE
AC_PADICO_CDEP
AC_PADICO_LINKER
AC_PADICO_DYNLD

dnl -- base flags given to pkg-config
dnl --------------------------------------------------------

PIOMAN_CFLAGS="-DPIOMAN"
PIOMAN_LIBS="-L${prefix}/lib -Wl,-rpath,${prefix}/lib -lpioman"


dnl -- options
dnl --------------------------------------------------------

AC_PADICO_DEBUG
AC_PADICO_OPTIMIZE
if test "x${ENABLE_DEBUG_FLAG}" = "xyes"; then
  AC_DEFINE([PIOMAN_DEBUG], [1], [whether PIOMan is built in DEBUG])
fi


dnl -- external packages
dnl --------------------------------------------------------

PKG_PROG_PKG_CONFIG

dnl -- tbx
PKG_CHECK_MODULES([tbx],    [ tbx ])

dnl -- marcel
AC_ARG_WITH([marcel],
	    [AS_HELP_STRING([--with-marcel], [use marcel threads @<:@default=no@:>@] )])
if test "x${with_marcel}" = "xyes"; then
  PKG_CHECK_MODULES([marcel], [ marcel ])
  pioman_pkg_requires="${pioman_pkg_requires} marcel"
  AC_DEFINE([PIOMAN_MARCEL], [1], [whether PIOMan is built with Marcel threads])
fi

dnl -- pthread
AC_ARG_WITH([pthread],
	[AS_HELP_STRING([--with-pthread], [use pthread @<:@default=no@:>@] )])
if test "x${with_pthread}" = "xyes"; then
  pthread_LIBS="-lpthread -lrt"
  PIOMAN_LIBS="${PIOMAN_LIBS} ${pthread_LIBS}"
  AC_DEFINE([PIOMAN_PTHREAD], [1], [whether PIOMan is built with pthreads])
fi

dnl -- argobots
AC_ARG_WITH([argobots],
	    [AS_HELP_STRING([--with-argobots], [use argobots threads @<:@default=no@:>@] )])
if test "x${with_argobots}" = "xyes"; then
  PKG_CHECK_MODULES([argobots], [ argobots ])
  pioman_pkg_requires="${pioman_pkg_requires} argobots"
  AC_DEFINE([PIOMAN_ABT], [1], [whether PIOMan is built with argobots threads])
fi

dnl -- gtg
AC_ARG_WITH([gtg],
	[AS_HELP_STRING([--with-gtg], [build with GTG traces @<:@default=no@:>@] )])
if test "x${with_gtg}" = "xyes"; then
  PKG_CHECK_MODULES([gtg],    [ gtg ])
  pioman_pkg_requires="${pioman_pkg_requires} gtg"
  AC_DEFINE([PIOMAN_TRACE], [1], [whether PIOMan is built with GTG traces])
fi

dnl -- internal flags used to build PIOMan
PIOMAN_BUILD_CFLAGS="${tbx_CFLAGS} ${marcel_CFLAGS} ${gtg_CFLAGS}"
AC_SUBST(PIOMAN_BUILD_CFLAGS)
PIOMAN_BUILD_LIBS="${tbx_LIBS} ${marcel_LIBS} ${gtg_LIBS} ${pthread_LIBS}"
AC_SUBST(PIOMAN_BUILD_LIBS)

dnl -- gen pioman.pc for pkg-config
AC_SUBST(pioman_pkg_requires)
AC_SUBST(PIOMAN_CFLAGS)
AC_SUBST(PIOMAN_LIBS)

dnl -- output 

out_mk="${out_mk} eztrace-pioman.tpl"
AC_PADICO_OUT_COMMON_MK
AC_PADICO_OUT_ROOT
AC_PADICO_OUTPUT


