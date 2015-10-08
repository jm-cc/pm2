dnl ----- configure.ac for mpibench -----

AC_PREREQ(2.50)

AC_INIT(mpi_bench_sendrecv.c)

m4_include([misc.m4])
m4_include([compiler.m4])
m4_include([tools.m4])
m4_include([features.m4])
m4_include([out.m4])
m4_include([ax_mpi.m4])

dnl --- Directories and version
dnl --------------------------------------------------------

AC_PADICO_PACKAGE([mpibenchmark])

dnl --- Compiler and base tools
dnl --------------------------------------------------------

AC_PADICO_GNU_MAKE
AC_PADICO_COMPILER
AC_PADICO_DEP
AC_PADICO_LINKER
AX_MPI

AC_SEARCH_LIBS(pthread_create,  [pthread c], [], [ AC_MSG_ERROR([Cannot find pthread_create() ]) ])
AC_SEARCH_LIBS(clock_gettime,  [rt c], [], [ AC_MSG_ERROR([Cannot find clock_gettime() ]) ])


dnl --- check features
dnl --------------------------------------------------------

AC_PADICO_OPTIMIZE
AC_PADICO_DEBUG


dnl --- output
dnl --------------------------------------------------------

AC_PADICO_OUT_COMMON_MK
AC_PADICO_OUT_ROOT
AC_PADICO_OUTPUT
