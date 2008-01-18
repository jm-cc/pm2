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

PM2_BUILD_DIR = $(shell pm2-config --flavor=$(PM2_FLAVOR) --builddir )
PUK_ROOT       := $(PM2_BUILD_DIR)/puk
PUK_SRCDIR     := $(CURDIR)
PUK_BUILDDIR   := $(PUK_ROOT)/puk
PUK_INSTALLDIR := $(PUK_ROOT)

PADICO_ROOT    := $(PUK_ROOT)

CONFIG_FILES   := externals/configure.ac externals/configure externals/Puk/padico_config.h.in VERSION


# ## front-end rules
# ##

libs:
	$(MAKE) $(PUK_BUILDDIR)/config.status
	$(MAKE) PADICO_ROOT=$(PADICO_ROOT) -C $(PUK_BUILDDIR)/Puk
	-@/bin/mkdir $(PM2_BUILD_DIR)/lib/
	@/bin/cp $(PADICO_ROOT)/lib/libPadicoPuk.a $(PM2_BUILD_DIR)/lib/

link: install

dot_h: install

init-packaging: $(PADICO_ROOT)/etc/padico-setup.sh

config: $(CONFIG_FILES)

configure: $(PUK_BUILDDIR)/config.status

install: $(PADICO_ROOT)/lib/libPadicoPuk.a

clean: clean-config clean-build

flavormaks:

# ## packaging tools
# ##

$(PADICO_ROOT)/etc/padico-setup.sh:
	@echo "# Installing Padico packaging tools in PADICO_ROOT=$(PADICO_ROOT)"
	@echo
	@./externals/packaging-tools/padico-init admindir=$(PUK_SRCDIR)/externals/packaging-tools installdir=$(PADICO_ROOT) srcdir=$(PUK_SRCDIR) builddir=$(PUK_BUILDDIR) logdir=$(PUK_BUILDDIR)/log padico_dialog=whiptail

# ## configure
# ##

check-config-tools:
	@echo "# Checking config tools versions..."
	@( autoconf_ver="`$${AUTOCONF:-autoconf} --version | grep 'GNU Autoconf'`"; \
           if [ "x$$autoconf_ver" = "x" ]; then echo "Bad autoconf version (should be at least GNU 2.50)"; exit 1; \
           else echo "  $$autoconf_ver"; fi )
	@( autoheader_ver="`$${AUTOHEADER:-autoheader} --version | grep 'GNU Autoconf'`"; \
           if [ "x$$autoheader_ver" = "x" ]; then echo "Bad autoheader version (should be GNU)"; exit 1; \
           else echo "  $$autoheader_ver"; fi )

VERSION:
	@echo "0.4.0" > VERSION

externals/Puk/padico_config.h.in: externals/configure.ac
	@echo "# Generating $@ ..."
	@$(MAKE) check-config-tools
	( cd externals ; $${AUTOHEADER:-autoheader} )

externals/configure.ac: configure.ac
	/bin/cp configure.ac externals/

externals/configure: externals/configure.ac externals/building-tools/*.m4
	@echo "# Generating $@ ..."
	@$(MAKE) check-config-tools
	( cd externals ; $${AUTOCONF:-autoconf} )

PM2_PUK_CONFIGURE_EXTRA_FLAGS=$(shell pm2-config --flavor=$(PM2_FLAVOR) --getvar=PM2_PUK_CONFIGURE_EXTRA_FLAGS )

$(PUK_BUILDDIR)/config.status: $(CONFIG_FILES) $(PADICO_ROOT)/etc/padico-setup.sh
	mkdir -p $(PUK_BUILDDIR)
	( cd $(PUK_BUILDDIR) ; $(PUK_SRCDIR)/externals/configure --disable-pukabi --prefix=$(PUK_INSTALLDIR) $(PM2_PUK_CONFIGURE_EXTRA_FLAGS) )

# ## build
# ##

build: $(PUK_BUILDDIR)/config.status
	$(MAKE) PADICO_ROOT=$(PADICO_ROOT) -C $(PUK_BUILDDIR)/Puk

# ## install
# ##

$(PADICO_ROOT)/lib/libPadicoPuk.a: $(PUK_BUILDDIR)/config.status
	$(MAKE) PADICO_ROOT=$(PADICO_ROOT) -C $(PUK_BUILDDIR)/Puk install

# ## clean
# ##

clean-config:
	-/bin/rm $(CONFIG_FILES)

clean-build:
	-/bin/rm -r $(PUK_ROOT)/puk

