

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

LIBNAME=bubblelib
FLAVOR_DEFAULT:=marcel-bubbles
CFLAGS += -I${MOD_GEN_INC} -I/usr/local/include

include $(PM2_ROOT)/make/libs-vars.mak

# The profile.c file includes fut_entries.h, but it cannot be actually
# detected by make depend since there would be a circular dependency
# problem...
ifneq ($(MAKECMDGOALS),fut)
$(MOD_GEN_OBJ)/bubblelib_fxt.o: $(MOD_GEN_INC)/fut_entries.h
endif

.PHONY: default libs

libs: $(MOD_GEN_INC)/fut_entries.h $(MOD_GEN_INC)/ming-version.h


$(MOD_GEN_INC)/fut_entries.h: $(MOD_GEN_STAMP)/fut_stamp ${PM2_ROOT}/bin/pm2-build-fut-table
	$(COMMON_BUILD)
	$(COMMON_MAIN) ${PM2_ROOT}/bin/pm2-build-fut-table -f $(FLAVOR) -m bubblelib

$(MOD_GEN_STAMP)/fut_stamp: $(MOD_STAMP_FLAVOR)
	$(COMMON_BUILD)
	$(COMMON_MAIN) touch $@

# Ming's `MING_VERSION' macro isn't usable from the C preprocessor, so
# generate something more convenient.
$(MOD_GEN_INC)/ming-version.h:
	@if pkg-config libming; then						\
	  full_version="`pkg-config libming --modversion`";			\
	  major="`echo $$full_version | cut -d . -f 1`";			\
	  minor="`echo $$full_version | cut -d . -f 2`";			\
	  micro="`echo $$full_version | cut -d . -f 3`";			\
	else									\
	  major="0";								\
	  minor="3";								\
	  micro="0";								\
	  echo "WARNING: assuming Ming version $$major.$$minor.$$micro";	\
	fi;									\
	( echo "/* Automatically generated, do not edit.  */";			\
	  echo "#ifndef BUBBLES_MING_VERSION_H";				\
	  echo "# define BUBBLES_MING_VERSION_H";				\
	  echo "# define BUBBLES_MING_VERSION_MAJOR $$major";			\
	  echo "# define BUBBLES_MING_VERSION_MINOR $$minor";			\
	  echo "# define BUBBLES_MING_VERSION_MICRO $$micro";			\
	  echo "#endif" ) > "$@"

include $(PM2_ROOT)/make/libs-rules.mak
