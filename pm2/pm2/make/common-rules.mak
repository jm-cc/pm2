# -*- mode: makefile;-*-

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

# Regle par defaut : aide
#---------------------------------------------------------------------
help:

# En cas d'erreur, on efface la cible
#---------------------------------------------------------------------
.DELETE_ON_ERROR:

# Declarations Phony
#---------------------------------------------------------------------
.PHONY: all
.PHONY: help globalhelp bannerhelp targethelp commonoptionshelp
.PHONY: bannertargethelp optionshelp thankshelp tailhelp
.PHONY: tags

# Aide
#--------------------------------------------------------------------
globalhelp: bannerhelp bannertargethelp targethelp commonoptionshelp optionshelp thankshelp tailhelp

# Note: interet ?
bannerhelp:

# Note: interet ?
targethelp:

bannertargethelp:
	@echo ""
	@echo "Targets:"

commonoptionshelp:
	@echo
	@echo "Common options:"
	@echo "  FLAVOR=flavor: build for flavor 'flavor'"
	@echo "  SHOW_FLAGS=true: show CFLAGS used"
	@echo "  SHOW_FLAVOR=true: show flavor target"
	@echo "  VERB=verbose|normal|quiet|silent: vebosity of building process"
	@echo
	@echo "Most of these options can be given with command line or"
	@echo "configurated in $(PM2_ROOT)/make/config.mak"

# Note: interet ?
suboptionshelp:

thankshelp:
	@echo
	@echo "Troubles ? Contact Luc.Bouge@ens-lyon.fr"
	@echo "Have a good day with PM2"

# Note: interet ?
tailhelp:

# Construction du repertoire destination des makefiles generes
#---------------------------------------------------------------------
$(PM2_MAK_DIR):
	$(COMMON_HIDE)mkdir -p $@

# Generation des caches des makefiles
#---------------------------------------------------------------------
$(PM2_MAK_DIR)/%-config.mak: $(MAIN_STAMP_FLAVOR)
	$(COMMON_MAKE)
	$(COMMON_HIDE) mkdir -p $(PM2_MAK_DIR)
	$(COMMON_MAIN) $(PM2_GEN_MAK) --stdout --flavor $(FLAVOR) \
		$(PM2_GEN_MAK_OPTIONS) $* > $@

# Construction d'un fichier de tags pour les éditeurs de fichiers
#---------------------------------------------------------------------
CTAGS := ctags

tags:
	$(CTAGS) --recurse=yes --exclude=SCCS --exclude=BitKeeper --exclude=ChangeSet --exclude=autogen-include .
