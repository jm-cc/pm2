

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
libs: $(LIBRARY)

# if someone does 'make' in an module directory, ...
#---------------------------------------------------------------------
no_goal:
	$(MAKE) -C $(PM2_ROOT)

# Regles de preprocessing
#---------------------------------------------------------------------
.PHONY: preproc fut
preproc: $(LIB_PREPROC)

fut: $(LIB_FUT)

# Regles de génération des .h
#---------------------------------------------------------------------
.PHONY: dot_h

# La librairie
#---------------------------------------------------------------------
#    Note: utilite de cette regle ?
# Pouvoir faire 'make marcel' dans le répertoire marcel...
$(LIBRARY):

# Regles communes
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/common-rules.mak

# Cas ou la librairie n'a pas le meme nom que le module
#---------------------------------------------------------------------
ifneq ($(LIBRARY),$(LIBNAME))
$(LIBNAME): $(LIBRARY)
endif

# Tout: construire la librairie
#---------------------------------------------------------------------
all: libs

.PHONY: $(LIBRARY) $(LIBNAME) libs

# Controle de l'affichage des messages
#---------------------------------------------------------------------
ifdef SHOW_FLAVOR
$(LIBRARY): showflavor
endif

ifdef SHOW_FLAGS
$(LIBRARY): showflags
endif

# Contribution aux dependances communes
#---------------------------------------------------------------------
#    Note: pourquoi ici precisemment plutot que dans libs-vars.mak ?
COMMON_DEPS += $(PM2_MAK_DIR)/$(LIBRARY)-config.mak

# Le module impose la construction des librairies (.a et/ou .so)
#---------------------------------------------------------------------
$(LIBRARY): $(STAMP_BUILD_LIB)

# Affichage des flags 
#---------------------------------------------------------------------
.PHONY: showflags
showflags:
	@echo "  Compiling $(LIBRARY) using CFLAGS=$(CFLAGS)"

# Affichage de la flavor 
#---------------------------------------------------------------------
showflavor:
	@echo "  Compiling $(LIBRARY) for flavor: $(FLAVOR)"

# - Construction des repertoires destination
# - inclusion des dependances
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))

# Target subdirectories
DUMMY_BUILD :=  $(foreach REP, $(LIB_REP_TO_BUILD), $(shell mkdir -p $(REP)))
ifneq ($(wildcard $(LIB_DEPENDS)),)
include $(wildcard $(LIB_DEPENDS))
endif # LIB_DEPENDS

endif # $(MAKECMDGOALS)

# Dependances communes
#---------------------------------------------------------------------
$(LIB_DEPENDS): $(COMMON_DEPS)

# Dependances vers *.d
#---------------------------------------------------------------------
$(LIB_OBJECTS): $(LIB_GEN_OBJ)/%.o: $(LIB_GEN_DEP)/%.d $(COMMON_DEPS)
$(LIB_C_PREPROC): $(LIB_GEN_CPP)/%.i: $(LIB_GEN_DEP)/%.d $(COMMON_DEPS)
$(LIB_S_PREPROC): $(LIB_GEN_CPP)/%.si: $(LIB_GEN_DEP)/%.d $(COMMON_DEPS)
$(LIB_PICS): $(LIB_GEN_OBJ)/%.pic: $(LIB_GEN_DEP)/%.d $(COMMON_DEPS)

# Archivage librairie(s)
#---------------------------------------------------------------------
buildstatic: $(STAMP_BUILD_LIB_A)
builddynamic: $(STAMP_BUILD_LIB_SO)

.PHONY: buildstatic builddynamic

ifeq ($(LIB_GEN_OBJ),)
$(STAMP_BUILD_LIB):
	@echo "The flavor $(FLAVOR) do not need this library"
else
$(STAMP_BUILD_LIB): $(LIB_LIB)
endif

$(STAMP_BUILD_LIB_A): $(LIB_LIB_A)
	$(COMMON_HIDE)touch $(STAMP_BUILD_LIB_A)
	$(COMMON_HIDE)touch $(STAMP_BUILD_LIB)

$(LIB_LIB_A): $(LIB_OBJECTS)
	$(COMMON_BUILD)
	$(COMMON_HIDE) rm -f $@
	$(COMMON_MAIN) ar cr $@ $(filter %.o, $^)

$(STAMP_BUILD_LIB_SO): $(LIB_LIB_SO) $(LIB_LIB_SO_MAJ) $(LIB_LIB_SO_MAJ_MIN)

VERSION_SCRIPT_OPT=-Xlinker --version-script=
LINK_CMD=$(LD) -Xlinker --soname=$(notdir $(LIB_LIB_SO_MAJ)) \
		$(addprefix $(VERSION_SCRIPT_OPT), $(strip $(LIB_SO_MAP))) \
		-shared -o $(LIB_LIB_SO_MAJ_MIN) $(LDFLAGS) \
		$(filter %.pic, $(LIB_PICS))
