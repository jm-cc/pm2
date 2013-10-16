dnl -- configure.ac for pioman


AC_PREREQ(2.50)
AC_REVISION($Revision: 3960 $)
AC_INIT(include/pioman.h)


dnl -- directories
dnl --------------------------------------------------------

dnl -- build dir
AC_MSG_CHECKING([pioman build directory])
pioman_builddir=`pwd`
if test -r ./configure; then
    AC_MSG_ERROR([ *** nmad must not be configured in its source tree. Create a separate 'build' dir instead. ])
fi
AC_SUBST(pioman_builddir)
AC_MSG_RESULT(${pioman_builddir})

dnl -- install dir
AC_MSG_CHECKING([nmad install directory])
pioman_root=$prefix
AC_SUBST(pioman_root)
if test x$prefix = xNONE; then
  AC_MSG_ERROR([No prefix given. Please supply a '--prefix' parameter.])
fi
AC_MSG_RESULT(${pioman_root})

dnl -- (absolute) source dir
AC_MSG_CHECKING([nmad source directory])
case ${srcdir} in
     /*) pioman_srcdir=${srcdir} ;;
     *)  pioman_srcdir="`pwd`/${srcdir}" ;;
esac
AC_SUBST(pioman_srcdir)
AC_MSG_RESULT(${pioman_srcdir})

dnl -- tools
dnl --------------------------------------------------------

AC_PATH_PROG(BIN_TRUE, true)
AC_SUBST(BIN_TRUE)

AC_PATH_PROG([BASH], [bash])
AC_SUBST([BASH])

AC_PROG_CC
AC_PROG_CPP

BASE_CFLAGS="-Wall"
DEBUG_CFLAGS="-g -DDEBUG"
OPT_CFLAGS="-O3 -DNDEBUG"
CPICFLAGS="-fPIC"
CDEPFLAGS="-MM"
DYNLIB_LDFLAGS="-shared -rdynamic"
OPT_LDFLAGS="-Wl,-O1"
AC_SUBST(CPPFLAGS)
AC_SUBST(CPICFLAGS)
AC_SUBST(DEBUG_CFLAGS)
AC_SUBST(BASE_CFLAGS)
AC_SUBST(OPT_CFLAGS)
AC_SUBST(OPT_LDFLAGS)

AC_MSG_CHECKING([how to check C dependency])
CC_DEP="${CC} ${CDEPFLAGS}"
AC_MSG_RESULT([${CC_DEP}])
AC_SUBST(CC_DEP)

AC_MSG_CHECKING([for dynamic library linker])
DYNLIB_LD="${CC} ${CPICFLAGS} ${DYNLIB_LDFLAGS}"
AC_MSG_RESULT([${DYNLIB_LD}])
AC_SUBST(DYNLIB_LD)

dnl -- options
dnl --------------------------------------------------------

dnl -- base flags
dnl --
PIOMAN_CFLAGS="-DPIOMAN -I${pioman_srcdir}/include"

PIOMAN_LIBS="-L${pioman_root}/lib -Wl,-rpath,${pioman_root}/lib -lpioman"

gcc_version="`${CC} -dumpversion`"
case $gcc_version in
   2.*|3.*|4.0*|4.1*)
    ;;
   *) dnl  -march=native is only available since gcc 4.2
	OPT_CFLAGS="$OPT_CFLAGS -march=native"
	;;
esac

AC_ARG_ENABLE(debug,
	      [AS_HELP_STRING([ --enable-debug ], [ enable debug @<:@default=no@:>@] )],
	      [ if test $enableval = yes; then 
	            PIOMAN_CFLAGS="${PIOMAN_CFLAGS} ${DEBUG_CFLAGS}"
	        fi
	      ],
	      [])
AC_ARG_ENABLE(optimize,
	      [AS_HELP_STRING([ --enable-optimize ], [ enable compiler optimizations @<:@default=no@:>@] )],
	      [ if test $enableval = yes; then 
	            PIOMAN_CFLAGS="${PIOMAN_CFLAGS} ${OPT_CFLAGS}"
	        fi
	      ],
	      [])

dnl -- external packages
dnl --------------------------------------------------------

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([tbx],    [ tbx ])

AC_ARG_WITH([marcel],
	    [AS_HELP_STRING([--with-marcel], [use marcel threads @<:@default=no@:>@] )])
if test "x${with_marcel}" = "xyes"; then
  PKG_CHECK_MODULES([marcel], [ marcel ])
  requires_marcel="marcel"
   PIOMAN_CFLAGS="${PIOMAN_CFLAGS} -DPIOMAN_MARCEL"
else
  requires_marcel=
fi
AC_SUBST([requires_marcel])

AC_ARG_WITH([pthread],
	[AS_HELP_STRING([--with-pthread], [use pthread @<:@default=no@:>@] )])
if test "x${with_pthread}" = "xyes"; then
   pioman_LIBS="${pioman_LIBS} -lpthread -lrt"
   PIOMAN_CFLAGS="${PIOMAN_CFLAGS} -DPIOMAN_PTHREAD"
   PIOMAN_LIBS="${PIOMAN_LIBS} -lpthread -lrt"
fi

AC_ARG_WITH([gtg],
	[AS_HELP_STRING([--with-gtg], [build with GTG traces @<:@default=no@:>@] )])
if test "x${with_gtg}" = "xyes"; then
   PKG_CHECK_MODULES([gtg],    [ gtg ])
   pioman_LIBS="${pioman_LIBS} ${gtg_LIBS}"
   PIOMAN_CFLAGS="${PIOMAN_CFLAGS} -DPIOMAN_TRACE ${gtg_CFLAGS}"
   PIOMAN_LIBS="${PIOMAN_LIBS} ${gtg_LIBS}"
fi

PIOMAN_EXT_CFLAGS="${tbx_CFLAGS} ${marcel_CFLAGS}"
AC_SUBST(PIOMAN_EXT_CFLAGS)

PIOMAN_EXT_LIBS="${tbx_LIBS} ${marcel_LIBS} ${pioman_LIBS}"
AC_SUBST(PIOMAN_EXT_LIBS)


AC_SUBST(PIOMAN_CFLAGS)
AC_SUBST(PIOMAN_LIBS)


out_sh=""
out_mk="Makefile pioman.pc"

dnl -- output 

dnl AC_OUTPUT_COMMANDS([chmod +x ${out_sh} ], [ out_sh="${out_sh}" ])
AC_OUTPUT(${out_mk} ${out_sh})

