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

AC_PATH_PROG([BASH], [bash])
AC_SUBST([BASH])


dnl -- options
dnl --------------------------------------------------------

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
		    nmad_interfaces="${nmad_interfaces} sampling"
	        fi
	      ],
	      [])


dnl -- NUIOA
dnl --
dnl - TODO- enable NUIOA by default if numa.h is present
AC_ARG_ENABLE(nuioa,
	      [  --enable-nuioa     Enable NUMA control on I/O],
	      [ if test $enableval = yes; then
	      	    AC_CHECK_HEADER(numa.h, [], [ AC_MSG_ERROR([numa.h is required for NUIOA]) ])
		    AC_SEARCH_LIBS(numa_available, numa, [], [ AC_MSG_ERROR([Cannot find working libnuma ]) ])
	            AC_DEFINE([PM2_NUIOA], 1, [enable NUIOA])
		    numa_LIBS="-lnuma"
	        fi
	      ],
	      [])

dnl -- DEBUG
AC_ARG_ENABLE(debug,
	      [  --enable-debug     Enable debugging symbols],
	      [ if test $enableval = yes; then
	      	    NMAD_ENABLE_DEBUG=yes
		    AC_SUBST(NMAD_ENABLE_DEBUG)
	        fi
	      ],
	      [])


dnl -- OPTIMIZE
AC_ARG_ENABLE(optimize,
	      [  --enable-optimize  Enable compiler optimization flags],
	      [ if test $enableval = yes; then
	      	    NMAD_ENABLE_OPTIMIZE=yes
		    AC_SUBST(NMAD_ENABLE_OPTIMIZE)
	        fi
	      ],
	      [])
dnl -- TODO: fine_grain_lock


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
AC_MSG_CHECKING([for PIOMan])
if test "x${with_pioman}" = "xyes"; then
  AC_MSG_RESULT([yes])
  PKG_CHECK_MODULES([pioman], [ pioman ])
  requires_pioman="pioman"
  AC_MSG_CHECKING([whether PIOMan is linked against Marcel.])
  has_marcel="`pkg-config --cflags pioman | grep -i PIOMAN_MARCEL`"
  if test "x${has_marcel}" != "x" ; then
     nmad_examples="${nmad_examples} piom"
     AC_MSG_RESULT([yes])
  else
     AC_MSG_RESULT([no])
  fi
else
  AC_MSG_RESULT([no])
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
        nmad_drivers="${nmad_drivers} ibverbs"
        if test "x$IBHOME" != "x" -a "x${IBHOME}" != "x/usr"; then
      	   if test "x${IBLIBPATH}" = "x" ; then
      	      IBLIBPATH=${IBHOME}/lib
           fi
      	   ibverbs_CLAGS="-I$IBHOME/include"
      	   ibverbs_LIBS="-Wl,-rpath,${IBLIBPATH} -L${IBLIBPATH}"
        fi
	ibverbs_LIBS="${ibverbs_LIBS} -libverbs"
	AC_SUBST([ibverbs_CFLAGS])
	AC_SUBST([ibverbs_LIBS])
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
        nmad_drivers="${nmad_drivers} mx"
        if test "x${MX_DIR}" != "x" -a "x${MX_DIR}" != "x/usr"; then
      	   mx_CFLAGS="-I${MX_DIR}/include"
      	   mx_LIBS="-Wl,-rpath,${MX_DIR}/lib -L${MX_DIR}/lib"
        fi
	mx_LIBS="${mx_LIBS} -lmyriexpress"
	AC_SUBST([mx_CFLAGS])
	AC_SUBST([mx_LIBS])
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
	      [  --enable-mpi       Enable builtin MPI implementation Mad-MPI],
	      [ if test $enableval = yes; then
	            AC_DEFINE([MAD_MPI], 1, [enable Mad-MPI])
		    nmad_interfaces="${nmad_interfaces} mpi"
		    nmad_examples="${nmad_examples} mpi/basics mpi/benchmarks"
		    out_sh="${out_sh} interfaces/mpi/bin/mpirun interfaces/mpi/bin/mpicc interfaces/mpi/bin/mpicxx interfaces/mpi/bin/mpif77 interfaces/mpi/bin/mpif90"
		    if test "x${nmad_target_fortran}" != "xnone"; then
		        nmad_examples="${nmad_examples} mpi/fortran mpi/f90"
		    fi
	        fi
	      ],
	      [])


dnl -- nmad content
dnl --

nmad_drivers="${nmad_drivers} self local tcp"
nmad_strategies="${nmad_strategies} default aggreg aggreg_extended aggreg_autoextended split_balance"
nmad_interfaces="${nmad_interfaces} launcher pack sendrecv session"
nmad_examples="pack sendrecv lowlevel session ${nmad_examples}"

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
AC_SUBST(nmad_examples)

dnl -- flags to use an installed nmad
dnl --

NMAD_CFLAGS="-DNMAD -I${nmad_root}/include"

NMAD_LIBS="-L${nmad_root}/lib -Wl,-rpath,${nmad_root}/lib -lnmad"

AC_SUBST(NMAD_CFLAGS)
AC_SUBST(NMAD_LIBS)

dnl -- flags to build nmad
dnl --

NMAD_LDFLAGS="${NMAD_CORE_LDFLAGS} -Wl,--export-dynamic"

NMAD_CORE_CFLAGS="\$(BASE_CFLAGS) -DNMAD -I${nmad_srcdir}/include -I${nmad_builddir}/include"
if test "x${NMAD_ENABLE_OPTIMIZE}" = "xyes"; then
   NMAD_CORE_CFLAGS="${NMAD_CORE_CFLAGS} \$(OPT_CFLAGS)"
   NMAD_CORE_LDFLAGS="${NMAD_CORE_LDFLAGS} \$(OPT_LDFLAGS)"
fi
if test "x${NMAD_ENABLE_DEBUG}" = "xyes"; then
   NMAD_CORE_CFLAGS="${NMAD_CORE_CFLAGS} \$(DEBUG_CFLAGS)"
fi
for d in ${nmad_alldirs}; do
    if test -r ${nmad_srcdir}/${d}/include; then
       NMAD_CORE_CFLAGS="${NMAD_CORE_CFLAGS} -I${nmad_srcdir}/${d}/include"
    fi
done
NMAD_CORE_CFLAGS="${NMAD_CORE_CFLAGS} ${tbx_CFLAGS} ${PukABI_CFLAGS} ${Puk_CFLAGS} ${pioman_CFLAGS}"

NMAD_CORE_LIBS="${tbx_LIBS} ${PukABI_LIBS} ${Puk_LIBS} ${pioman_LIBS} ${numa_LIBS} -lrt"

AC_SUBST(NMAD_CORE_CFLAGS)
AC_SUBST(NMAD_CORE_LDFLAGS)
AC_SUBST(NMAD_CORE_LIBS)


dnl -- output 
dnl --

for d in ${nmad_alldirs}; do
    if test -r ${nmad_srcdir}/${d}/Makefile.in; then
       out_mk="${out_mk} ${d}/Makefile"
    fi
done
for e in ${nmad_examples}; do
    if test -r ${nmad_srcdir}/examples/${e}/Makefile.in; then
       out_mk="${out_mk} examples/${e}/Makefile"
    fi
done

out_sh="${out_sh} interfaces/sampling/nmad-sample"
out_mk="${out_mk} Makefile examples/Makefile nmad.pc doc/Doxyfile"


AC_OUTPUT_COMMANDS([chmod +x ${out_sh} ], [ out_sh="${out_sh}" ])
AC_OUTPUT(${out_mk} ${out_sh})

