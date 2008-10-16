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

PM2_BUILD_DIR = $(shell pm2-config --flavor=$(FLAVOR) --builddir )
PUK_ROOT       := $(PM2_BUILD_DIR)/puk
PUK_SRCDIR     := $(CURDIR)
PUK_BUILDDIR   := $(PUK_ROOT)/puk
PUK_INSTALLDIR := $(PUK_ROOT)

PADICO_ROOT    := $(PUK_ROOT)

CONFIG_FILES   := $(PADICO_ROOT)/etc/padico-setup.sh externals/configure externals/Puk/padico_config.h.in VERSION

PUK_LIBS = $(PADICO_ROOT)/lib/libPadicoPuk.a

PUK_ENABLE_PUKABI := $(shell pm2-config --flavor=$(PM2_FLAVOR) --getvar=PM2_PUK_ENABLE_PUKABI)
ifeq ($(PUK_ENABLE_PUKABI),yes)
  PUK_LIBS += $(PADICO_ROOT)/lib/PukABI/libPukABI.so
endif


# ## front-end rules
# ##


libs: $(PADICO_ROOT)/lib/libPadicoPuk.a
	@/bin/cp $(PUK_LIBS) $(PM2_BUILD_DIR)/lib/

link:

dot_h: $(PUK_BUILDDIR)/config.status

clean:
	-/bin/rm $(CONFIG_FILES)
	-/bin/rm -r $(PUK_ROOT)/puk

flavormaks: $(CONFIG_FILES)


# ## init packaging framework and generate base config files
# ##

$(CONFIG_FILES): externals/packaging-tools/padico-setup.sh.in configure.ac externals/building-tools/*.m4
	@echo "    generating Puk version file..."
	@echo "0.4.0" > VERSION
	@echo "    installing Padico packaging tools in PADICO_ROOT=$(PADICO_ROOT)"
	@echo
	@./externals/packaging-tools/padico-init \
          admindir=$(PUK_SRCDIR)/externals/packaging-tools \
          installdir=$(PADICO_ROOT) \
          srcdir=$(PUK_SRCDIR) \
          builddir=$(PUK_BUILDDIR) \
          logdir=$(PUK_BUILDDIR)/log \
          padico_dialog=whiptail
	@echo "    checking config tools versions..."
	@( autoconf_ver="`$${AUTOCONF:-autoconf} --version | grep 'GNU Autoconf'`"; \
           if [ "x$$autoconf_ver" = "x" ]; then echo "Bad autoconf version (should be at least GNU 2.50)"; exit 1; \
           else echo "      - $$autoconf_ver"; fi )
	@( autoheader_ver="`$${AUTOHEADER:-autoheader} --version | grep 'GNU Autoconf'`"; \
           if [ "x$$autoheader_ver" = "x" ]; then echo "Bad autoheader version (should be GNU)"; exit 1; \
           else echo "      - $$autoheader_ver"; fi )
	@echo "    generating externals/configure [autoconf] ..."
	@/bin/cp configure.ac externals/
	@( cd externals ; $${AUTOCONF:-autoconf} )
	@echo "    generating externals/Puk/padico_config.h.in [autoheader] ..."
	@( cd externals ; $${AUTOHEADER:-autoheader} )
	-@/bin/mkdir $(PUK_BUILDDIR)
	-@/bin/mkdir $(PM2_BUILD_DIR)/lib/

# ## configure
# ##

PM2_PUK_CONFIGURE_EXTRA_FLAGS := $(shell pm2-config --flavor=$(PM2_FLAVOR) --getvar=PM2_PUK_CONFIGURE_EXTRA_FLAGS )

$(PUK_BUILDDIR)/config.status: $(CONFIG_FILES)
	( cd $(PUK_BUILDDIR) ; \
           $(PUK_SRCDIR)/externals/configure LDFLAGS= CFLAGS= --prefix=$(PUK_INSTALLDIR) $(PM2_PUK_CONFIGURE_EXTRA_FLAGS) )

# ## build
# ##

$(PUK_BUILDDIR)/Puk/libPadicoPuk.a: $(PUK_BUILDDIR)/config.status
	$(MAKE) PADICO_ROOT=$(PADICO_ROOT) -C $(PUK_BUILDDIR)/Puk

# ## install
# ##

$(PADICO_ROOT)/lib/libPadicoPuk.a: $(PUK_BUILDDIR)/Puk/libPadicoPuk.a
	$(MAKE) PADICO_ROOT=$(PADICO_ROOT) -C $(PUK_BUILDDIR)/Puk install


