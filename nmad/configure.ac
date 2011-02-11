dnl -- configure.ac for nmad
dnl -- contains excerpts from PadicoTM configure.ac


AC_PREREQ(2.50)
AC_REVISION($Revision: 3960 $)
AC_INIT(include/nm_public.h)
AC_CONFIG_HEADER(include/nm_config.h)


dnl -- directories
dnl --------------------------------------------------------

dnl -- build dir
AC_MSG_CHECKING([nmad build directory])
nmad_builddir=`pwd`
if test -r ./configure; then
    AC_MSG_ERROR([ *** nmad must not be configured in its source tree. Create a separate 'build' dir instead. ])
fi
AC_SUBST(nmad_builddir)
AC_MSG_RESULT(${nmad_builddir})

dnl -- install dir
AC_MSG_CHECKING([nmad install directory])
nmad_root=$prefix
AC_SUBST(nmad_root)
if test x$prefix = xNONE; then
  AC_MSG_ERROR([No prefix given. Please supply a '--prefix' parameter.])
fi
AC_MSG_RESULT(${nmad_root})

dnl -- (absolute) source dir
AC_MSG_CHECKING([nmad source directory])
case ${srcdir} in
     /*) nmad_srcdir=${srcdir} ;;
     *)  nmad_srcdir="`pwd`/${srcdir}" ;;
esac
AC_SUBST(nmad_srcdir)
AC_MSG_RESULT(${nmad_srcdir})


dnl -- required packages
dnl --------------------------------------------------------

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([TBX],    [ tbx ])
PKG_CHECK_MODULES([PUKABI], [ PukABI ])
PKG_CHECK_MODULES([PUK],    [ Puk ])



dnl -- nmad content
dnl --------------------------------------------------------

nmad_drivers="self local tcp"
AC_SUBST(nmad_drivers)

nmad_strategies="default aggreg aggreg_extended aggreg_autoextended split_balance"
AC_SUBST(nmad_strategies)

nmad_interfaces="launcher pack sendrecv session"
AC_SUBST(nmad_interfaces)


dnl -- options
dnl --------------------------------------------------------

tag_container="tag_as_hashtable"
AC_ARG_ENABLE(taghuge,
	      [  --enable-taghuge  Enable 64 bit tags ],
              [ if test $enableval = yes; then
                    tag_container="tag_huge";
                fi
              ],
	      [])
AC_ARG_ENABLE(tagarray,
	      [  --enable-tagarray  Enable 8 bit tags in flat array ],
              [ if test $enableval = yes; then
                    tag_container="tag_as_flat_array";
                fi
              ],
	      [])
dnl -- TODO


out_sh=""
out_mk="Makefile"

dnl -- output 

dnl AC_OUTPUT_COMMANDS([chmod +x ${out_sh} ], [ out_sh="${out_sh}" ])
AC_OUTPUT(${out_mk} ${out_sh})