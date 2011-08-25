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

# if someone does 'make' in an module directory, ...
#---------------------------------------------------------------------
no_goal:
	$(MAKE) -C $(PM2_OBJROOT)

.DELETE_ON_ERROR:

# Affichage en fonction des options 
#---------------------------------------------------------------------
ifdef SHOW_FLAVOR
libs: showflavor
link: showflavor
endif

ifdef SHOW_FLAGS
libs: showflags-obj
link: showflags-ld
endif

# Regles de preprocessing
#---------------------------------------------------------------------
.PHONY: preproc fut
preproc: $(MOD_PREPROC)

fut: $(MOD_FUT)
#	echo $(MOD_FUT)

# Regles de génération des .h
#---------------------------------------------------------------------
.PHONY: dot_h dot_h_all flavormaks

# Regles communes
#---------------------------------------------------------------------
include $(PM2_SRCROOT)/make/common-rules.mak

# Dépendances du fichier de cache sur la flavor
#---------------------------------------------------------------------
$(PM2_MAK_DIR)/$(MODULE)-config.mak: $(MOD_STAMP_FLAVOR)

# Affichage des flags 
#---------------------------------------------------------------------
.PHONY: showflags showflags-obj showflags-ld
showflags: showflags-obj showflags-ld

showflags-obj: CFLAGS+=$(MOD_CFLAGS)
showflags-obj:
	@echo "  Compiling $(MODULE) using CFLAGS=$(CFLAGS)"

showflags-ld:
	@echo "  Linking $(MODULE) using LDFLAGS=$(LDFLAGS)"

# Affichage de la flavor 
#---------------------------------------------------------------------
showflavor:
	@echo "  Compiling $(MODULE) for flavor: $(FLAVOR)"

# - Construction des repertoires destination
# - inclusion des dependances
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))

# Target subdirectories
DUMMY_BUILD :=  $(foreach REP, $(MOD_REP_TO_BUILD), $(shell mkdir -p $(REP)))
ifneq ($(wildcard $(MOD_DEPENDS)),)
include $(wildcard $(MOD_DEPENDS))
endif # MOD_DEPENDS
endif # $(MAKECMDGOALS)

# Dummy target to update generated makefiles early enough
#--------------------------------------------------------------------- 
flavormaks:

# Dependances globales *.d
#---------------------------------------------------------------------
$(MOD_OBJECTS): $(MOD_GEN_OBJ)/%.o: $(COMMON_DEPS)
$(MOD_PICS): $(MOD_GEN_OBJ)/%.pic: $(COMMON_DEPS)
$(MOD_C_PREPROC): $(MOD_GEN_CPP)/%.i: $(COMMON_DEPS)
$(MOD_CXX_PREPROC): $(MOD_GEN_CPP)/%.cppi: $(COMMON_DEPS)
$(MOD_S_PREPROC): $(MOD_GEN_CPP)/%.si: $(COMMON_DEPS)
ifeq ($(DEP_ON_FLY),false)
$(MOD_DEPENDS): $(MOD_GEN_DEP)/%.d: $(COMMON_DEPS)
$(MOD_OBJECTS) $(MOD_PICS): $(MOD_GEN_OBJ)/%: $(MOD_GEN_DEP)/%.d
$(MOD_PREPROC): $(MOD_GEN_CPP)/%: $(MOD_GEN_DEP)/%.d

