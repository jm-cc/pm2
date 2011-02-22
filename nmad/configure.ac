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


dnl -- options
dnl --------------------------------------------------------

dnl -- base CFLAGS
dnl --
NMAD_CFLAGS="-DNMAD -I${nmad_srcdir}/include -I${nmad_builddir}/include -I${nmad_srcdir}/ccs"

dnl -- tag container
dnl --
AC_MSG_CHECKING([for tag container])
tag_container="TAG_AS_HASHTABLE"
AC_ARG_ENABLE(taghuge,
	      [  --enable-taghuge   Enable 64 bit tags ],
              [ if test $enableval = yes; then
                    tag_container="TAG_HUGE";
                fi
              ],
	      [])
AC_ARG_ENABLE(tagarray,
	      [  --enable-tagarray  Enable 8 bit tags in flat array ],
              [ if test $enableval = yes; then
                    tag_container="TAG_AS_FLAT_ARRAY";
                fi
              ],
	      [])
NMAD_CFLAGS="${NMAD_CFLAGS} -DCONFIG_${tag_container}"
AC_MSG_RESULT([${tag_container}])

dnl -- Mad-MPI
dnl --
AC_ARG_ENABLE(mpi,
	      [  --enable-mpi       Enable builtin MPI implementation Mad-MPI],
	      [ if test $enableval = yes; then
	            AC_DEFINE([MAD_MPI], 1, [enable Mad-MPI])
		    nmad_interfaces="${nmad_interfaces} mpi"
	        fi
	      ],
	      [])

dnl -- sampling
dnl --
AC_ARG_ENABLE(sampling,
	      [  --enable-sampling  Enable network sampling],
	      [ if test $enableval = yes; then
	            AC_DEFINE([SAMPLING], 1, [enable sampling])
	        fi
	      ],
	      [])

dnl -- Fortran
NMAD_CFLAGS="${NMAD_CFLAGS} -DPM2_FORTRAN_TARGET_NONE"

dnl -- NUIOA
dnl --
dnl - TODO- enable NUIOA by default if numa.h is present
dnl - TODO- check for numa.h
AC_ARG_ENABLE(nuioa,
	      [  --enable-nuioa     Enable NUMA control on I/O],
	      [ if test $enableval = yes; then
	            AC_DEFINE([PM2_NUIOA], 1, [enable NUIOA])
	        fi
	      ],
	      [])


dnl -- external packages
dnl --------------------------------------------------------

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([tbx],    [ tbx ])
PKG_CHECK_MODULES([Puk],    [ Puk ])

PKG_CHECK_MODULES([PukABI], [ PukABI ])

NMAD_EXT_CFLAGS="${tbx_CFLAGS} ${PukABI_CFLAGS} ${Puk_CFLAGS}"
AC_SUBST(NMAD_CFLAGS)
AC_SUBST(NMAD_EXT_CFLAGS)

NMAD_EXT_LIBS="${tbx_LIBS} ${PukABI_LIBS} ${Puk_LIBS}"
AC_SUBST(NMAD_EXT_LIBS)
NMAD_LIBS="-L${nmad_root}/lib -Wl,-rpath,${nmad_root}/lib -lnmad"
AC_SUBST(NMAD_LIBS)

NMAD_LDFLAGS="${NMAD_LDFLAGS} -Wl,--export-dynamic"
AC_SUBST(NMAD_LDFLAGS)


dnl -- nmad content
dnl --------------------------------------------------------

nmad_drivers="${nmad_drivers} self local tcp"
AC_SUBST(nmad_drivers)

nmad_strategies="${nmad_strategies} default aggreg aggreg_extended aggreg_autoextended split_balance"
AC_SUBST(nmad_strategies)

nmad_interfaces="${nmad_interfaces} launcher pack sendrecv session"
AC_SUBST(nmad_interfaces)




out_sh=""
out_mk="Makefile examples/sendrecv/Makefile nmad.pc"

dnl -- output 

dnl AC_OUTPUT_COMMANDS([chmod +x ${out_sh} ], [ out_sh="${out_sh}" ])
AC_OUTPUT(${out_mk} ${out_sh})