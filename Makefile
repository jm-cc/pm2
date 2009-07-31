
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

# Regular Makefile

# PM2_ROOT -> racine des sources de PM2
#---------------------------------------------------------------------
ifndef PM2_ROOT
export PM2_ROOT := $(CURDIR)
endif

# Par securite, il vaut mieux considerer ROOT que PM2_ROOT, pour ne
# pas inclure de Makefiles provenant d'une distribution obsolete par
# exemple...
ROOT	:=	$(CURDIR)

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
link: ARCHITECTURES dot_h fut libs

# Regles communes
#---------------------------------------------------------------------
include $(ROOT)/make/common-rules.mak

# all : construction de la distribution PM2 et des exemples
#---------------------------------------------------------------------
#       Note: s'agit-il de tous les exemples ou de ceux de PM2 ?
all: pm2 examples
.PHONY: pm2 all

# pm2: construction des librairies
#---------------------------------------------------------------------
pm2: link

# Macros pour règles récursives
#---------------------------------------------------------------------
# appelé avec $(eval $(call RECURSIVE_template, 'cible', 'subdirs...', 
#	               'dependances', "text"))
# défini les cibles :
# 'cible'
# 'cible'-'subdir' pour chaque élément 'subdir' de 'subdirs...'
# 'cible'-print
# Tous dépendent de 'dependances'
# 'cible'-print affiche "text"
# 'cible'-'subdir' invoque make avec comme cible 'cible' dans modules/'subdir'
#
# Attention, make ne "voit" pas $(MAKE) dans cette définition, on est donc
# obligé d'ajouter '+' à la main

define RECURSIVE_template
 .PHONY: $(1) $(1)-print $(addprefix $(1)-,$(2))
 $(1): $(1)-print $(addprefix $(1)-,$(2))
 $(1)-print: $(3) $(if $(4),;$$(COMMON_HIDE) echo $(4))
 $(addprefix $(1)-,$(2)): $(3) $(1)-print
	+$(COMMON_HIDE) $(MAKE) -C modules/$$(patsubst $(1)-%,%,$$@) $(1)
endef

# Comme le précédent, make 'subdirs...' est automatiquement $(LIBS)
define RECURSIVE_LIBS_template
 $(eval $(call RECURSIVE_template, $(1), $(LIBS), $(2), $(3)))
endef

# libs: descente recursive dans chaque module pour construction des
#       fichiers objets et bibliothèques
#---------------------------------------------------------------------
$(eval $(call RECURSIVE_LIBS_template, libs, fut, \
	">>> Creating object files and libs"))

# link: descente recursive dans chaque module pour édition de liens
#---------------------------------------------------------------------
$(eval $(call RECURSIVE_LIBS_template, link, libs, \
	">>> Linking dynamic libs and programs"))

# examples: descente recursive dans chaque module pour construction
#           des exemples 
#---------------------------------------------------------------------
$(eval $(call RECURSIVE_LIBS_template, examples, flavormaks, \
	">>> Building examples"))

# preproc: descente recursive dans chaque module pour preprocessing
#---------------------------------------------------------------------
$(eval $(call RECURSIVE_LIBS_template, preproc, flavormaks, \
	">>> Creating preproc files"))

# fut: descente recursive dans chaque module pour action `fut'
#---------------------------------------------------------------------
$(eval $(call RECURSIVE_template, fut, $(PROF_LIBS), dot_h, \
	">>> Making fut files"))
fut: fut-app
.PHONY: fut-app
fut-app: dot_h fut-print
#	$(COMMON_HIDE) if [ $(APP_PROFILE) = yes ] ; then \
#		$(MAKE) -C $(APP_DIR) APP_RECURSIF=true TARGET=$(APP_TARGET) fut ; \
#	fi

# dot_h: descente recursive dans chaque module pour génération des `headers'
#-------------------------------------------------------------------------
$(eval $(call RECURSIVE_template, dot_h, $(DOT_H_GEN_LIBS), flavormaks, \
	">>> Generating specific header files"))

# dot_h_all: descente recursive dans chaque module pour génération des `headers'
#-------------------------------------------------------------------------
$(eval $(call RECURSIVE_template, dot_h_all, $(DOT_H_GEN_LIBS), flavormaks, \
	">>> Generating all header files"))

# flavormaks
#---------------------------------------------------------------------
$(eval $(call RECURSIVE_LIBS_template, flavormaks, flavors, \
	">>> Generating/updating config makefiles"))