$(filter %.o.d, $(MOD_C_DEPENDS)): CFLAGS+=$(MOD_CFLAGS)
$(filter %.o.d, $(MOD_C_DEPENDS)): $(MOD_GEN_DEP)/%$(LIB_EXT).o.d: %.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CC_DEPFLAGS) $(CFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d \1:|' > $@
$(filter %.pic.d, $(MOD_C_DEPENDS)): CFLAGS+=$(MOD_CFLAGS)
$(filter %.pic.d, $(MOD_C_DEPENDS)): $(MOD_GEN_DEP)/%$(LIB_EXT).pic.d: %.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CC_DEPFLAGS) $(CFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d \1:|' > $@
$(filter %.o.d, $(MOD_CXX_DEPENDS)): CXXFLAGS+=$(MOD_CXXFLAGS)
$(filter %.o.d, $(MOD_CXX_DEPENDS)): $(MOD_GEN_DEP)/%$(LIB_EXT).o.d: %.cpp
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CXX) $(CC_DEPFLAGS) $(CXXFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d \1:|' > $@
$(filter %.pic.d, $(MOD_CXX_DEPENDS)): CXXFLAGS+=$(MOD_CXXFLAGS)
$(filter %.pic.d, $(MOD_CXX_DEPENDS)): $(MOD_GEN_DEP)/%$(LIB_EXT).pic.d: %.cpp
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CXX) $(CC_DEPFLAGS) $(CXXFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d \1:|' > $@
$(filter %.o.d, $(MOD_S_DEPENDS)): CFLAGS+=$(MOD_CFLAGS)
$(filter %.o.d, $(MOD_S_DEPENDS)): $(MOD_GEN_DEP)/%$(LIB_EXT).o.d: %.S
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CC_DEPFLAGS) $(CFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d \1:|' > $@
$(filter %.pic.d, $(MOD_S_DEPENDS)): CFLAGS+=$(MOD_CFLAGS)
$(filter %.pic.d, $(MOD_S_DEPENDS)): $(MOD_GEN_DEP)/%$(LIB_EXT).pic.d: %.S
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CC_DEPFLAGS) $(CFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d \1:|' > $@
$(filter %.o.d, $(MOD_S_DEPENDS)): FFLAGS+=$(MOD_FFLAGS)
$(filter %.o.d, $(MOD_S_DEPENDS)): $(MOD_GEN_DEP)/%$(LIB_EXT).o.d: %.f90
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CC_DEPFLAGS) $(FFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d \1:|' > $@
$(filter %.pic.d, $(MOD_S_DEPENDS)): FFLAGS+=$(MOD_FFLAGS)
$(filter %.pic.d, $(MOD_S_DEPENDS)): $(MOD_GEN_DEP)/%$(LIB_EXT).pic.d: %.f90
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CC_DEPFLAGS) $(FFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d \1:|' > $@
$(MOD_I_DEPENDS): CFLAGS+=$(MOD_CFLAGS)
$(MOD_I_DEPENDS): $(MOD_GEN_DEP)/%$(LIB_EXT).d: %.i
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CC_DEPFLAGS)  -DPREPROC $(CFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d $$(MOD_GEN_CPP)/\1:|' > $@
$(MOD_CI_DEPENDS): CXXFLAGS+=$(MOD_CXXFLAGS)
$(MOD_CI_DEPENDS): $(MOD_GEN_DEP)/%$(LIB_EXT).d: %.cppi
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CXX) $(CC_DEPFLAGS)  -DPREPROC $(CXXFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d $$(MOD_GEN_CPP)/\1:|' > $@
$(MOD_SI_DEPENDS): CFLAGS+=$(MOD_CFLAGS) $(MOD_AFLAGS)
$(MOD_SI_DEPENDS): $(MOD_GEN_DEP)/%$(LIB_EXT).d: %.si
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CC_DEPFLAGS)  -DPREPROC $(CFLAGS) $< | \
		sed -e 's|\(.*\):|$$(MOD_GEN_DEP)/\1.d $$(MOD_GEN_CPP)/\1:|' > $@
endif

# Dependances vers *.c
#---------------------------------------------------------------------
$(MOD_C_OBJECTS): CFLAGS+=$(MOD_CFLAGS) -DSTATIC_BUILD
$(MOD_C_OBJECTS): CFLAGS+=$($<-cflags)
$(MOD_C_OBJECTS): CPPFLAGS+=$($<-cppflags)
$(MOD_C_OBJECTS): $(MOD_GEN_OBJ)/%.o: $(MOD_GEN_C_INC)
$(MOD_C_OBJECTS): $(MOD_GEN_OBJ)/%.o: %.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(MOD_C_PREPROCESSED): CFLAGS+=$(MOD_CFLAGS)
$(MOD_C_PREPROCESSED): $(MOD_GEN_OBJ)/%.cpp: $(MOD_GEN_C_INC)
$(MOD_C_PREPROCESSED): $(MOD_GEN_OBJ)/%.cpp: %.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CPPFLAGS) $(CXXFLAGS) -E $< -o $@

