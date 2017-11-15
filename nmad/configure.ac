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

dnl -- directories
dnl --------------------------------------------------------

AC_PADICO_PACKAGE([NewMadeleine], [newmadeleine])

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
AC_ARG_WITH([pukabi],
	    [AS_HELP_STRING([--with-pukabi], [use PukABI ABI interceptor @<:@default=yes@:>@] )])
AC_MSG_CHECKING([for PukABI])
if test "x${with_pukabi}" = "xno"; then
  AC_MSG_RESULT([no])
else
  AC_MSG_RESULT([yes])
  PKG_CHECK_MODULES([PukABI], [ PukABI ])
  requires_pukabi="PukABI"
  AC_DEFINE([NMAD_PUKABI], [1], [interface with PukABI interposer])
fi
AC_SUBST([requires_pukabi])
AC_SUBST(with_pukabi)

dnl -- PIOMan
AC_ARG_WITH([pioman],
	    [AS_HELP_STRING([--with-pioman], [use pioman I/O manager @<:@default=no@:>@] )])
AC_MSG_CHECKING([for PIOMan])
if test "x${with_pioman}" = "xyes"; then
  AC_MSG_RESULT([yes])
  PKG_CHECK_MODULES([pioman], [ pioman ])
  requires_pioman="pioman"
  AC_DEFINE([NMAD_PIOMAN], [1], [use Pioman progression engine])
else
  AC_MSG_RESULT([no])
  requires_pioman=
fi
AC_SUBST([requires_pioman])
AC_SUBST(with_pioman)

dnl -- ibverbs
AC_PADICO_CHECK_INFINIBAND

dnl -- dcfa
HAVE_DCFA=no
AC_ARG_WITH([dcfa],
	    [AS_HELP_STRING([--with-dcfa], [use Direct Communication Facility for Accelerators @<:@default=check@:>@])],
	    [], [ with_dcfa=check ])
if test "x${with_dcfa}" != "xno" ; then
    save_CPPFLAGS="${CPPFLAGS}"
    if test "x${with_dcfa}" != "xyes"; then
        DCFA_DIR="${with_dcfa}"
	CPPFLAGS="${CPPFLAGS} -I${DCFA_DIR}/include"
    fi
    AC_CHECK_HEADER(dcfa.h, 
                    [ HAVE_DCFA=yes ],
                    [ HAVE_DCFA=no  ])
    CPPFLAGS="${save_CPPFLAGS}"
    AC_SUBST(HAVE_DCFA)
    if test ${HAVE_DCFA} = yes; then
        nmad_drivers="${nmad_drivers} dcfa"
        if test "x${DCFA_DIR}" != "x" -a "x${DCFA_DIR}" != "x/usr"; then
      	   dcfa_CPPFLAGS="-I${DCFA_DIR}/include"
      	   dcfa_LIBS="-Wl,-rpath,${DCFA_DIR}/lib -L${DCFA_DIR}/lib"
        fi
	dcfa_LIBS="${dcfa_LIBS} -ldcfa"
	AC_SUBST([dcfa_CPPFLAGS])
	AC_SUBST([dcfa_LIBS])
    elif test "x${with_dcfa}" != "xcheck"; then
        AC_MSG_FAILURE([--with-dcfa was given, but dcfa.h is not found])
    fi
fi

dnl -- MX
if test "x$MX_DIR" = "x" ; then
    if test "x${MX_ROOT}" = "x"; then
        if test -r /opt/mx/include/myriexpress.h ; then
    	    MX_DIR=/opt/mx
        fi
    else
	MX_DIR=${MX_ROOT}
    fi
fi
HAVE_MX=no
with_mx=no
AC_ARG_WITH([mx],
	    [AS_HELP_STRING([--with-mx], [use Myrinet MX @<:@default=no@:>@] )])
if test "x${with_mx}" != "xno" ; then
    save_CPPFLAGS="${CPPFLAGS}"
    CPPFLAGS="${CPPFLAGS} -I${MX_DIR}/include"
    AC_CHECK_HEADER(myriexpress.h, 
                    [ HAVE_MX=yes ],
                    [ HAVE_MX=no  ])
    CPPFLAGS="${save_CPPFLAGS}"
    AC_SUBST(HAVE_MX)
    if test ${HAVE_MX} = yes; then
        nmad_drivers="${nmad_drivers} mx"
        if test "x${MX_DIR}" != "x" -a "x${MX_DIR}" != "x/usr"; then
      	   mx_CPPFLAGS="-I${MX_DIR}/include"
      	   mx_LIBS="-Wl,-rpath,${MX_DIR}/lib -L${MX_DIR}/lib"
        fi
	mx_LIBS="${mx_LIBS} -lmyriexpress"
	AC_SUBST([mx_CPPFLAGS])
	AC_SUBST([mx_LIBS])
    elif test "x${with_mx}" != "xcheck"; then
        AC_MSG_FAILURE([--with-mx was given, but myrinetexpress.h is not found])
    fi
fi

dnl -- CCI
HAVE_CCI=no
AC_ARG_WITH([cci],
	    [AS_HELP_STRING([--with-cci], [use Common Communication Interface (CCI) @<:@default=check@:>@] )],
	    [], [ with_cci=check ])
