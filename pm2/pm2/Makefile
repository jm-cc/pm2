


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

ifeq (,$(findstring _$(MAKECMDGOALS)_,_sos_))

# Regular Makefile

# PM2_ROOT -> racine des sources de PM2
#---------------------------------------------------------------------
ifndef PM2_ROOT
export PM2_ROOT := $(CURDIR)
endif

# Par securite, il vaut mieux considerer ROOT que PM2_ROOT, pour ne
# pas inclure de Makefiles provenant d'une distribution obsolete par
# exemple...
ROOT	:=	.

# Variables communes
#---------------------------------------------------------------------
include $(ROOT)/make/common-vars.mak

# Cache - partie principale des librairies
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/main-config.mak
endif

# Regles
#/////////////////////////////////////////////////////////////////////

# Regle par defaut : construction des librairies
# ATTENTION: l'ordre des dépendances est TRES IMPORTANT !
#---------------------------------------------------------------------
libs: ARCHITECTURES dot_h fut

# Regles communes
#---------------------------------------------------------------------
include $(ROOT)/make/common-rules.mak

# all : construction de la distribution PM2 et des exemples
#---------------------------------------------------------------------
#       Note: s'agit-il de tous les exemples ou de ceux de PM2 ?
all: pm2 examples
.PHONY: pm2 examples all libs preproc fut dot_h

# pm2: construction des librairies
#---------------------------------------------------------------------
pm2: libs

# libs: descente recursive dans chaque module pour construction des
#       librairies
#---------------------------------------------------------------------
libs: flavors
	$(COMMON_HIDE) echo ">>> Making libs"
	$(COMMON_HIDE) set -e; \
	for i in $(LIBS) ; do \
		$(MAKE) -C modules/$$i $@; \
	done

# examples: descente recursive dans chaque module pour construction
#           des exemples 
#---------------------------------------------------------------------
examples: flavors
	$(COMMON_HIDE) set -e; \
	for i in $(LIBS) ; do \
		$(MAKE) -C modules/$$i $@ ; \
	done

# preproc: descente recursive dans chaque module pour preprocessing
#---------------------------------------------------------------------
preproc: flavors
	$(COMMON_HIDE) set -e; \
	for i in $(LIBS) ; do \
		$(MAKE) -C modules/$$i $@ ; \
	done

# fut: descente recursive dans chaque module pour action `fut'
#---------------------------------------------------------------------
fut: flavors
	$(COMMON_HIDE) echo ">>> Making fut files"
	$(COMMON_HIDE) set -e; EMPTY=""; \
	for i in $(PROF_LIBS) $$EMPTY; do \
		$(MAKE) -C modules/$$i $@ ; \
	done ; \
	if [ $(APP_PROFILE) = yes ] ; then \
		$(MAKE) -C $(APP_DIR) APP_RECURSIF=true TARGET=$(APP_TARGET) fut ; \
	fi

# dot_h: descente recursive dans chaque module pour génération des `headers'
#-------------------------------------------------------------------------
dot_h: flavors
	$(COMMON_HIDE) echo ">>> Generating specific header files"
	$(COMMON_HIDE) set -e; EMPTY=""; \
	for i in $(DOT_H_GEN_LIBS) $$EMPTY; do \
		$(MAKE) -C modules/$$i $@ ; \
	done

# Nettoyage
#---------------------------------------------------------------------
.PHONY: refresh refreshall

refresh:
	$(COMMON_HIDE) set -e; \
	$(ROOT)/bin/pm2-clean --refresh $(FLAVOR)

refreshall:
	$(COMMON_HIDE) set -e; \
	$(ROOT)/bin/pm2-clean --all --refresh

.PHONY: clean cleantools cleanall cleantoolsall
clean: cleantools
	$(COMMON_HIDE) set -e; \
	$(ROOT)/bin/pm2-clean $(FLAVOR)

cleantools:
	$(COMMON_HIDE) set -e; \
	for i in ezflavor sigmund; do \
		$(MAKE) -s -C $$i clean ; \
	done

cleanall: cleantoolsall
	$(COMMON_HIDE) set -e; \
	$(ROOT)/bin/pm2-clean --all

cleantoolsall:
	$(COMMON_HIDE) set -e; \
	for i in ezflavor sigmund; do \
		$(MAKE) -s -C $$i cleanall ; \
	done

.PHONY: distclean distcleandoc

distclean: cleanall distcleandoc distcleanflavors

distcleanflavors:
	@echo "*********************"
	@echo "Remove your flavors yourself if you want."
	@echo "You can find them in $$HOME/.pm2/flavors."
	@echo "*********************"
#	$(RM) -r flavors
#	$(RM) -r stamp

distcleandoc:
	$(MAKE) -C doc distclean

# Initialisation de la distribution PM2
#---------------------------------------------------------------------
.PHONY: init checkmake cvsinit flavorinit
init: checkmake cvsinit flavorinit

