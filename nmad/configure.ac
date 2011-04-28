dnl -- configure.ac for nmad
dnl -- contains excerpts from PadicoTM configure.ac


AC_PREREQ(2.50)
AC_REVISION($Revision: 3960 $)
AC_INIT(include/nm_public.h)
AC_CONFIG_HEADER(include/nm_private_config.h include/nm_config.h)

m4_include([./m4/pkg.m4])

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


dnl -- tools
dnl --------------------------------------------------------

AC_PATH_PROG([BASH], [bash])
AC_SUBST([BASH])


dnl -- options
dnl --------------------------------------------------------

dnl -- base CFLAGS
dnl --
NMAD_CFLAGS="-DNMAD -DNMAD_AUTOCONF -I${nmad_root}/include"

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
case ${tag_container} in
     TAG_AS_HASHTABLE)  AC_DEFINE([CONFIG_TAG_AS_HASHTABLE], [1], [use 32 bits tags in hashtable]);;
     TAG_HUGE)          AC_DEFINE([CONFIG_TAG_HUGE], [1], [use 64 bits tags in hashtable]);;
     TAG_AS_FLAT_ARRAY) AC_DEFINE([CONFIG_TAG_AS_FLAT_ARRAY], [1], [use 8 bits tags in arrays]);;
esac
AC_MSG_RESULT([${tag_container}])

dnl -- sampling
dnl --
AC_ARG_ENABLE(sampling,
	      [  --enable-sampling  Enable network sampling],
	      [ if test $enableval = yes; then
	            AC_DEFINE([NMAD_SAMPLING], 1, [enable sampling])
	        fi
	      ],
	      [])


dnl -- NUIOA
dnl --
dnl - TODO- enable NUIOA by default if numa.h is present
dnl - TODO- check for numa.h
AC_ARG_ENABLE(nuioa,
	      [  --enable-nuioa     Enable NUMA control on I/O],
	      [ if test $enableval = yes; then
	      	    AC_CHECK_HEADER(numa.h, [], [ AC_MSG_ERROR([numa.h is required for NUIOA]) ])
	            AC_DEFINE([PM2_NUIOA], 1, [enable NUIOA])
		    numa_LIBS="-lnuma"
	        fi
	      ],
	      [])

dnl -- TODO: debug, optimize, fine_grain_lock

dnl -- external packages
dnl --------------------------------------------------------

PKG_PROG_PKG_CONFIG

dnl -- tbx
PKG_CHECK_MODULES([tbx],    [ tbx ])

dnl -- Puk
PKG_CHECK_MODULES([Puk],    [ Puk ])
AC_MSG_CHECKING([for Puk root])
puk_root="`pkg-config --variable=prefix Puk`"
AC_SUBST([puk_root])
AC_MSG_RESULT([${puk_root}])
AC_CHECK_FILE([${puk_root}/etc/common_rules.mk],
	      [], [ AC_MSG_ERROR([required ${puk_root}/etc/common_rules.mk not found]) ])

dnl -- PukABI
PKG_CHECK_MODULES([PukABI], [ PukABI ])

dnl -- PIOMan
AC_ARG_WITH([pioman],
	    [AS_HELP_STRING([--with-pioman], [use pioman I/O manager @<:@default=no@:>@] )])
if test "x${with_pioman}" = "xyes"; then
   PKG_CHECK_MODULES([pioman], [ pioman ])
  requires_pioman="pioman"
else
  requires_pioman=
fi
AC_SUBST([requires_pioman])
AC_SUBST(with_pioman)

dnl -- ibverbs
HAVE_INFINIBAND=no
with_ibverbs=check
AC_ARG_WITH([ibverbs],
	    [AS_HELP_STRING([--with-ibverbs], [use Infiniband ibverbs @<:@default=check@:>@] )])
