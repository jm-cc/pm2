#
# PM2: Parallel Multithreaded Machine
# Copyright (C) 2010 "the PM2 team" (see AUTHORS file)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#


# PUSH_BUILDENV()
# ---------------
# Push the current build environment
AC_DEFUN([PUSH_BUILDENV],
[
	__CPPFLAGS_save=
	__CFLAGS_save=
	__LDFLAGS_save="$LDFLAGS"
])


# POP_BUILDENV()
# --------------
# Pop the precedently pushed build environment
AC_DEFUN([POP_BUILDENV],
[
	CPPFLAGS="$__CPPFLAGS_save"
	CFLAGS="$__CFLAGS_save"
	LDFLAGS="$__LDFLAGS_save"
])


# ADD_TO(FLAG, OPTION_TO_ADD)
# --------------------------------
# Same as FLAG = $FLAGS OPTION_TO_ADD
AC_DEFUN([ADD_TO],
[
	$1="$$1 $2"
])


# PRINT_ERROR(message)
# --------------------
# Print the error message and add newlines for better visibility
AC_DEFUN([PRINT_ERROR],
[
	printf "\n"
	AC_MSG_ERROR($1)
])


# PRINT_WARNING(message)
# -------------------
# Print the warning, add stars for better visibility
AC_DEFUN([PRINT_WARNING],
[
	AC_MSG_RESULT(*** WARNING ****** $1)
])


# OPTION_ENABLE(ARGNAME, HELP, CONFIG) 
# ----------------------------------------
# Convenience wrapper over AC_ARG_ENABLE with the option deactivated by default
AC_DEFUN([OPTION_ENABLE],
[
	AC_MSG_CHECKING($3)
	AC_ARG_ENABLE($1, AC_HELP_STRING(--enable-$1, $2), __opt_$1=$enableval, __opt_$1=no)

	if test x$__opt_$1 = xyes ; then
	  AC_MSG_RESULT(yes)
	else
	  AC_MSG_RESULT(no)
	fi
])


# OPTION_DISABLE(ARGNAME, HELP, CONFIG) 
# ----------------------------------------
# Convenience wrapper over AC_ARG_ENABLE with the option activated by default
AC_DEFUN([OPTION_DISABLE],
[
	AC_MSG_CHECKING($3)
	AC_ARG_ENABLE($1, AC_HELP_STRING(--disable-$1, $2), __opt_$1=yes, __opt_$1=no)

	if test x$__opt_$1 = xyes ; then
	  AC_MSG_RESULT(yes)
	else
	  AC_MSG_RESULT(no)
	fi
])


# OPTION_CHECK_ONEOFTHEM(optname, options)
# ----------------------------------------
# check if at least one option specified in the list 'options' is enabled
AC_DEFUN([OPTION_CHECK_ONEOFTHEM],
[
	__one_opt_is_set=no
	for optname in $2
	do
		optstatus=`eval "echo $"__opt_$optname""`
		if test x$optstatus = xyes; then
		   __one_opt_is_set=yes
		fi
	done

	if test $__one_opt_is_set = no; then
	   PRINT_ERROR(please use a valid argument in $2 for $1)
	fi
])


# OPTION_CHECK_DEPEND(ARGNAME, dependencies, conflicts)
# --------------------------------------------------
# check dependencies with other options and possible conflicts
# !! use echo to get a and b instead of "a b" ...
# !! some shell does not support for i in ; do echo $i; done
AC_DEFUN([OPTION_CHECK_DEPEND],
[
	module_optname=$__opt_$1

	if test x$module_optname = xyes; then
	   	# check conflicts
	   	for conflict in `echo "$3"`
		do
			optstatus=`eval "echo $"__opt_$conflict""`
			if test x$optstatus = xyes; then
		   	   	PRINT_ERROR(Conflicts between $1 and $conflict)
			fi
		done

		## check dependencies
	   	for depend in `echo "$2"`
		do
			optstatus=`eval "echo $"__opt_$depend""`
			if test x$optstatus != xyes; then
		   	   	PRINT_ERROR($depend option must be activated)
			fi
		done
	fi
])


# OPTION_APPLY(ARGNAME, action)
# --------------------------
AC_DEFUN([OPTION_APPLY],
[
	module_optname=$__opt_$1

	if test x$module_optname = xyes; then
	   $2
	fi
])


# OPTION_WITH(ARGNAME, ARGSTRING, DESC, MSG, DEFAULTVAL)
# ------------------------------------------------------
# Convenience wrapper over AC_ARG_WITH
AC_DEFUN([OPTION_WITH],
[
	AC_ARG_WITH($1, AC_HELP_STRING(--with-$1=<$2 : default=$5>, $3), __with_$1=$withval, __with_$1=$5)

	if test $__with_$1 = $5; then
	   	echo $5 | grep -q -
	   	if test $? -ne 0; then
		   __opt_$5=yes
		fi
	   	PRINT_WARNING(set $4 to... $__with_$1)
	else
	   	echo $withval | grep -q -
	   	if test $? -ne 0; then
		   eval __opt_$withval=yes
		fi
		AC_MSG_RESULT(set $4 to... $__with_$1)
	fi
])


# OPTION_SET_CONST(ARGNAME, DEFINENAME)
# -------------------------------------
# Add a CPPFLAGS -DDEFINENAME=ARGNAME_value
AC_DEFUN([OPTION_SET_CONST],
[
	if test x$__with_$1 != xno; then
		ADD_TO(GLOBAL_AM_CPPFLAGS, -D$2=$__with_$1)
	fi
])


AC_DEFUN([OPTION_SET_POSITIVE_CONST],
[
	if test x$__with_$1 != xno && test $__with_$1 -gt 0 ; then
		ADD_TO(GLOBAL_AM_CPPFLAGS, -D$2=$__with_$1)
	else
		PRINT_ERROR($1 value must be positive)
	fi
])