$(MOD_C_PICS): CFLAGS+=$(MOD_CFLAGS) -DDYNAMIC_BUILD
$(MOD_C_PICS): $(MOD_GEN_OBJ)/%.pic: $(MOD_GEN_C_INC)
$(MOD_C_PICS): $(MOD_GEN_OBJ)/%.pic: %.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ -fPIC

$(MOD_C_PREPROC): CFLAGS+=$(MOD_CFLAGS)
$(MOD_C_PREPROC): $(MOD_GEN_CPP)/%.i: $(MOD_GEN_C_INC)
$(MOD_C_PREPROC): $(MOD_GEN_CPP)/%.i: %.c
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CPPFLAGS) -E -DPREPROC -DPM2_NOINLINE $(CFLAGS) $< > $@

# Dependances vers *.cpp
#---------------------------------------------------------------------
$(MOD_CXX_OBJECTS): CXXFLAGS+=$(MOD_CFLAGS) $(MOD_CXXFLAGS)
$(MOD_CXX_OBJECTS): CXXFLAGS+=$($<-cflags)
$(MOD_CXX_OBJECTS): CXXFLAGS+=$($<-cxxflags)
$(MOD_CXX_OBJECTS): $(MOD_GEN_OBJ)/%.o: %.cpp
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(MOD_CXX_PICS): CXXFLAGS+=$(MOD_CFLAGS) $(MOD_CXXFLAGS)
$(MOD_CXX_PICS): $(MOD_GEN_OBJ)/%.pic: %.cpp
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@ -fPIC

$(MOD_CXX_PREPROC): CXXFLAGS+=$(MOD_CFLAGS) $(MOD_CXXFLAGS)
$(MOD_CXX_PREPROC): $(MOD_GEN_CPP)/%.cppi: %.cpp
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CXX) $(CPPFLAGS) -E -DPREPROC $(CXXFLAGS) $< > $@

# Dependances vers *.S
#---------------------------------------------------------------------
$(MOD_S_OBJECTS): CFLAGS+=$(MOD_CFLAGS) $(MOD_AFLAGS)
$(MOD_S_OBJECTS): $(MOD_GEN_OBJ)/%.o: %.S
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CPPFLAGS) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

$(MOD_S_PICS): CFLAGS+=$(MOD_CFLAGS) $(MOD_AFLAGS)
$(MOD_S_PICS): $(MOD_GEN_OBJ)/%.pic: %.S
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CPPFLAGS) $(CFLAGS) -x assembler-with-cpp -c $< -o $@ -fPIC

$(MOD_S_PREPROC): CFLAGS+=$(MOD_CFLAGS) $(MOD_AFLAGS)
$(MOD_S_PREPROC): $(MOD_GEN_CPP)/%.si: %.S
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CPPFLAGS) -E -DPREPROC $(CFLAGS) $< > $@

# Dependances vers *.f90
#---------------------------------------------------------------------
$(MOD_F_OBJECTS): FFLAGS+=$(MOD_FFLAGS)
$(MOD_F_OBJECTS): $(MOD_GEN_OBJ)/%.o: %.f90
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CPPFLAGS) $(FFLAGS) -c $< -o $@

$(MOD_F_PICS): FFLAGS+=$(MOD_FFLAGS)
$(MOD_F_PICS): $(MOD_GEN_OBJ)/%.pic: %.f90
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CPPFLAGS) $(FFLAGS) -c $< -o $@ -fPIC

$(MOD_F_PREPROC): FFLAGS+=$(MOD_FFLAGS)
$(MOD_F_PREPROC): $(MOD_GEN_CPP)/%.si: %.f90
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CPPFLAGS) -E -DPREPROC $(FFLAGS) $< > $@


# Dependances vers *.i
#---------------------------------------------------------------------
$(MOD_FUT): $(MOD_GEN_CPP)/%.fut: $(MOD_GEN_CPP)/%.i
	$(COMMON_BUILD)
	$(COMMON_HIDE) cp /dev/null $@
	$(COMMON_HIDE) touch $@
	$(COMMON_MAIN) $(CC) -c -O0 $< -o /tmp/foo-$$$$.o && \
	nm /tmp/foo-$$$$.o | grep this_is_the_ | sed -e 's/^.*this_is_the_\(.*_code\).*/\1/' >> $@ && \
	rm -f /tmp/foo-$$$$.o
	$(COMMON_HIDE) touch $(MOD_GEN_STAMP)/fut_stamp

