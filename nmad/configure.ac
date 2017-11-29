dnl -*- mode: Autoconf;-*-
dnl -- configure.ac for nmad


AC_PREREQ(2.50)
AC_REVISION($Revision: 3960 $)
AC_INIT(include/nm_public.h)
AC_CONFIG_HEADER(include/nm_private_config.h include/nm_config.h)

m4_include([pkg.m4])
m4_include([tools.m4])
m4_include([misc.m4])
m4_include([out.m4])
m4_include([compiler.m4])
m4_include([features.m4])
m4_include([./nmad.m4])

dnl -- directories
dnl --------------------------------------------------------

AC_PADICO_PACKAGE([NewMadeleine], [nmad])

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
AC_DEFINE_UNQUOTED([NMAD_ROOT], ["${nmad_root}"], [nmad install directory])
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


dnl -- tools
dnl --------------------------------------------------------

AC_PADICO_BASE_TOOLS
AC_PADICO_GNU_MAKE
AC_PADICO_CDEP
AC_PADICO_LINKER
AC_PADICO_DYNLD

dnl -- options
dnl --------------------------------------------------------

AC_PADICO_DEBUG
AC_PADICO_OPTIMIZE

dnl -- tag container
dnl --
AC_MSG_CHECKING([for tag container])
tag_container="taghuge"
AC_ARG_ENABLE(taghuge,
	      [AS_HELP_STRING([--enable-taghuge], [Enable 64 bit tags @<:@default@:>@]) ],
              [ if test $enableval = yes; then
                    tag_container="taghuge";
                fi
              ],
	      [])
AC_ARG_ENABLE(tagarray,
	      [AS_HELP_STRING([--enable-tagarray], [Enable 8 bit tags in flat array]) ],
              [ if test $enableval = yes; then
                    tag_container="tagarray";
                fi
              ],
	      [])
AC_ARG_ENABLE(taghashtable,
	      [AS_HELP_STRING([--enable-taghashtable], [Enable 32 bit tags in hashtable]) ],
              [ if test $enableval = yes; then
                    tag_container="taghashtable";
                fi
              ],
	      [])
case ${tag_container} in
     tagarray)     AC_DEFINE([NM_TAGS_AS_FLAT_ARRAY],    [1], [use 8 bits tags in arrays]);;
     taghashtable) AC_DEFINE([NM_TAGS_AS_HASHTABLE],     [1], [use 32 bits tags in hashtable]);;
     taghuge)      AC_DEFINE([NM_TAGS_AS_INDIRECT_HASH], [1], [use 64 bits tags in hashtable]);;
esac
AC_MSG_RESULT([${tag_container}])

dnl -- sampling
dnl --
AC_ARG_ENABLE(sampling,
	      [AS_HELP_STRING([--enable-sampling], [Enable network sampling]) ],
	      [ if test $enableval = yes; then
	            AC_DEFINE([NMAD_SAMPLING], 1, [enable sampling])
		    nmad_interfaces="${nmad_interfaces} sampling"
	        fi
	      ],
	      [])

dnl -- NMAD_TRACE
AC_ARG_ENABLE(trace,
	      [AS_HELP_STRING([--enable-trace], [Enable PAJE trace]) ],
	      [ if test $enableval = yes; then
	            AC_DEFINE([NMAD_TRACE], 1, [enable trace])
		    nmad_interfaces="${nmad_interfaces} trace"
	        fi
	      ],
	      [])

dnl -- TODO: fine_grain_lock


dnl -- external packages
dnl --------------------------------------------------------

PKG_PROG_PKG_CONFIG

dnl -- gtg
AC_ARG_WITH([gtg],
	[AS_HELP_STRING([--with-gtg], [build with GTG traces @<:@default=no@:>@] )])
AC_MSG_CHECKING([for gtg])
if test "x${with_gtg}" = "xyes"; then
   AC_MSG_RESULT([yes])
   PKG_CHECK_MODULES([gtg],    [ gtg ])
else
   AC_MSG_RESULT([no])
fi
AC_SUBST(with_gtg)


dnl -- tbx
PKG_CHECK_MODULES([tbx],    [ tbx ])

dnl -- Puk
PKG_CHECK_MODULES([Puk],    [ Puk ])
AC_PADICO_PUK_ROOT

dnl -- PukABI
AC_NMAD_PUKABI

dnl -- PadicoTM
AC_NMAD_PADICOTM

dnl -- PIOMan
AC_NMAD_PIOMAN

dnl -- ibverbs
AC_PADICO_CHECK_INFINIBAND

dnl -- PSM
AC_PADICO_CHECK_PSM

dnl -- PSM2
AC_PADICO_CHECK_PSM2

dnl -- OFI
AC_PADICO_CHECK_OFI

dnl -- dcfa
AC_NMAD_CHECK_DCFA

dnl -- MX
AC_NMAD_CHECK_MX

dnl -- CCI
AC_NMAD_CHECK_CCI

dnl -- LZ4 compression
AC_PADICO_CHECK_LZ4

dnl -- Fortran
AC_MSG_CHECKING([for Fortran support in tbx])
nmad_fortran_target=none
echo $tbx_CFLAGS | grep -q FORTRAN_COMPILER
__tbx_have_fortran=$?
if test ${__tbx_have_fortran} -eq 0; then
  echo $tbx_CFLAGS | grep -q TBX_FORTRAN_COMPILER_GNU
  __tbx_have_gfortran=$?
  echo $tbx_CFLAGS | grep -q TBX_FORTRAN_COMPILER_INTEL
  __tbx_have_ifort=$?
  if test ${__tbx_have_gfortran} -eq 0; then
    nmad_fortran_target=gfortran
    AC_DEFINE([NMAD_FORTRAN_TARGET_GFORTRAN], 0, [GNU gfortran Fortran support])
  elif test ${__tbx_have_ifort} -eq 0; then
    nmad_fortran_target=ifort
    AC_DEFINE([NMAD_FORTRAN_TARGET_IFORT], 0, [Intel ifort Fortran support])
  fi
