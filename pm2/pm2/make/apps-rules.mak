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

ifdef APP_RECURSIF

# Target subdirectories
DUMMY_BUILD :=  $(shell mkdir -p $(APP_BIN))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_OBJ))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_ASM))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_DEP))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_STAMP))
ifeq ($(wildcard $(DEPENDS)),$(DEPENDS))
include $(DEPENDS)
endif
#include $(wildcard $(DEPENDS))

examples: $(APPS)
.PHONY: examples

include $(PM2_ROOT)/make/common-rules.mak

all: examples

$(APP_STAMP)/stamp$(APP_EXT):
	$(COMMON_HIDE) touch $@

$(DEPENDS): $(COMMON_DEPS)
$(OBJECTS): $(APP_OBJ)/%.o: $(APP_DEP)/%.d $(COMMON_DEPS)

$(APP_OBJ)/%$(APP_EXT).o: $(SRC_DIR)/%.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(DEPENDS): $(APP_DEP)/%$(APP_EXT).d: $(SRC_DIR)/%.c
	$(COMMON_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(APP_BIN)/%: $(APP_OBJ)/%.o
	$(COMMON_PREFIX) $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

%: $(APP_BIN)/% ;

else 

MAKE_LIBS = for module in $(PM2_MODULES); do \
		$(MAKE) -C $(PM2_ROOT)/modules/$$module ; \
	    done

examples:
.PHONY: examples $(APPS)

include $(PM2_ROOT)/make/common-rules.mak

all: examples

examples:
	@$(MAKE_LIBS)
	@$(MAKE) APP_RECURSIF=true $@

.PHONY: clean appclean repclean
clean: appclean repclean

appclean:
	$(COMMON_CLEAN) $(RM) $(APP_OBJ)/*$(APP_EXT).o \
		$(APP_DEP)/*$(APP_EXT).d $(APP_ASM)/*$(LIB_EXT).s \
		$(APPS)

repclean:
	@for rep in $(APP_OBJ) $(APP_ASM) $(APP_BIN) \
		$(APP_DEP) $(APP_STAMP); do \
		if rmdir $$rep 2> /dev/null; then \
			echo "empty repertory $$rep removed" ; \
		fi ; \
	done

distclean:
	$(RM) -r build

$(PROGS):
	@$(MAKE_LIBS)
	@$(MAKE) APP_RECURSIF=true $@$(APP_EXT)

config:
	$(PM2_ROOT)/bin/pm2_create-flavor --flavor=$(FLAVOR) --ext=$(EXT) \
		--builddir=$(BUILDDIR) --modules="$(CONFIG_MODULES)" \
		--all="$(ALL_OPTIONS)" --marcel="$(MARCEL_OPTIONS)" \
		--mad="$(MAD_OPTIONS)" --pm2="$(PM2_OPTIONS)" \
		--dsm="$(DSM_OPTIONS)" --common="$(COMMON_OPTIONS)"

.PHONY: help bannerhelpapps targethelpapps
help: globalhelp

bannerhelp: bannerhelpapps

bannerhelpapps:
	@echo "This is PM2 Makefile for examples"

targethelp: targethelpapps

PROGSLIST:=$(foreach PROG,$(PROGS),$(PROG))
targethelpapps:
	@echo "  all|examples: build the examples"
	@echo "  help: this help"
	@echo "  clean: clean examples source tree for current flavor"
	@echo "  distclean: clean examples source tree for all flavors"
	@echo
	@echo "Examples to build:"
	@echo "  $(PROGSLIST)"

endif

