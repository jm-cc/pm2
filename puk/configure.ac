dnl ----- configure.ac for Puk in PM2 -----

AC_PREREQ(2.50)

AC_INIT(Puk/Puk.h)
AC_CONFIG_HEADER(Puk/padico_config.h)

m4_include([building-tools/misc.m4])
m4_include([building-tools/compiler.m4])
m4_include([building-tools/tools.m4])
m4_include([building-tools/abimagic.m4])
m4_include([building-tools/features.m4])
m4_include([building-tools/out.m4])

AC_PADICO_TOOLS_DIR([building-tools])

dnl --- Directories and version
dnl --------------------------------------------------------

AC_PADICO_BUILDDIR
AC_PADICO_SRCDIR
AC_PADICO_DIRS
AC_PADICO_VERSION

dnl --- Compiler and base tools
dnl --------------------------------------------------------

AC_PADICO_BASE_TOOLS
AC_PADICO_GNU_MAKE
AC_PADICO_COMPILER
AC_PADICO_GCC_VERSION
AC_PADICO_GXX_VERSION
AC_PADICO_DEP
AC_PADICO_LINKER
AC_PADICO_DYNLD
AC_PADICO_DSO
AC_PADICO_EXPAT

dnl --- ABI black magic
dnl --------------------------------------------------------

AC_PADICO_PUKABI
if test "$PADICO_ENABLE_PUKABI" = "yes"; then
  AC_PADICO_GXX_ABI
  AC_PADICO_ERRNO
  AC_PADICO_NOSTDLIB
  AC_PADICO_ATOSO
  AC_PADICO_WRAPPING
  AC_PADICO_OBJCOPY
  AC_PADICO_ELFWRAP
  AC_PADICO_LIBRESOLV
  AC_PADICO_LIBCMAGIC
fi

dnl --- check features
dnl --------------------------------------------------------

AC_PADICO_TYPES
AC_PADICO_LIBS
AC_PADICO_HEADERS
AC_PADICO_INFINIBAND

AC_PADICO_OPTIMIZE
AC_PADICO_DEBUG
AC_PADICO_TRACE
AC_PADICO_DISABLE_OUTPUT
AC_PADICO_PM2

dnl --- output
dnl --------------------------------------------------------

AC_PADICO_OUT_COMMON
AC_PADICO_OUT_PUK
#AC_PADICO_OUT_MODULES
#AC_PADICO_OUT_ROOT
AC_PADICO_OUTPUT