else
  AC_DEFINE([NMAD_FORTRAN_TARGET_NONE], 0, [no Fortran support])
fi
AC_MSG_RESULT([${nmad_fortran_target}])
AC_SUBST([nmad_fortran_target])
AC_PROG_FC([], [90])
AC_PROG_F77
dnl -- TODO- check whether tbx and nmad FC match, don't know about F77


dnl -- Mad-MPI
dnl --
AC_ARG_ENABLE(mpi,
	      [AS_HELP_STRING([--enable-mpi], [Enable builtin MPI implementation MadMPI]) ],
	      [ if test $enableval = yes; then
	            AC_DEFINE([MAD_MPI], 1, [enable MadMPI])
                    nmad_enable_mpi=yes
		    nmad_interfaces="${nmad_interfaces} mpi"
		    AC_PADICO_OUT_SH([interfaces/mpi/libexec/madmpi-compile-common.sh])
		    AC_PADICO_OUT_SH([interfaces/mpi/bin/mpirun.madmpi])
		    AC_PADICO_OUT_SH([interfaces/mpi/bin/mpicc.madmpi])
		    AC_PADICO_OUT_SH([interfaces/mpi/bin/mpicxx.madmpi])
		    AC_PADICO_OUT_SH([interfaces/mpi/bin/mpif77.madmpi])
		    AC_PADICO_OUT_SH([interfaces/mpi/bin/mpif90.madmpi])
		    AC_PADICO_OUT_SH([interfaces/mpi/bin/mpiexec-nobind])
		fi
	      ],
	      [
                nmad_enable_mpi=no
	      ])
AC_SUBST([nmad_enable_mpi])

AC_ARG_ENABLE(romio,
	      [AS_HELP_STRING([--enable-romio], [Enable ROMIO MPI I/O implementation]) ],
              [ if test $enableval = yes; then
	      	    nmad_mpi_romio_libs="-lmpio"
                    AC_SUBST([nmad_mpi_romio_libs])
	      	    AC_DEFINE([NMAD_MPI_ENABLE_ROMIO], 1, [enable ROMIO MPI I/O])
                fi
              ],
	      [])



dnl -- nmad content
dnl --

nmad_drivers="${nmad_drivers} self local tcp psm2 ibverbs psp shm mx dcfa cci"
nmad_strategies="${nmad_strategies} default aggreg split_balance decision_tree prio"
nmad_interfaces="${nmad_interfaces} launcher pack sendrecv session connector coll rpc madico"

nmad_alldirs=
for d in ${nmad_drivers}; do
    nmad_alldirs="${nmad_alldirs} drivers/${d}"
done
for s in ${nmad_strategies}; do
    nmad_alldirs="${nmad_alldirs} strategies/strat_${s}"
done
for i in ${nmad_interfaces}; do
    nmad_alldirs="${nmad_alldirs} interfaces/${i}"
done
AC_SUBST(nmad_alldirs)

dnl -- flags to build nmad
dnl --

NMAD_CORE_CPPFLAGS="-DNMAD -DNMAD_BUILD -I${nmad_srcdir}/include -I${nmad_builddir}/include"
for d in ${nmad_alldirs}; do
    if test -r ${nmad_srcdir}/${d}/include; then
       NMAD_CORE_CPPFLAGS="${NMAD_CORE_CPPFLAGS} -I${nmad_srcdir}/${d}/include"
    fi
done
NMAD_CORE_CFLAGS="${tbx_CFLAGS} ${PukABI_CFLAGS} ${Puk_CFLAGS} ${pioman_CFLAGS} ${gtg_CFLAGS}"
NMAD_CORE_LIBS="${tbx_LIBS} ${PukABI_LIBS} ${Puk_LIBS} ${pioman_LIBS} ${numa_LIBS} ${gtg_LIBS} -lrt"

AC_SUBST(NMAD_CORE_CPPFLAGS)
AC_SUBST(NMAD_CORE_CFLAGS)
AC_SUBST(NMAD_CORE_LDFLAGS)
AC_SUBST(NMAD_CORE_LIBS)

dnl -- flags to use an installed nmad
dnl --

NMAD_CFLAGS="-DNMAD -I${nmad_root}/include"
NMAD_LIBS="-L${nmad_root}/lib -Wl,-rpath,${nmad_root}/lib -lnmad ${NMAD_CORE_LIBS}"

AC_SUBST(NMAD_CFLAGS)
AC_SUBST(NMAD_LIBS)

dnl -- output 
dnl --

for d in ${nmad_alldirs}; do
    if test -r ${nmad_srcdir}/${d}/Makefile.in; then
       out_mk="${out_mk} ${d}/Makefile"
    fi
done
for e in `( cd ${nmad_srcdir}; find examples -name Makefile.in )`; do
    out_mk="${out_mk} `echo ${e} | sed -e s/Makefile.in/Makefile/g`"
done

AC_PADICO_OUT_SH([interfaces/sampling/nmad-sample])
out_mk="${out_mk} Makefile examples/Makefile nmad.pc eztrace_nmad_core.tpl doc/Doxyfile"

AC_PADICO_OUT_COMMON_MK
AC_PADICO_OUTPUT

