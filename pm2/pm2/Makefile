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

# PM2_ROOT -> racine des sources de PM2
#---------------------------------------------------------------------
ifndef PM2_ROOT
export PM2_ROOT := $(CURDIR)
endif

# Variables communes
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/common-vars.mak

# Cache - partie principale des librairies
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/main-libs.mak
endif

# Regles
#/////////////////////////////////////////////////////////////////////

# Regle par defaut : construction des librairies
# ATTENTION: l'ordre des dépendances est TRES IMPORTANT !
#---------------------------------------------------------------------
libs: ARCHITECTURES dot_h fut

# Regles communes
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/common-rules.mak

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
#          Note: utilise dans quel cadre ???
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
	done

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
#         Note: quel est le status actuel des commandes de nettoyage ?
.PHONY: clean cleanlibs cleanmak
clean: cleanlibs cleanmak

cleanlibs:
	$(COMMON_HIDE) set -e; \
	for i in `$(PM2_ROOT)/bin/pm2-module modules` ; do \
		$(MAKE) -C modules/$$i clean ; \
	done

cleanmak:
	$(COMMON_CLEAN) $(RM) $(PM2_MAK_DIR)/*.mak

.PHONY: distclean distcleanlibs distcleanflavors distcleandoc distcleanall
distclean: distcleandoc distcleanlibs
distcleanall: distclean distcleanflavors

distcleanlibs:
	@echo "*********************"
	@echo "Remove your building tree yourself to clean all your compiled files."
	@echo "Without specific settings, your building tree should be in"
	@echo $$HOME/build
	@echo "*********************"

distcleanflavors:
	@echo "*********************"
	@echo "Remove your flavors yourself if you want."
	@echo "You can find them in $$HOME/.pm2/flavors."
	@echo "*********************"
#	$(RM) -r flavors
#	$(RM) -r stamp

distcleandoc:
	$(MAKE) -C doc distclean

# Initialisation de la distriubtion PM2
#---------------------------------------------------------------------
.PHONY: init checkmake cvsinit flavorinit
init: checkmake cvsinit flavorinit

checkmake:
	@if ( expr $(MAKE_VERSION) \< 3.76 >> /dev/null ) then \
	echo "Wrong make version. Upgrade to version 3.77"; exit 1;\
	fi

cvsinit ARCHITECTURES modules:
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

# Documentation
#---------------------------------------------------------------------
.PHONY: doc
doc:
	$(MAKE) -C doc

# ???
#---------------------------------------------------------------------
#      Note: quel est l'objet de ces deux^H^H^H^H^H^Htte lignes^H ?
flavors: # To remove ? This does not test any more the sample flavors


# Aide
#---------------------------------------------------------------------
.PHONY: help
help: globalhelp

bannerhelp:
	@echo "This is PM2 main Makefile"

targethelp:
	@echo "  init: initialise PM2 source tree"
	@echo "  pm2: build the PM2 libraries"
	@echo "  examples: build all the examples (do not use the FLAVOR="
	@echo "    option there, unless you know what you do)"
	@echo "  config|menuconfig|xdialogconfig|xconfig: configure flavors"
	@echo "    menuconfig: require the dialog utility"
	@echo "    dialogconfig: require the Xdialog utility"
	@echo "    xconfig: require X11 and GTK (libs and *.h)"
	@echo "  doc: build the documentation"
	@echo "  help: this help"
	@echo "  clean: clean source tree for current flavor"
	@echo "  distclean: clean all source tree for all flavors"
	@echo "  distcleanall: clean all source tree for all flavors and the"
	@echo "     flavors themselves"

# Configuration
#---------------------------------------------------------------------
.PHONY: config textconfig menuconfig xdialogconfig xconfig
config: textconfig

textconfig: flavors
	$(COMMON_HIDE) $(PM2_ROOT)/bin/pm2-config-flavor --text || :

menuconfig: flavors
	$(COMMON_HIDE) $(PM2_ROOT)/bin/pm2-config-flavor --dialog || :

xdialogconfig: flavors
	$(COMMON_HIDE) $(PM2_ROOT)/bin/pm2-config-flavor --xdialog || :

xconfig: flavors
	$(COMMON_HIDE) $(MAKE) -C ezflavor NO_REBUILD=1
	$(PM2_ROOT)/bin/ezflavor

# Generation du cache principal des librairies
#---------------------------------------------------------------------
#         Note: pourquoi pas dans libs-rules.mak ?
$(PM2_MAK_DIR)/main-libs.mak: $(MAIN_STAMP_FLAVOR)
	@if [ ! -z "$(PM2_MAK_DIR)" ]; then \
		$(PM2_GEN_MAK) all ;\
	else \
		echo "Cannot generate $@" ;\
		exit 1 ;\
	fi


# Fin du Makefile
######################################################################
