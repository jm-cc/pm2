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
$(LIBRARY): $(LIB_LIB)

# Affichage des flags 
#---------------------------------------------------------------------
.PHONY: showflags
showflags:
	@echo Compiling using CFLAGS=$(CFLAGS)

# Affichage de la flavor 
#---------------------------------------------------------------------
showflavor:
	@echo Compiling for flavor: $(FLAVOR)

# - Construction des repertoires destination
# - inclusion des dependances
#---------------------------------------------------------------------
ifeq (,$(findstring _$(MAKECMDGOALS)_,_clean_ $(DO_NOT_GENERATE_MAK_FILES)))

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
ifeq ($(LIB_GEN_OBJ),)
$(LIB_LIB_A):
	@echo "The flavor $(FLAVOR) do not need this library"
$(LIB_LIB_SO):
	@echo "The flavor $(FLAVOR) do not need this library"
else
$(LIB_LIB_A): $(LIB_OBJECTS)
	$(COMMON_HIDE) rm -f $(LIB_LIB_A)
	$(LIB_PREFIX) ar cr $(LIB_LIB_A) $(LIB_OBJECTS)

$(LIB_LIB_SO): $(LIB_PICS)
	$(COMMON_HIDE) rm -f $(LIB_LIB_SO)
	$(LIB_PREFIX) $(LD) -shared -o $(LIB_LIB_SO) $(LIB_PICS)
endif

# Dependances vers *.c
#---------------------------------------------------------------------
$(LIB_C_OBJECTS): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: $(LIB_SRC)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(LIB_C_PICS): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: $(LIB_SRC)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -fPIC -c $< -o $@

$(LIB_C_DEPENDS): $(LIB_GEN_DEP)/%$(LIB_EXT).d: $(LIB_SRC)/%.c
	$(LIB_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) -DDEPEND $< \
		| sed '\''s|.*:|$(LIB_DEP_TO_OBJ) $@ :|g'\'' > $@'

$(LIB_C_PREPROC): $(LIB_GEN_CPP)/%$(LIB_EXT).i: $(LIB_SRC)/%.c
	$(LIB_PREFIX) $(CC) -E -P -DPREPROC $(CFLAGS) $< > $@

# Dependances vers *.S
#---------------------------------------------------------------------
$(LIB_S_OBJECTS): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: $(LIB_SRC)/%.S
	$(COMMON_HIDE) $(CC) -E -P $(CFLAGS) $< > $(LIB_OBJ_TO_S)
	$(LIB_PREFIX) $(AS) $(CFLAGS) -c $(LIB_OBJ_TO_S) -o $@

$(LIB_S_PICS): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: $(LIB_SRC)/%.S
	$(COMMON_HIDE) $(CC) -E -P $(CFLAGS) $< > $(LIB_PIC_TO_S)
	$(LIB_PREFIX) $(AS) $(CFLAGS) -fPIC -c $(LIB_PIC_TO_S) -o $@

$(LIB_S_DEPENDS): $(LIB_GEN_DEP)/%$(LIB_EXT).d: $(LIB_SRC)/%.S
	$(LIB_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) -DDEPEND $< \
		| sed '\''s|.*:|$(LIB_DEP_TO_OBJ) $@ :|g'\'' > $@'

$(LIB_S_PREPROC): $(LIB_GEN_CPP)/%$(LIB_EXT).si: $(LIB_SRC)/%.S
	$(LIB_PREFIX) $(CC) -E -P -DPREPROC $(CFLAGS) $< > $@


# Dependances vers *.i
#---------------------------------------------------------------------
$(LIB_FUT): $(LIB_GEN_CPP)/%.fut: $(LIB_GEN_CPP)/%.i
	$(LIB_PREFIX) cp /dev/null $@
	$(COMMON_HIDE) gcc -c -O0 $< -o /tmp/foo.o
	$(COMMON_HIDE) nm /tmp/foo.o | fgrep this_is_the_ | sed -e 's/^.*this_is_the_//' >> $@
	$(COMMON_HIDE) touch $(LIB_GEN_STAMP)/fut_stamp

# Regles de nettoyage
#---------------------------------------------------------------------
.PHONY: clean libclean repclean examplesclean distclean
clean: libclean repclean examplesclean

libclean:
	$(COMMON_CLEAN) $(RM) $(LIB_GEN_OBJ)/*$(LIB_EXT).{o,pic} \
		$(LIB_GEN_DEP)/*$(LIB_EXT).d $(LIB_GEN_ASM)/*$(LIB_EXT).s \
		$(LIB_LIB) \
		$(LIBRARY)-config.mak

ifneq ($(strip $(LIB_REP_TO_BUILD)),)
repclean:
	@echo "cleaning directories :-$(LIB_REP_TO_BUILD)-"
	@for rep in $(LIB_REP_TO_BUILD); do \
		if rmdir $$rep 2> /dev/null; then \
			echo "empty repertory $$rep removed" ; \
		fi ; \
	done
endif

examplesclean:
	@set -e; \
	if [ -d examples ]; then \
		$(MAKE) -C examples clean ; \
	fi

#distclean:
#	$(COMMON_CLEAN)$(RM) -r build $(LIB_GEN_STAMP)/libstamp-$(LIBRARY)*
#	@set -e; \
#	if [ -d examples ]; then \
#		$(MAKE) -C examples distclean ; \
#	fi

# Exemples
#---------------------------------------------------------------------
.PHONY: examples
examples:
	@set -e; \
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
	@echo "Generating $@"
	@$(PM2_CONFIG) --gen_mak $(LIBRARY)
endif

######################################################################
