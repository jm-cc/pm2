dnl  Process this file with autoconf to produce configure.

AC_INIT([pm2-marcel], [0.1-forestgomp], [pm2-users@lists.gforge.inria.fr])
AC_PREREQ([2.59])

AC_CONFIG_SRCDIR([init/source/init.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADER([init/include/pm2_config.h])

AC_CANONICAL_HOST

dnl The scripts that are instantiated at configure-time.
m4_define([pm2_bin_scripts], [			\
  bin/ezflavor					\
  bin/fut-print.pl				\
  bin/fut-codes.pl				\
  bin/marcel-install				\
  bin/marcel-fut.pl				\
  bin/nmad-fut2nam.pl				\
  bin/nmad-fut2txt.pl				\
  bin/nmad-trace.py				\
  bin/pm2-all					\
  bin/pm2-bench					\
  bin/pm2-build-fut-table			\
  bin/pm2-clean					\
  bin/pm2-conf					\
  bin/pm2-conf-file				\
  bin/pm2-config				\
  bin/pm2-config-flavor				\
  bin/pm2-config-tools.sh			\
  bin/pm2-create-sample-flavors			\
  bin/pm2-flavor				\
  bin/pm2-flavor-diff				\
  bin/pm2-flavor-file				\
  bin/pm2-gen-make.sh				\
  bin/pm2-indent-log.pl				\
  bin/pm2-install.pl				\
  bin/pm2-load					\
  bin/pm2-log2paje				\
  bin/pm2-logload				\
  bin/pm2-logs					\
  bin/pm2-module				\
  bin/pm2-pfmon-analyze				\
  bin/pm2-pre-script.sh				\
  bin/pm2-prof					\
  bin/pm2-split-h-file				\
  bin/pm2-test					\
  bin/pm2-text-config				\
  bin/pm2-topology				\
  bin/pm2-valgrind-load				\
  bin/pm2-vlocallogs				\
  bin/pm2-which					\
  bin/pm2-wrapper				\
  bin/pm2-wrapper-valgrind			\
  bin/pm2-xload					\
])

dnl This must be the first command run.
AC_CONFIG_COMMANDS([pm2-executable-scripts], [chmod +x ]pm2_bin_scripts)

dnl Symlink magic launchers
mkdir -p bin
for i in bubblegum bubbles leonie
do
	ln -sf ezflavor bin/$i
done

for i in pm2-menu-config pm2-xconfig
do
	ln -sf pm2-text-config bin/$i
done

dnl Copy modules links
test -d modules || cp -a $srcdir/modules .

AC_PATH_PROG([BASH], [bash], [not-found])

if test "x$BASH" = "xnot-found"; then
  # If GNU Bash isn't available, use the current shell and hope for
  # the best.
  AC_MSG_WARN([GNU Bash not found, falling back to `$SHELL'])
  BASH="$SHELL"
fi

AC_PATH_PROG([PERL], [perl], [not-found])

if test "x$PERL" = "xnot-found"; then
  AC_MSG_ERROR([Perl was not found.  Please install it first.])
fi

AC_GNU_SOURCE

ACX_GNU_MAKE
ACX_GNU_C
ACX_GNU_C_MAJOR_VERSION
ACX_GNU_LD

AC_SUBST([BASH])
AC_SUBST([PERL])
AC_SUBST([MAKE])

PM2_OPTIONS
PM2_SYS_ARCH
PM2_PROFILE_DEPENDENCIES

m4_include([include.ac])

AC_CONFIG_FILES([GNUmakefile
		 make/apps-vars.mak
		 make/common-vars.mak
	         make/config.mak
	         generic/config/options/Makefile
		 ]
		 pm2_bin_scripts)
AC_CONFIG_HEADERS([init/include/pm2_config_public.h])

AC_OUTPUT