checkmake:
	@if ( expr $(MAKE_VERSION) \< 3.76 >> /dev/null ) then \
	echo "Wrong make version. Upgrade to version 3.77"; exit 1;\
	fi

cvsinit ARCHITECTURES modules:
	$(COMMON_HIDE) if [ -d CVS ]; then \
		$(ROOT)/bin/pm2-recreate-links ; \
	fi

flavorinit: ARCHITECTURES # remove _flavors_ before :
	$(COMMON_HIDE) $(ROOT)/bin/pm2-create-sample-flavors

flavors:

# Documentation
#---------------------------------------------------------------------
.PHONY: doc
doc:
	$(MAKE) -C doc

# Aide
#---------------------------------------------------------------------
.PHONY: help
help: globalhelp

bannerhelp:
	@echo "This is the PM2 main Makefile"

targethelp:
	@echo "  init       : initialise PM2 source tree"
	@echo "  pm2        : build the PM2 libraries"
	@echo "  examples   : build all the examples (do not use the FLAVOR="
	@echo "               option here, unless you know what you do!)"
	@echo "  config|menuconfig|xdialogconfig|xconfig: configure flavors"
	@echo "    menuconfig   : require the dialog utility"
	@echo "    dialogconfig : require the Xdialog utility"
	@echo "    xconfig      : require X11 and GTK (libs and *.h)"
	@echo "  doc        : build the documentation"
	@echo "  help       : show this help"
	@echo "  clean      : clean binary tree for current flavor"
	@echo "  cleanall   : clean binary tree for all flavors"
	@echo "  distclean  : cleanall + destroy the flavors themselves"
	@echo "  refresh    : regenerate the current flavor"
	@echo "  refreshall : regenerate all flavors"
	@echo "  checkflavor: check correctness of all flavors"
	@echo "  sos        : make sanity checks"

# Configuration
#---------------------------------------------------------------------
.PHONY: config textconfig menuconfig xdialogconfig xconfig
config: textconfig

textconfig: flavors
	$(COMMON_HIDE) $(ROOT)/bin/pm2-config-flavor --text || :

menuconfig: flavors
	$(COMMON_HIDE) $(ROOT)/bin/pm2-config-flavor --dialog || :

xdialogconfig: flavors
	$(COMMON_HIDE) $(ROOT)/bin/pm2-config-flavor --xdialog || :

xconfig: flavors
	$(COMMON_HIDE) $(MAKE) -C ezflavor NO_REBUILD=1
	$(ROOT)/bin/ezflavor


# sos: verification des variables d'environnement PM2_*
#---------------------------------------------------------------------

checkflavor: flavors
	$(COMMON_HIDE) \
	for f in `$(ROOT)/bin/pm2-flavor list` ; do \
		echo "Checking flavor $$f" ; \
		$(ROOT)/bin/pm2-flavor check --flavor=$$f ; \
	done

else #ifeq (,$(findstring _$(MAKECMDGOALS)_,_sos_))

ifndef FLAVOR
  ifdef PM2_FLAVOR
    export FLAVOR:=$(PM2_FLAVOR)
  else # PM2_FLAVOR
    ifdef FLAVOR_DEFAULT
      export FLAVOR:=$(FLAVOR_DEFAULT)
    else # FLAVOR_DEFAULT
      export FLAVOR:=default
    endif # FLAVOR_DEFAULT
  endif # PM2_FLAVOR
endif # FLAVOR

.PHONY: sos
sos:
	@set -e ; \
	echo "********* Checking environment variables *********" ; \
	echo "PM2_HOME = $(PM2_HOME)" ; \
	echo "FLAVOR = $(FLAVOR)" ; \
	if [ ! -f `./bin/pm2-flavor-file -f $(FLAVOR)` ] ; then \
		echo "ERROR: the flavor \"$(FLAVOR)\" does not exist." ; \
		exit 1 ; \
	fi ; \
	echo "CURDIR = $(CURDIR)" ; \
	echo "PM2_ROOT = $(PM2_ROOT)" ; \
	if [ ! -d "$(PM2_ROOT)" ] ; then \
		echo "ERROR: \"$(PM2_ROOT)\" is not a valid directory." ; \
		exit 1 ; \
	fi ; \
	if [ `ls -id $(CURDIR) | awk '{print $$1;}'` -ne `ls -id $(PM2_ROOT) | awk '{print $$1;}'` ]; then \
		echo "ERROR: PM2_ROOT does not have a proper value (should be: $(CURDIR))." ; \
		exit 1 ; \
	fi ; \
	echo "PM2_BUILD_DIR = $(PM2_BUILD_DIR)" ; \
	echo "********* Refreshing files for current flavor *********" ; \
	$(MAKE) -C . refresh ; \
	echo "********* Checking correctness for current flavor *********" ; \
	./bin/pm2-flavor check --flavor=$(FLAVOR) ; \
	echo "Humm... Well, all should be ok now!"

endif #ifeq (,$(findstring _$(MAKECMDGOALS)_,_sos_))

# Fin du Makefile
######################################################################
