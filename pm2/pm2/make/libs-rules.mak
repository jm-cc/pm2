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

libs: $(LIBRARY)

$(LIBRARY):

include $(PM2_ROOT)/make/common-rules.mak

ifneq ($(LIBRARY),$(LIBNAME))
$(LIBNAME): $(LIBRARY)
endif

all: libs

.PHONY: $(LIBRARY) $(LIBNAME) libs

ifdef SHOW_FLAVOR
$(LIBRARY): showflavor
endif

ifdef SHOW_FLAGS
$(LIBRARY): showflags
endif

$(LIBRARY): $(LIB_LIB)

.PHONY: showflags
showflags:
	@echo $(CFLAGS)

showflavor:
	@echo Compiling for flavor: $(FLAVOR)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)

# Target subdirectories
DUMMY_BUILD :=  $(foreach REP, $(LIB_REP_TO_BUILD), $(shell mkdir -p $(REP)))

ifeq ($(wildcard $(LIB_DEPENDS)),$(LIB_DEPENDS))
include $(LIB_DEPENDS)
endif
endif
endif

#$(LIB_GEN_STAMP)/$(LIBRARY)$(LIB_EXT): $(LIB_LIB)
#	$(COMMON_HIDE) touch $@

$(LIB_DEPENDS): $(COMMON_DEPS)
$(LIB_OBJECTS): $(LIB_GEN_OBJ)/%.o: $(LIB_GEN_DEP)/%.d $(COMMON_DEPS)
$(LIB_PICS): $(LIB_GEN_OBJ)/%.pic: $(LIB_GEN_DEP)/%.d $(COMMON_DEPS)

ifeq ($(LIB_GEN_OBJ),)
$(LIB_LIB_A):
	@echo "The flavor $(FLAVOR) do not need this library"
$(LIB_LIB_SO):
	@echo "The flavor $(FLAVOR) do not need this library"
else
$(LIB_LIB_A): $(LIB_OBJECTS)
	$(COMMON_HIDE) rm -f $(LIB_LIB_A)
	$(LIB_PREFIX) ar cr $(LIB_LIB_A) $(LIB_OBJECTS)
	$(COMMON_HIDE) touch $(LIB_STAMP_FILE)

$(LIB_LIB_SO): $(LIB_PICS)
	$(COMMON_HIDE) rm -f $(LIB_LIB_SO)
	$(LIB_PREFIX) $(LD) -shared -o $(LIB_LIB_SO) $(LIB_PICS)
	$(COMMON_HIDE) touch $(LIB_STAMP_FILE)
endif

$(LIB_C_OBJECTS): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: $(LIB_SRC)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(LIB_C_PICS): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: $(LIB_SRC)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -fPIC -c $< -o $@

$(LIB_C_DEPENDS): $(LIB_GEN_DEP)/%$(LIB_EXT).d: $(LIB_SRC)/%.c
	$(LIB_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(LIB_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(LIB_S_OBJECTS): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: $(LIB_SRC)/%.S
	$(COMMON_HIDE) $(CC) -E -P $(CFLAGS) $< > $(LIB_OBJ_TO_S)
	$(LIB_PREFIX) $(AS) $(CFLAGS) -c $(LIB_OBJ_TO_S) -o $@

$(LIB_S_PICS): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: $(LIB_SRC)/%.S
	$(COMMON_HIDE) $(CC) -E -P $(CFLAGS) $< > $(LIB_PIC_TO_S)
	$(LIB_PREFIX) $(AS) $(CFLAGS) -fPIC -c $(LIB_PIC_TO_S) -o $@

$(LIB_S_DEPENDS): $(LIB_GEN_DEP)/%$(LIB_EXT).d: $(LIB_SRC)/%.S
	$(LIB_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(LIB_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

.PHONY: clean libclean repclean examplesclean distclean
clean: libclean repclean examplesclean

libclean:
	$(COMMON_CLEAN) $(RM) $(LIB_GEN_OBJ)/*$(LIB_EXT).{o,pic} \
		$(LIB_GEN_DEP)/*$(LIB_EXT).d $(LIB_GEN_ASM)/*$(LIB_EXT).s \
		$(LIB_LIB) \
		$(LIB_STAMP_FILE)

repclean:
	@for rep in $(LIB_REP_TO_BUILD); do \
		if rmdir $$rep 2> /dev/null; then \
			echo "empty repertory $$rep removed" ; \
		fi ; \
	done

examplesclean:
	@set -e; \
	if [ -d examples ]; then \
		$(MAKE) -C examples clean ; \
	fi

distclean:
	$(COMMON_CLEAN)$(RM) -r build $(LIB_GEN_STAMP)/libstamp-$(LIBRARY)*
	@set -e; \
	if [ -d examples ]; then \
		$(MAKE) -C examples distclean ; \
	fi

.PHONY: examples
examples:
	@set -e; \
	if [ -d examples ]; then \
		$(MAKE) -C examples ; \
	fi

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
	@echo "  distclean: clean module source tree for all flavors"
