dnl ----- configure.ac for mpibench -----

AC_PREREQ(2.50)

AC_INIT(autogen.sh)

m4_include([misc.m4])
m4_include([compiler.m4])
m4_include([tools.m4])
m4_include([features.m4])
m4_include([out.m4])
m4_include([ax_mpi.m4])

dnl --- Directories and version
dnl --------------------------------------------------------

AC_PADICO_BUILDDIR
AC_PADICO_SRCDIR

dnl --- Compiler and base tools
dnl --------------------------------------------------------

AC_PADICO_GNU_MAKE
AC_PADICO_COMPILER
AC_PADICO_DEP
AC_PADICO_LINKER
AX_MPI


dnl --- output
dnl --------------------------------------------------------

AC_PADICO_OUT_COMMON_MK(./building-tools)
AC_PADICO_OUT_ROOT
AC_PADICO_OUTPUT
