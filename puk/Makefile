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


include $(PM2_ROOT)/make/common-vars.mak

PUK_ROOT       := $(shell pm2-config --builddir)
PUK_SRCDIR     := $(CURDIR)
PUK_BUILDDIR   := $(PUK_ROOT)/puk
PUK_INSTALLDIR := $(PUK_ROOT)

CONFIG_FILES := externals/configure.ac externals/configure externals/Puk/padico_config.h.in externals/Puk/Version.h

VERSION        := 0.4.0
VERSION_MAJOR  := 0
VERSION_MIDDLE := 4
VERSION_MINOR  := 0

all: build

config: $(CONFIG_FILES)

run-configure: $(PUK_BUILDDIR)/config.status

clean: clean-config clean-build

check-config-tools:
	@echo "# Checking config tools versions..."
	@( autoconf_ver="`$${AUTOCONF:-autoconf} --version | grep 'GNU Autoconf'`"; \
           if [ "x$$autoconf_ver" = "x" ]; then echo "Bad autoconf version (should be at least GNU 2.50)"; exit 1; \
           else echo "  $$autoconf_ver"; fi )
	@( autoheader_ver="`$${AUTOHEADER:-autoheader} --version | grep 'GNU Autoconf'`"; \
           if [ "x$$autoheader_ver" = "x" ]; then echo "Bad autoheader version (should be GNU)"; exit 1; \
           else echo "  $$autoheader_ver"; fi )

externals/Puk/padico_config.h.in: externals/configure.ac
	@echo "# Generating $@ ..."
	@$(MAKE) check-config-tools
	( cd externals ; $${AUTOHEADER:-autoheader} )

externals/Puk/Version.h:
	@echo "Generating version file... $@"
	@echo "#define PADICO_VERSION_MAJOR  $(VERSION_MAJOR)"   > $@
	@echo "#define PADICO_VERSION_MIDDLE $(VERSION_MIDDLE)" >> $@
	@echo "#define PADICO_VERSION_MINOR  $(VERSION_MINOR)"  >> $@
	@echo "#define PADICO_VERSION_STRING \"$(VERSION)\""    >> $@

externals/configure.ac: configure.ac
	/bin/cp configure.ac externals/

externals/configure: externals/configure.ac externals/building-tools/*.m4
	@echo "# Generating $@ ..."
	@$(MAKE) check-config-tools
	( cd externals ; $${AUTOCONF:-autoconf} )

$(PUK_BUILDDIR)/config.status: config
	mkdir -p $(PUK_BUILDDIR)
	( cd $(PUK_BUILDDIR) ; $(PUK_SRCDIR)/externals/configure --prefix=$(PUK_INSTALLDIR) )

build: run-configure
	$(MAKE) -C $(PUK_BUILDDIR)/Puk

clean-config:
	-/bin/rm $(CONFIG_FILES)

clean-build:
	-/bin/rm -r $(PUK_ROOT)/puk

