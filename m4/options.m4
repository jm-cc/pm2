dnl -*- Autoconf -*-
dnl
dnl PM2: Parallel Multithreaded Machine
dnl Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or (at
dnl your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl General Public License for more details.

dnl PM2_DETERMINE_DEFAULT_FLAVOR VAR
dnl
dnl Set VAR to the "preferred" default flavor for this platform.
dnl Here, "preferred" means "best for ForestGOMP".
dnl FIXME: make it "best for what is available"
AC_DEFUN([PM2_DETERMINE_DEFAULT_FLAVOR], [

  dnl Call the generated macro that fills in $PM2_FLAVOR_CONTENT.
  AC_REQUIRE([PM2_FLAVOR_OPTIONS])

  if test "x$PM2_FLAVOR_CONTENT" != "x"; then
     dnl Use the flavor named `default', whose parameters will be
     dnl taken from $PM2_FLAVOR_CONTENT.
     AC_MSG_CHECKING([for compilation options])
     AC_MSG_RESULT([$PM2_FLAVOR_CONTENT])
     AC_MSG_CHECKING([for default flavor])
     $1="default"
  else
      AC_REQUIRE([AC_CANONICAL_HOST])

      dnl XXX: We know that PukABI requires GDB and Expat, and the
      dnl `libpthread' requires PukABI.  This hack checks in advance
      dnl whether these are available, to avoid delaying the failure until
      dnl `make' is invoked (`make' runs Puk's `configure').
      AC_PATH_PROG([GDB], [gdb])
      if test "x$GDB" = "x"; then
	 have_gdb="no"
      else
	 have_gdb="yes"
      fi
      AC_CHECK_HEADERS([expat.h], [have_expat="yes"], [have_expat="no"])

# FIXME: choose according to available modules
      AC_MSG_CHECKING([for the best default flavor])
      case "$host--$have_gdb-$have_expat" in
	*-linux-gnu--yes-yes)
	  $1="libpthread"
	  ;;
	*)
	  $1="pm2"
	  ;;
      esac
   fi
   AC_MSG_RESULT([$]$1)])

dnl PM2_OPTIONS
dnl
dnl Produce `configure' options relevant to PM2.
AC_DEFUN([PM2_OPTIONS], [

  dnl PM2 is currently not installable, so make it clear.
  AC_PREFIX_DEFAULT([/nowhere])
  if test "x$prefix" != "xNONE"; then
     AC_MSG_ERROR([`--prefix' and "make install" are not supported; instead, build products are stored under `$srcdir/build'.])
  fi

  AC_ARG_WITH([default-flavor],
    [AS_HELP_STRING([--with-default-flavor=FLAVOR], [use FLAVOR as the default PM2 flavor])],
    [PM2_DEFAULT_FLAVOR="$withval"],
    [PM2_DETERMINE_DEFAULT_FLAVOR([PM2_DEFAULT_FLAVOR])])
  AC_SUBST([PM2_DEFAULT_FLAVOR])

  if test "x$PM2_HOME" != "x"; then
    AC_MSG_WARN([The `PM2_HOME' environment variable is defined ($PM2_HOME) but it will be ignored.])
  fi
  unset PM2_HOME

  AC_ARG_WITH([home],
    [AS_HELP_STRING([--with-home=DIRECTORY],
      [use DIRECTORY as the PM2 home, where flavor configuration is stored])],
    [PM2_HOME="$withval"],
    [PM2_HOME="${PM2_HOME:-$PWD/build/home}"])

  AC_MSG_CHECKING([where to store flavor configuration (PM2_HOME)])
  if ! test -d "$PM2_HOME"; then
     mkdir -p "$PM2_HOME"
  fi
  AC_MSG_RESULT([$PM2_HOME])

  export PM2_HOME
  AC_SUBST([PM2_HOME])


  AC_CONFIG_COMMANDS([pm2-create-flavor], [
    if test "x$PM2_FLAVOR_CONTENT" != "x"; then

      AC_MSG_NOTICE([configuration options `$PM2_CONFIG_OPTIONS'])
      # Create a new flavor based on the user's `--enable-' options.
      AC_MSG_NOTICE([creating flavor `$PM2_DEFAULT_FLAVOR' with `$PM2_FLAVOR_CONTENT'])
      if "${ac_top_builddir}./bin/pm2-flavor" set --flavor="$PM2_DEFAULT_FLAVOR" \
          $PM2_FLAVOR_CONTENT
      then
	AC_MSG_NOTICE([flavor `$PM2_DEFAULT_FLAVOR' created])
      else
	AC_MSG_WARN([Flavor `$PM2_DEFAULT_FLAVOR' could not be created.])
      fi

    else

      # Use a pre-existing flavor.
      AC_MSG_CHECKING([whether flavor `$PM2_DEFAULT_FLAVOR' already exists'])
      if "${ac_top_builddir}./bin/pm2-flavor" check --flavor="$PM2_DEFAULT_FLAVOR" >/dev/null 2>&1; then
	AC_MSG_RESULT([yes])
      else
	AC_MSG_RESULT([no])
	AC_MSG_NOTICE([creating default flavor `$PM2_DEFAULT_FLAVOR'...])
	if "$ac_abs_top_builddir/bin/pm2-create-sample-flavors" -f "$PM2_DEFAULT_FLAVOR"; then
	  AC_MSG_NOTICE([flavor `$PM2_DEFAULT_FLAVOR' created])
	else
	  sample_flavors="`"$ac_abs_top_builddir/bin/pm2-create-sample-flavors" -l`"
	  AC_MSG_WARN([Flavor `$PM2_DEFAULT_FLAVOR' could not be created.])
	  AC_MSG_WARN([Please choose a flavor among the following: $sample_flavors.])
	fi
      fi

    fi],
    [PM2_DEFAULT_FLAVOR="$PM2_DEFAULT_FLAVOR"
     PM2_CONFIG_OPTIONS="$PM2_CONFIG_OPTIONS"
     PM2_FLAVOR_CONTENT="$PM2_FLAVOR_CONTENT"
     PM2_HOME="$PM2_HOME"])])
