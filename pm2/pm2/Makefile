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

ifneq ($(MAKECMDGOALS),init)
ifneq ($(MAKECMDGOALS),help)
ifneq ($(MAKECMDGOALS),textconfig)
ifneq ($(MAKECMDGOALS),menuconfig)
ifneq ($(MAKECMDGOALS),xconfig)
-include $(PM2_MAK_DIR)/main-libs.mak
endif
endif
endif
endif
endif

libs:

include $(PM2_ROOT)/make/common-rules.mak

all: pm2 examples
.PHONY: pm2 examples all libs preproc fut

pm2: libs


libs: flavors ARCHITECTURES
	$(COMMON_HIDE) set -e; \
	for i in $(LIBS) ; do \
		$(MAKE) -C modules/$$i ; \
	done

examples: flavors
	$(COMMON_HIDE) set -e; \
	for i in $(LIBS) ; do \
		$(MAKE) -C modules/$$i examples ; \
	done

preproc: flavors
	$(COMMON_HIDE) set -e; \
	for i in $(LIBS) ; do \
		$(MAKE) -C modules/$$i preproc ; \
	done

fut: flavors
	$(COMMON_HIDE) set -e; \
	for i in $(LIBS) ; do \
		$(MAKE) -C modules/$$i fut ; \
	done

.PHONY: clean cleanlibs
clean: cleanlibs

cleanlibs:
	$(COMMON_HIDE) set -e; \
	for i in $(MODULES) ; do \
		$(MAKE) -C modules/$$i clean ; \
	done

.PHONY: distclean distcleanlibs distcleanflavors distcleandoc distcleanall
distclean: distcleanlibs distcleandoc
distcleanall: distclean distcleanflavors

distcleanlibs:
	$(COMMON_HIDE) set -e; \
	for i in $(MODULES) ; do \
		$(MAKE) -C modules/$$i distclean ; \
	done
	if [ -d stamp ]; then \
		find stamp -iname "libstamp*" -exec rm {} ;\
	fi

distcleanflavors:
	$(RM) -r flavors
	$(RM) -r stamp

distcleandoc:
	$(MAKE) -C doc distclean

.PHONY: init cvsinit flavorinit
init: checkmake cvsinit flavorinit


checkmake:
	@if ( expr $(MAKE_VERSION) \< 3.77 >> /dev/null ) then \
	echo "Wrong make version. Upgrade to version 3.77"; exit 1;\
	fi


cvsinit ARCHITECTURES:
	$(COMMON_HIDE) if [ -d CVS ]; then \
		$(PM2_ROOT)/bin/pm2_recreate_links ; \
	fi

flavorinit: ARCHITECTURES # remove _flavors_ before :
	$(COMMON_HIDE) echo Creating initial sample flavors...
	$(COMMON_HIDE) if [ -d flavors ]; then \
		echo "Aborting, some flavor already exists" ; \
		echo "Do 'make distcleanall' to remove all" ; \
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
	@echo "  menuconfig: configure flavors (need the dialog utility)"
	@echo "  xconfig: configure flavors (need the Xdialog utility)"
	@echo "  doc: build the documentation"
	@echo "  help: this help"
	@echo "  clean: clean source tree for current flavor"
	@echo "  distclean: clean all source tree for all flavors"
	@echo "  distcleanall: clean all source tree for all flavors and the"
	@echo "     flavors themselves"

.PHONY: config textconfig menuconfig xconfig
config: textconfig

textconfig: flavors
	$(COMMON_HIDE) $(PM2_ROOT)/bin/pm2-config-flavor --text || :

menuconfig: flavors
	$(COMMON_HIDE) $(PM2_ROOT)/bin/pm2-config-flavor --dialog || :

xconfig: flavors
	$(COMMON_HIDE) $(PM2_ROOT)/bin/pm2-config-flavor --xdialog || :


$(PM2_MAK_DIR)/main-libs.mak: $(MAIN_STAMP_FLAVOR)
	@echo "Generating $@"
	@$(PM2_CONFIG) --gen_mak main-libs