if test "x${with_ibverbs}" != "xno" ; then
    save_CFLAGS="${CFLAGS}"
    CFLAGS="${CFLAGS} -I${IBHOME}/include"
    AC_CHECK_HEADER(infiniband/verbs.h, 
                    [ HAVE_INFINIBAND=yes ],
                    [ HAVE_INFINIBAND=no  ])
    CFLAGS="${save_CFLAGS}"
    AC_SUBST(HAVE_INFINIBAND)
    if test ${HAVE_INFINIBAND} = yes; then
        AC_DEFINE([CONFIG_IBVERBS], 1, [enable ibverbs support])
        nmad_drivers="${nmad_drivers} ibverbs"
        if test "x$IBHOME" != "x" -a "x${IBHOME}" != "x/usr"; then
      	   if test "x${IBLIBPATH}" = "x" ; then
      	      IBLIBPATH=${IBHOME}/lib
           fi
      	   ibverbs_CLAGS="-I$IBHOME/include"
      	   ibverbs_LIBS="-Wl,-rpath,${IBLIBPATH} -L${IBLIBPATH}"
        fi
	ibverbs_LIBS="${ibverbs_LIBS} -libverbs"
    elif test "x${with_ibverbs}" != "xcheck"; then
        AC_MSG_FAILURE([--with-ibverbs was given, but infiniband/verbs.h is not found])
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
with_mx=check
AC_ARG_WITH([mx],
	    [AS_HELP_STRING([--with-mx], [use Myrinet MX @<:@default=check@:>@] )])
if test "x${with_mx}" != "xno" ; then
    save_CFLAGS="${CFLAGS}"
    CFLAGS="${CFLAGS} -I${MX_DIR}/include"
    AC_CHECK_HEADER(myriexpress.h, 
                    [ HAVE_MX=yes ],
                    [ HAVE_MX=no  ])
    CFLAGS="${save_CFLAGS}"
    AC_SUBST(HAVE_MX)
    if test ${HAVE_MX} = yes; then
        AC_DEFINE([CONFIG_MX], 1, [enable MX support])
        nmad_drivers="${nmad_drivers} mx"
        if test "x${MX_DIR}" != "x" -a "x${MX_DIR}" != "x/usr"; then
      	   mx_CFLAGS="-I${MX_DIR}/include"
      	   mx_LIBS="-Wl,-rpath,${MX_DIR}/lib -L${MX_DIR}/lib"
        fi
	mx_LIBS="${mx_LIBS} -lmyriexpress"
    elif test "x${with_mx}" != "xcheck"; then
        AC_MSG_FAILURE([--with-mx was given, but myrinetexpress.h is not found])
    fi
fi

dnl -- Fortran
AC_MSG_CHECKING([for Fortran support in tbx])
nmad_fortran_target=none
echo $tbx_CFLAGS | grep -q FORTRAN_COMPILER
__tbx_have_fortran=$?
if test ${__tbx_have_fortran} -eq 0; then
  echo $tbx_CFLAGS | grep -q GNU_FORTRAN_COMPILER
  __tbx_have_gfortran=$?
  echo $tbx_CFLAGS | grep -q INTEL_FORTRAN_COMPILER
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
	      [  --enable-mpi       Enable builtin MPI implementation Mad-MPI],
	      [ if test $enableval = yes; then
	            AC_DEFINE([MAD_MPI], 1, [enable Mad-MPI])
		    nmad_interfaces="${nmad_interfaces} mpi"
		    out_mk="${out_mk} examples/mpi/basics/Makefile examples/mpi/benchmarks/Makefile"
		    out_sh="${out_sh} interfaces/mpi/bin/mpirun interfaces/mpi/bin/mpicc interfaces/mpi/bin/mpicxx interfaces/mpi/bin/mpif77 interfaces/mpi/bin/mpif90"
		    if test "x${nmad_target_fortran}" != "xnone"; then
		        out_mk="${out_mk} examples/mpi/fortran/Makefile examples/mpi/f90/Makefile"
		    fi
	        fi
	      ],
	      [])

dnl -- compute flags
NMAD_EXT_CFLAGS="${tbx_CFLAGS} ${PukABI_CFLAGS} ${Puk_CFLAGS} ${pioman_CFLAGS} ${ibverbs_CFLAGS} ${mx_CFLAGS}"
AC_SUBST(NMAD_CFLAGS)
AC_SUBST(NMAD_EXT_CFLAGS)

NMAD_EXT_LIBS="${tbx_LIBS} ${PukABI_LIBS} ${Puk_LIBS} ${pioman_LIBS} ${ibverbs_LIBS} ${mx_LIBS} ${numa_LIBS}"
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


out_sh="${out_sh} bin/nmad-driver-conf bin/nmad-strategy-conf bin/nmad-sample"
out_mk="${out_mk} Makefile examples/sendrecv/Makefile examples/pack/Makefile examples/lowlevel/Makefile nmad.pc"

dnl -- output 

AC_OUTPUT_COMMANDS([chmod +x ${out_sh} ], [ out_sh="${out_sh}" ])
AC_OUTPUT(${out_mk} ${out_sh})
