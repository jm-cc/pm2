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

# Regle par defaut: construction de la librairie
#---------------------------------------------------------------------
libs:

# La librairie
#---------------------------------------------------------------------
#    Note: utilite de cette regle ?
# Pouvoir faire 'make marcel' dans le r�pertoire marcel...
$(LIBRARY):

# Regles communes
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/objs-rules.mak

# Regle par defaut: construction de la librairie
# (apr�s objs-rules.mak pour que la flavor s'affiche avant la compilation)
#---------------------------------------------------------------------
libs: $(LIBRARY)

# Cas ou la librairie n'a pas le meme nom que le module
#---------------------------------------------------------------------
ifneq ($(LIBRARY),$(LIBNAME))
$(LIBNAME): $(LIBRARY)
endif

# Tout: construire la librairie
#---------------------------------------------------------------------
all: libs

.PHONY: $(LIBRARY) $(LIBNAME) libs

# Le module impose la construction des librairies (.a et/ou .so)
#---------------------------------------------------------------------
$(LIBRARY): $(STAMP_BUILD_LIB)

# Dependances communes
#---------------------------------------------------------------------
$(MOD_DEPENDS): $(COMMON_DEPS)

# Archivage librairie(s)
#---------------------------------------------------------------------
buildstatic: $(STAMP_BUILD_LIB_A)
builddynamic: $(STAMP_BUILD_LIB_SO)

.PHONY: buildstatic builddynamic

ifeq ($(MOD_GEN_OBJ),)
$(STAMP_BUILD_LIB):
	@echo "The flavor $(FLAVOR) do not need this library"
else
$(STAMP_BUILD_LIB): $(LIB_LIB)
endif

$(STAMP_BUILD_LIB_A): $(LIB_LIB_A)
	$(COMMON_HIDE)touch $(STAMP_BUILD_LIB_A)
	$(COMMON_HIDE)touch $(STAMP_BUILD_LIB)

$(LIB_LIB_A): $(MOD_OBJECTS)
	$(COMMON_BUILD)
	$(COMMON_HIDE) rm -f $@
	$(COMMON_MAIN) ar crs $@ $(filter-out %/_$(LIBRARY)_link.o, $(filter %.o, $^))

$(STAMP_BUILD_LIB_SO): $(LIB_LIB_SO) $(LIB_LIB_SO_MAJ) $(LIB_LIB_SO_MAJ_MIN)

VERSION_SCRIPT_OPT=-Xlinker --version-script=
LINK_CMD=$(LD) -Xlinker --soname=$(notdir $(LIB_LIB_SO_MAJ)) \
		$(addprefix $(VERSION_SCRIPT_OPT), $(strip $(LIB_SO_MAP))) \
		-shared -o $(LIB_LIB_SO_MAJ_MIN) $(MOD_EARLY_LDFLAGS) $(LINK_LDFLAGS) \
		$(filter-out %/_$(LIBRARY)_link.pic, $(filter %.pic, $(MOD_PICS)))
$(LIB_LIB_SO_MAJ_MIN): $(MOD_PICS) $(LIB_LIB_SO_MAP)
	$(COMMON_LINK)
	$(COMMON_HIDE) rm -f $@
	$(COMMON_MAIN) $(LINK_CMD)
	$(COMMON_HIDE) touch $(STAMP_BUILD_LIB_SO)
	$(COMMON_HIDE) touch $(STAMP_BUILD_LIB)

ifneq ($(LIB_LIB_SO_MAJ_MIN),$(LIB_LIB_SO_MAJ))
$(LIB_LIB_SO_MAJ): 
#$(LIB_LIB_SO_MAJ_MIN)
	$(COMMON_BUILD)
	$(COMMON_MAIN) ln -sf $(notdir $(LIB_LIB_SO_MAJ_MIN)) $@
endif
ifneq ($(LIB_LIB_SO_MAJ),$(LIB_LIB_SO))
$(LIB_LIB_SO): $(LIB_LIB_SO_MAJ)
	$(COMMON_BUILD)
	$(COMMON_MAIN) ln -sf $(notdir $(LIB_LIB_SO_MAJ)) $@
endif

link: $(LINK_LIB)
linkdynamic: $(STAMP_LINK_LIB)

ifeq ($(LINK_DYNAMIC_LIBS),pm2)

endif

$(STAMP_LINK_LIB): $(foreach name,$(MOD_PM2_SHLIBS), \
			$(MOD_GEN_STAMP)/stamp-build-$(name).so)
	$(COMMON_LINK)
	$(COMMON_MAIN) $(LINK_CMD) $(addprefix -Xlinker -L,$(MOD_GEN_LIB)) \
		$(addprefix -Xlinker -l,$(filter-out $(LIBNAME), $(MOD_PM2_SHLIBS)))
	$(COMMON_HIDE)touch $(STAMP_LINK_LIB)


######################################################################
