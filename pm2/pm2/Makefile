#
#                      PM2 HIGH-PERF/ISOMALLOC
#           High Performance Parallel Multithreaded Machine
#                           version 3.0
#
#     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
#       Christian Perez, Jean-Francois Mehaut, Raymond Namyst
#
#            Laboratoire de l'Informatique du Parallelisme
#                        UMR 5668 CNRS-INRIA
#                 Ecole Normale Superieure de Lyon
#
#                      External Contributors:
#                 Yves Denneulin (LMC - Grenoble),
#                 Benoit Planquelle (LIFL - Lille)
#
#                    1998 All Rights Reserved
#
#
#                             NOTICE
#
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose and without fee is hereby granted
# provided that the above copyright notice appear in all copies and
# that both the copyright notice and this permission notice appear in
# supporting documentation.
#
# Neither the institutions (Ecole Normale Superieure de Lyon,
# Laboratoire de L'informatique du Parallelisme, Universite des
# Sciences et Technologies de Lille, Laboratoire d'Informatique
# Fondamentale de Lille), nor the Authors make any representations
# about the suitability of this software for any purpose. This
# software is provided ``as is'' without express or implied warranty.
#

ifndef PM2_ROOT
export PM2_ROOT := $(CURDIR)
endif

include $(PM2_ROOT)/make/common-vars.mak

MODULES := $(shell $(PM2_ROOT)/bin/pm2-module modules)

ifneq ($(MAKECMDGOALS),init)
ifneq ($(MAKECMDGOALS),help)
ifneq ($(MAKECMDGOALS),)
LIBS = $(shell $(PM2_CONFIG) --modules)
endif
endif
endif

help:

include $(PM2_ROOT)/make/common-rules.mak

all: pm2 examples
.PHONY: pm2 examples all

pm2: flavors
	@set -e; \
	for i in $(LIBS) ; do \
		$(MAKE) -C modules/$$i ; \
	done

examples: flavors
	@set -e; \
	for i in $(LIBS) ; do \
		$(MAKE) -C modules/$$i examples ; \
	done

.PHONY: clean cleanlibs
clean: cleanlibs

cleanlibs:
	@set -e; \
	for i in $(MODULES) ; do \
		$(MAKE) -C modules/$$i clean ; \
	done

.PHONY: distclean distcleanlibs distcleanflavors distcleandoc
distclean: distcleanlibs distcleanflavors distcleandoc

distcleanlibs:
	@set -e; \
	for i in $(MODULES) ; do \
		$(MAKE) -C modules/$$i distclean ; \
	done

distcleanflavors:
	$(MAKE) -C doc distclean

distcleandoc:
	$(RM) -r stamp
	$(RM) -r flavors

.PHONY: init cvsinit flavorinit
init: cvsinit flavorinit

cvsinit ARCHITECTURES:
	@if [ -d CVS ]; then \
		$(PM2_ROOT)/bin/pm2_recreate_links ; \
	fi

flavorinit flavors: ARCHITECTURES
	@echo Creating initial sample flavors...
	@if [ -d flavors ]; then \
		echo "Aborting, some flavor already exists" ; \
		echo "Do 'make distclean' to remove all" ; \
	else \
		$(PM2_ROOT)/bin/pm2_create-sample-flavors ;\
	fi

.PHONY: doc
doc:
	$(MAKE) -C doc

flavors:
ARCHITECTURES:

.PHONY: help
help: globalhelp

bannerhelp:
	@echo "This is PM2 main Makefile"

targethelp:
	@echo "  init: initialise PM2 source tree"
	@echo "  pm2: build the PM2 libraries"
	@echo "  examples: build all the example (do not use the FLAVOR="
	@echo "    option there, unless you know what you do)"
	@echo "  config: configure flavors"
	@echo "  doc: build the documentation"
	@echo "  help: this help"
	@echo "  clean: clean source tree for current flavor"
	@echo "  distclean: clean all source tree for all flavors"

.PHONY: config
config: flavors
	@$(PM2_ROOT)/bin/pm2-config-flavor || :
