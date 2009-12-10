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
# Pouvoir faire 'make marcel' dans le répertoire marcel...
$(LIBRARY):

# Regles communes
#---------------------------------------------------------------------
include $(PM2_SRCROOT)/make/objs-rules.mak

# Regle par defaut: construction de la librairie
# (après objs-rules.mak pour que la flavor s'affiche avant la compilation)
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
$(LIBRARY): $(LIB_LIB)

# Dependances communes
#---------------------------------------------------------------------
$(MOD_DEPENDS): $(COMMON_DEPS)

# Archivage librairie(s)
#---------------------------------------------------------------------
.PHONY: buildstatic builddynamic

buildstatic: $(LIB_LIB_A)
$(LIB_LIB_A): $(MOD_OBJECTS)
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(AR) crs $@ $(filter-out %/_$(LIBRARY)_link.o, $(filter %.o, $^))
	$(COMMON_MAIN) $(foreach ADD_LIBNAME, $(ADDITIONAL_LIBNAME),  ln -sf $(notdir $@) $(dir $@)/lib$(ADD_LIBNAME).a ;)

ifeq ($(HAVE_GNU_LD),yes)
VERSION_SCRIPT_OPT = $(addprefix -Xlinker --version-script=,$(1))
else
VERSION_SCRIPT_OPT =
endif

SONAME=$(notdir $(LIB_LIB_SO_MAJ))
# AIX doesn't support sonames
ifeq ($(MOD_SYS),AIX_SYS)
  SONAMEFLAGS=
else
  ifeq ($(MOD_SYS),OSF_SYS)
    SONAMEFLAGS=-Xlinker -soname -Xlinker $(SONAME)
  else
    ifeq ($(HAVE_GNU_LD),yes)
      SONAMEFLAGS = -Xlinker --soname=$(SONAME)
    else
      SONAMEFLAGS =
    endif
  endif
endif

LINK_CMD:=$(LD) $(SONAMEFLAGS)\
		$(call VERSION_SCRIPT_OPT,$(strip $(LIB_SO_MAP))) \
		-shared -o $(LIB_LIB_SO_MAJ_MIN) $(MOD_EARLY_LDFLAGS) $(LINK_EARLY_LDFLAGS) $(LINK_LDFLAGS) \
		$(filter-out %/_$(LIBRARY)_link.pic, $(filter %.pic, $(MOD_PICS)))

ifeq ($(LINK_OTHER_LIBS),yes)
# Depend on other libs
$(LIB_LIB_SO_MAJ_MIN): $(foreach name,$(filter-out $(LIBNAME), $(MOD_PM2_SHLIBS)), $(MOD_GEN_LIB)/lib$(name)$(LIB_EXT).so)
# and link against them
LINK_CMD+= $(shell $(PM2_CONFIG) --libs-only-L) \
	   $(addprefix -Xlinker -L,$(MOD_GEN_LIB)) \
	   $(addprefix -Xlinker -l,$(addsuffix $(LIB_EXT),$(filter-out $(LIBNAME), $(MOD_PM2_SHLIBS)))) \
	   $(addprefix -Xlinker -l,$(addsuffix $(LIB_EXT),$(filter-out $(LIBNAME), $(MOD_PM2_STLIBS))))

ifeq ($(HAVE_GNU_LD),yes)
LINK_CMD += -Wl,--rpath=$(MOD_GEN_LIB)
endif # HAVE_GNU_LD

endif # LINK_OTHER_LIBS

ifeq ($(LINK_OTHER_LIBS),yes)
# We link at the link stage, not build stage. But compile pics ASAN.
builddynamic: $(MOD_PICS)
linkdynamic: $(LIB_LIB_SO) $(LIB_LIB_SO_MAJ) $(LIB_LIB_SO_MAJ_MIN)
else
builddynamic: $(LIB_LIB_SO) $(LIB_LIB_SO_MAJ) $(LIB_LIB_SO_MAJ_MIN)
endif
link: $(LINK_LIB)

$(LIB_LIB_SO_MAJ_MIN): $(MOD_PICS) $(LIB_SO_MAP)
	$(COMMON_LINK)
	$(COMMON_MAIN) $(LINK_CMD)

ifneq ($(LIB_LIB_SO_MAJ_MIN),$(LIB_LIB_SO_MAJ))
$(LIB_LIB_SO_MAJ): 
#$(LIB_LIB_SO_MAJ_MIN)
	$(COMMON_BUILD)
	$(COMMON_MAIN) ln -sf $(notdir $(LIB_LIB_SO_MAJ_MIN)) $@
	$(COMMON_MAIN) $(foreach ADD_LIBNAME, $(ADDITIONAL_LIBNAME),  ln -sf $(notdir $@) $(dir $@)/lib$(ADD_LIBNAME).so ;)
endif
ifneq ($(LIB_LIB_SO_MAJ),$(LIB_LIB_SO))
$(LIB_LIB_SO): $(LIB_LIB_SO_MAJ)
	$(COMMON_BUILD)
	$(COMMON_MAIN) ln -sf $(notdir $(LIB_LIB_SO_MAJ)) $@
	$(COMMON_MAIN) $(foreach ADD_LIBNAME, $(ADDITIONAL_LIBNAME), ln -sf $(notdir $@) $(dir $@)/lib$(ADD_LIBNAME).so ;)
endif

# Test suite
#-----------

# Target provided for convenience, similar to that found in
# Automake-generated makefiles.
check: dot_h
	module_name="$(shell basename $(realpath .))" ;		\
	$(PM2_OBJROOT)/bin/pm2-test --torture $$module_name

.PHONY: check


# Generation de la doc et copie sur le serveur web
#-------------------------------------------------
ifeq (,$(LIBDOC))
LIBDOC := $(LIBNAME)
endif

docs:
	doxygen

docs-upload:
	rsync -acvrlt --exclude .svn --delete doc gforge:/home/groups/pm2/htdocs/$(LIBDOC)

######################################################################
