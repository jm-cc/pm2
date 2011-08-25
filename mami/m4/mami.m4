# MaMI --- NUMA Memory Interface
#
# Copyright (C) 2011  Centre National de la Recherche Scientifique
#
# MaMI is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or (at
# your option) any later version.
#
# MaMI is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# See the GNU Lesser General Public License in COPYING.LGPL for more details.


# CHECK_HWLOC_PKG
# ---------------
AC_DEFUN([CHECK_HWLOC_PKG],
[
	PKG_CHECK_EXISTS(hwloc, __with_hwloc=yes, __with_hwloc=no)
	if test x$__with_hwloc = xyes; then
		ADD_TO(GLOBAL_AM_CFLAGS, `pkg-config hwloc --cflags`)
		ADD_TO(LIBS, `pkg-config hwloc --libs`)
		ADD_TO(REQUIRED_PKG, hwloc)
	fi

	AC_MSG_NOTICE(whether hwloc is available: $__with_hwloc)
])

# DEP_TBX_WITH_TOPO(MINVER)
# -------------------------
# Check if TBX was built with VER(hwloc) >= MINVER
AC_DEFUN([DEP_TBX_WITH_TOPO],
[
	__hwloc_minver=$1

	# check tbx lib
	DEP_PKG(tbx, TBX)

	# check if tbx include topology support
	echo $GLOBAL_AM_CPPFLAGS | grep -q PM2_TOPOLOGY
	if test $? -ne 0; then
	   PRINT_ERROR(Please install hwloc >= $__hwloc_minver and rebuild the TBX library)
	fi

	# check hwloc version
	AC_MSG_CHECKING(whether installed hwloc version >= $__hwloc_minver)
	PKG_CHECK_EXISTS([hwloc >= $__hwloc_minver], 
			 [AC_MSG_RESULT(yes)],
		  	 [AC_MSG_RESULT(no); 
			  PRINT_ERROR(Please install hwloc >= $__hwloc_minver and rebuilt TBX)])
])