$(LIB_LIB_SO_MAJ_MIN): $(LIB_PICS) $(LIB_LIB_SO_MAP)
	$(COMMON_BUILD)
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

link_libs: $(LINK_LIB)
linkdynamic: $(STAMP_LINK_LIB)

ifeq ($(LINK_DYNAMIC_LIBS),pm2)

endif

$(STAMP_LINK_LIB): $(foreach name,$(LIB_PM2_SHLIBS), \
			$(LIB_GEN_STAMP)/stamp-build-$(name).so)
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(LINK_CMD) $(addprefix -Xlinker -L,$(LIB_GEN_LIB)) \
		$(addprefix -Xlinker -l,$(filter-out $(LIBNAME), $(LIB_PM2_SHLIBS)))
	$(COMMON_HIDE)touch $(STAMP_LINK_LIB)

# Dependances vers *.c
#---------------------------------------------------------------------
$(LIB_C_OBJECTS): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: %.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CFLAGS) -c $< -o $@

$(LIB_C_PICS): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: %.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CFLAGS) -fPIC -c $< -o $@

$(LIB_C_DEPENDS): $(LIB_GEN_DEP)/%$(LIB_EXT).d: %.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(SHELL) -ec '$(CC) -MM $(CFLAGS) -DDEPEND $< \
		| sed '\''s|.*:|$(LIB_DEP_TO_OBJ) $@ :|g'\'' > $@'

$(LIB_C_PREPROC): $(LIB_GEN_CPP)/%$(LIB_EXT).i: %.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) -E -P -DPREPROC $(CFLAGS) $< > $@

# Dependances vers *.S
#---------------------------------------------------------------------
$(LIB_S_OBJECTS): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: %.S
	$(COMMON_BUILD)
	$(COMMON_HIDE) $(CC) -E -P $(CFLAGS) $< > $(LIB_OBJ_TO_S)
	$(COMMON_MAIN) $(AS) $(CFLAGS) -c $(LIB_OBJ_TO_S) -o $@

$(LIB_S_PICS): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: %.S
	$(COMMON_BUILD)
	$(COMMON_HIDE) $(CC) -E -P $(CFLAGS) $< > $(LIB_PIC_TO_S)
	$(COMMON_MAIN) $(AS) $(CFLAGS) -fPIC -c $(LIB_PIC_TO_S) -o $@

$(LIB_S_DEPENDS): $(LIB_GEN_DEP)/%$(LIB_EXT).d: %.S
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(SHELL) -ec '$(CC) -MM $(CFLAGS) -DDEPEND $< \
		| sed '\''s|.*:|$(LIB_DEP_TO_OBJ) $@ :|g'\'' > $@'

$(LIB_S_PREPROC): $(LIB_GEN_CPP)/%$(LIB_EXT).si: %.S
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) -E -P -DPREPROC $(CFLAGS) $< > $@


# Dependances vers *.i
#---------------------------------------------------------------------
$(LIB_FUT): $(LIB_GEN_CPP)/%.fut: $(LIB_GEN_CPP)/%.i
	$(COMMON_BUILD)
	$(COMMON_HIDE) cp /dev/null $@
	$(COMMON_MAIN) gcc -c -O0 $< -o /tmp/foo-$(USER).o
	$(COMMON_MAIN) nm /tmp/foo-$(USER).o | fgrep this_is_the_ | sed -e 's/^.*this_is_the_//' >> $@
	$(COMMON_HIDE) rm -f /tmp/foo-$(USER).o
	$(COMMON_HIDE) touch $(LIB_GEN_STAMP)/fut_stamp

# Regles de nettoyage
#---------------------------------------------------------------------
.PHONY: clean cleanall refresh refreshall sos
clean cleanall refresh refreshall sos:
	$(COMMON_HIDE) make -s -C $(PM2_ROOT) $@

# Exemples
#---------------------------------------------------------------------
.PHONY: examples
examples:
	$(COMMON_HIDE)set -e; \
	if [ -d examples ]; then \
		$(MAKE) -C examples ; \
	fi

# Aide
#---------------------------------------------------------------------
.PHONY: help bannerhelplibs targethelplibs
help: globalhelp

bannerhelp: bannerhelplibs

bannerhelplibs:
	@echo "This is PM2 Makefile for module $(LIBRARY)"

targethelp: targethelplibs

targethelplibs:
ifneq ($(LIBRARY),$(LIBNAME))
	@echo "  libs|all|$(LIBRARY)|$(LIBNAME): build the module libraries"
else
	@echo "  libs|all|$(LIBRARY): build the module libraries"
endif
	@echo "  examples: build the example of this module (if any)"
	@echo "  help: this help"
	@echo "  clean: clean module source tree for current flavor"
#	@echo "  distclean: clean module source tree for all flavors"

# Regle de construction du cache de configuration de la librairie
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
$(PM2_MAK_DIR)/$(LIBRARY)-config.mak: $(LIB_STAMP_FLAVOR)
	$(COMMON_HIDE) $(PM2_GEN_MAK) $(LIBRARY)
endif

######################################################################