if test "x${with_cci}" != "xno" ; then
    save_CPPFLAGS="${CPPFLAGS}"
    if test "x${with_cci}" != "xyes"; then
        CCI_DIR="${with_cci}"
	CPPFLAGS="${CPPFLAGS} -I${CCI_DIR}/include"
    fi
    AC_CHECK_HEADER(cci.h, 
                    [ HAVE_CCI=yes ],
                    [ HAVE_CCI=no  ])
    CPPFLAGS="${save_CPPFLAGS}"
    AC_SUBST(HAVE_CCI)
    if test ${HAVE_CCI} = yes; then
        nmad_drivers="${nmad_drivers} cci"
        if test "x${CCI_DIR}" != "x" -a "x${CCI_DIR}" != "x/usr"; then
      	   cci_CPPFLAGS="-I${CCI_DIR}/include"
      	   cci_LIBS="-Wl,-rpath,${CCI_DIR}/lib -L${CCI_DIR}/lib"
        fi
	cci_LIBS="${cci_LIBS} -lcci"
	AC_SUBST([cci_CPPFLAGS])
	AC_SUBST([cci_LIBS])
    elif test "x${with_cci}" != "xcheck"; then
        AC_MSG_FAILURE([--with-cci was given, but cci.h is not found])
    fi
fi

dnl -- PSM
HAVE_PSM=no
AC_ARG_WITH([psm],
	    [AS_HELP_STRING([--with-psm], [use Intel Performance Scaled Messasging (PSM) @<:@default=check@:>@] )],
	    [], [ with_psm=check ])
if test "x${with_psm}" != "xno"; then
    save_CPPFLAGS="${CPPFLAGS}"
    if test "x${with_psm}" != "xyes" -a "x${with_psm}" != "xcheck"; then
        PSM_DIR="${with_psm}"
      	CPPFLAGS="${CPPFLAGS} -I${PSM_DIR}/include"
    fi
    AC_CHECK_HEADER(psm.h, [ HAVE_PSM_H=yes ], [ HAVE_PSM_H=no ])
    AC_SEARCH_LIBS(psm_init, [ psm psm_infinipath ], [ HAVE_LIBPSM=yes ], [ HAVE_LIBPSM=no ])
    psm_lib="${ac_cv_search_psm_init}"
    if test ${HAVE_PSM_H} = yes -a ${HAVE_LIBPSM} = yes; then
        HAVE_PSM=yes
    else
	HAVE_PSM=no
    fi
    AC_SUBST(HAVE_PSM)
    CPPFLAGS="${save_CPPFLAGS}"
    if test ${HAVE_PSM} = yes; then
	if test "x${PSM_DIR}" != "x" -a "x${PSM_DIR}" != "x/usr"; then
	    psm_CPPFLAGS="-I${PSM_DIR}/include"
	    psm_LIBS="-Wl,-rpath,${PSM_DIR}/lib -L${PSM_DIR}/lib"
	fi
	psm_LIBS="${psm_LIBS} ${psm_lib}"
	AC_SUBST([psm_CPPFLAGS])
	AC_SUBST([psm_LIBS])
    elif test "x${with_psm}" != "xcheck"; then
    	AC_MSG_FAILURE([--with-psm was given but psm.h is not found.])
    fi
fi

dnl -- PSM2
HAVE_PSM2=no
AC_ARG_WITH([psm2],
	    [AS_HELP_STRING([--with-psm2], [use Intel Performance Scaled Messasging 2 (PSM2) @<:@default=check@:>@] )],
	    [], [ with_psm2=check ])
if test "x${with_psm2}" != "xno"; then
    save_CPPFLAGS="${CPPFLAGS}"
    if test "x${with_psm2}" != "xyes" -a "x${with_psm2}" != "xcheck"; then
        PSM2_DIR="${with_psm2}"
      	CPPFLAGS="${CPPFLAGS} -I${PSM2_DIR}/include"
    fi
    AC_CHECK_HEADER(psm2.h, [ HAVE_PSM2_H=yes ], [ HAVE_PSM2_H=no ])
    AC_CHECK_LIB(psm2, psm2_init, [ HAVE_LIBPSM2=yes ], [ HAVE_LIBPSM2=no ])
    if test ${HAVE_PSM2_H} = yes -a ${HAVE_LIBPSM2} = yes; then
        HAVE_PSM2=yes
    else
	HAVE_PSM2=no
    fi
    AC_SUBST(HAVE_PSM2)
    CPPFLAGS="${save_CPPFLAGS}"
    if test ${HAVE_PSM2} = yes; then
	if test "x${PSM2_DIR}" != "x" -a "x${PSM2_DIR}" != "x/usr"; then
	    psm2_CPPFLAGS="-I${PSM2_DIR}/include"
	    psm2_LIBS="-Wl,-rpath,${PSM2_DIR}/lib -L${PSM2_DIR}/lib"
	fi
	psm2_LIBS="${psm2_LIBS} -lpsm2"
	AC_SUBST([psm2_CPPFLAGS])
	AC_SUBST([psm2_LIBS])
    elif test "x${with_psm2}" != "xcheck"; then
    	AC_MSG_FAILURE([--with-psm2 was given but psm2.h is not found.])
    fi
fi

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

nmad_drivers="${nmad_drivers} self local tcp psm2 ibverbs"
nmad_strategies="${nmad_strategies} default aggreg split_balance decision_tree prio"
nmad_interfaces="${nmad_interfaces} launcher pack sendrecv session connector coll rpc"

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