# Fichiers *.h découpés
#---------------------------------------------------------------------
#ifneq ($(strip $(MOD_HSPLITS_PARTS)),)
dot_h: $(MOD_HSPLITS_MAKEFILES)
dot_h_all: dot_h
#endif

ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
ifneq ($(wildcard $(MOD_HSPLITS_MAKEFILES)),)
-include $(MOD_HSPLITS_MAKEFILES)
endif # MOD_HSPLITS_MAKEFILES
endif # $(MAKECMDGOALS)

#$(MOD_HSPLITS_MAKEFILE): $(MOD_HSPLITS_COMMON_DEP) \
#		$(foreach PART, $(MOD_HSPLITS_PARTS), \
#			$(addprefix $($(PART)_SRCDIR)/,$($(PART)_SOURCES)))
#	$(COMMON_BUILD)
#	$(COMMON_MAIN) $(PM2_OBJROOT)/bin/pm2-split-h-file \
#		--makefile $@ \
#		--gendir $(MOD_GEN_INC) \
#		--masterfile $(MODULE)-master.h \
#		$(foreach PART, $(MOD_HSPLITS_PARTS), \
#			$(addprefix --srcdir "" --start-file ,$($(PART)_COMMON)) \
#			$(addprefix --srcdir ,$($(PART)_SRCDIR)) \
#			$(addprefix --gensubdir ,$($(PART)_GENSUBDIR)) \
#			$(addprefix --default-section ,$($(PART)_DEFAULT_SECTION)) \
#			$($(PART)_SOURCES) )

# Lex
#---------------------------------------------------------------------
$(MOD_GEN_C_L_SOURCES): $(MOD_GEN_SRC)/%.c: %.l
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(LEX) $<
	$(COMMON_HIDE) mv lex.yy.c $@

# Yacc
#---------------------------------------------------------------------
$(MOD_GEN_C_Y_SOURCES): $(MOD_GEN_SRC)/%.c: %.y
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(YACC) $<
	$(COMMON_HIDE) mv y.tab.c $@
	$(COMMON_HIDE) mv y.tab.h $(MOD_GEN_C_TO_H)

$(MOD_GEN_C_Y_INC): $(MOD_GEN_INC)/%.h: %.y
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(YACC) $<
	$(COMMON_HIDE) mv y.tab.h $@
	$(COMMON_HIDE) mv y.tab.c $(MOD_GEN_H_TO_C)

# Regles de nettoyage
#---------------------------------------------------------------------
.PHONY: clean cleanall refresh refreshall sos
clean cleanall refresh refreshall sos:
	$(COMMON_HIDE) $(MAKE) -C $(PM2_OBJROOT) $@

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

bannerhelp: bannerhelpmodule

bannerhelpmodule:
	@echo "This is PM2 Makefile for module $(MODULE)"

targethelp: targethelplibs

targethelplibs:
	@echo "  objs|all: build the module objects (and libraries if any)"
	@echo "  link: link the module libraries or programs"
	@echo "  examples: build the example of this module (if any)"
	@echo "  help: this help"
	@echo "  clean: clean module source tree for current flavor"
#	@echo "  distclean: clean module source tree for all flavors"


# GNU Flymake, for Emacs.
#---------------------------------------------------------------------
.PHONY: check-syntax
check-syntax: CFLAGS += -S
check-syntax: $(foreach obj,$(CHK_SOURCES:%.c=%.o),$(MOD_GEN_OBJ)/$(notdir $(obj)))
	-rm -f $^

# paths pour éviter d'avoir à spécifier le chemin exact tout le temps
# permet de rajouter facilement d'autres répertoires sources
#---------------------------------------------------------------------
vpath %.c $(MOD_SRC)
vpath %.cpp $(MOD_SRC)
vpath %.c $(MOD_GEN_SRC)
vpath %.S $(MOD_SRC)
vpath %.f90 $(MOD_SRC)
vpath %.o $(MOD_GEN_OBJ)
vpath %.pic $(MOD_GEN_OBJ)
vpath %.d $(MOD_GEN_DEP)
vpath %.i $(MOD_GEN_CPP)
vpath %.cppi $(MOD_GEN_CPP)
vpath %.si $(MOD_GEN_CPP)
vpath %.l $(MOD_SRC)
vpath %.y $(MOD_SRC)