# Nettoyage
#---------------------------------------------------------------------
.PHONY: refresh refreshall

refresh:
	$(COMMON_HIDE) set -e; \
	$(ROOT)/bin/pm2-clean --refresh $(FLAVOR)

refreshall:
	$(COMMON_HIDE) set -e; \
	$(ROOT)/bin/pm2-clean --all --refresh

.PHONY: clean cleanall clean-header-autogen

FLAVOR_WITH_MARCEL :=	$(shell $(ROOT)/bin/pm2-flavor get --flavor=$(FLAVOR) | grep marcel)

clean-header-autogen:
	@if [ "$(FLAVOR_WITH_MARCEL)" != "" ] ; then $(MAKE) -C marcel clean_autogen ; fi

clean: clean-header-autogen
	$(COMMON_HIDE) set -e; \
	$(ROOT)/bin/pm2-clean $(FLAVOR)

cleanall: clean-header-autogen
	$(COMMON_HIDE) set -e; \
	$(ROOT)/bin/pm2-clean --all

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
	$(MAKE) -C doc/Getting_started distclean

# Initialisation de la distribution PM2
#---------------------------------------------------------------------
.PHONY: init initnoflavor checkmake bkco optionsinit \
	linksinit flavorinit componentsinit
init: checkmake linksinit optionsinit componentsinit flavorinit
initupdateflavor: checkmake linksinit optionsinit componentsinit flavorupdate
initnoflavor: checkmake linksinit componentsinit optionsinit

ifneq ($(FORCEOLDMAKE),1)
checkmake:
	@if ( expr $(MAKE_VERSION) \< 3.81 >> /dev/null ) then \
	echo "Wrong make version. Upgrade to version 3.81"; exit 1;\
	fi
else
checkmake:
endif

README:
	$(MAKE) bkco

optionsinit: README modules
	$(COMMON_HIDE) $(ROOT)/bin/pm2-make-option-sets

linksinit modules ARCHITECTURES: $(if $(wildcard README),,README)
	$(COMMON_HIDE) $(ROOT)/bin/pm2-recreate-links --nooptionsets

svninit: optionsinit

flavorinit: ARCHITECTURES # remove _flavors_ before :
	$(COMMON_HIDE) $(ROOT)/bin/pm2-create-sample-flavors

flavorupdate: ARCHITECTURES # remove _flavors_ before :
	$(COMMON_HIDE) $(ROOT)/bin/pm2-create-sample-flavors -f -v

flavors:

componentsinit:
	$(COMMON_HIDE) $(ROOT)/bin/nmad-driver-conf --gen-default
	$(COMMON_HIDE) $(ROOT)/bin/nmad-strategy-conf --gen-default

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
	$(COMMON_HIDE) $(MAKE) -C ezflavor FLAVOR=ezflavor
	$(ROOT)/bin/ezflavor


# sos: verification des variables d'environnement PM2_*
#---------------------------------------------------------------------

checkflavor: flavors
	$(COMMON_HIDE) \
	for f in `$(ROOT)/bin/pm2-flavor list` ; do \
		echo "Checking flavor $$f" ; \
		$(ROOT)/bin/pm2-flavor check --flavor=$$f ; \
	done

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
	echo "********* Checking source *********" ; \
	for i in modules/* ; do \
		echo $$i ; \
		if [ ! -L $$i ] ; then \
			echo "$$i is a directory" ; \
			echo "ERROR: the modules/ directory should only contain symlinks." ; \
			echo "please correctly get sources:" ; \
			echo "- when using cp, add the -a option" ; \
			echo "- when using scp, build a tar file first" ; \
			exit 1 ; \
		fi ; \
	done ; \
	echo "********* Refreshing files for current flavor *********" ; \
	$(MAKE) -C . refresh ; \
	echo "********* Checking correctness for current flavor *********" ; \
	./bin/pm2-flavor check --flavor=$(FLAVOR) ; \
	echo "Humm... Well, all should be ok now!"

.PHONY: lines
lines: distclean
	wc -l $$(find . \( -name '*.[chC]' -o -name '*.m4' \) -type f ! -path '*SCCS/*' ! -path '*BitKeeper/*' ! -path '*.svn/*' ! -path '*linux_archive/*' ) | sed -e 's/^ *//' | head -n -1 | xdu

# Fin du Makefile
######################################################################
