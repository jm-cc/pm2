
ifdef APP_RECURSIF

# Target subdirectories
DUMMY_BUILD :=  $(shell mkdir -p $(APP_BIN))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_OBJ))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_ASM))
DUMMY_BUILD :=  $(shell mkdir -p $(APP_DEP))
ifeq ($(wildcard $(DEPENDS)),$(DEPENDS))
include $(DEPENDS)
endif
#include $(wildcard $(DEPENDS))

examples: $(APPS)
.PHONY: examples

include $(PM2_ROOT)/make/common-rules.mak

all: examples

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

#MAKE_LIBS = set -e ; for module in $(PM2_MODULES); do \
#		$(MAKE) -C $(PM2_ROOT)/modules/$$module libs ; \
#	    done

MAKE_LIBS = $(MAKE) -C $(PM2_ROOT) libs

examples:
.PHONY: examples $(APPS)

include $(PM2_ROOT)/make/common-rules.mak

all: examples

examples:
	$(COMMON_HIDE) $(MAKE_LIBS)
	$(COMMON_HIDE) $(MAKE) APP_RECURSIF=true $@

.PHONY: clean appclean repclean libclean
clean: appclean repclean

libclean:
	$(COMMON_HIDE) for module in $(PM2_MODULES); do \
		$(MAKE) -C $(PM2_ROOT)/modules/$$module clean ; \
	done

appclean:
	$(COMMON_CLEAN) $(RM) $(APP_OBJ)/*$(APP_EXT).o \
		$(APP_DEP)/*$(APP_EXT).d $(APP_ASM)/*$(LIB_EXT).s \
		$(APPS)

repclean:
	$(COMMON_HIDE) for rep in $(APP_OBJ) $(APP_ASM) $(APP_BIN) \
		$(APP_DEP) ; do \
		if rmdir $$rep 2> /dev/null; then \
			echo "empty repertory $$rep removed" ; \
		fi ; \
	done

#distclean:
#	$(RM) -r build

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
#	@echo "  distclean: clean examples source tree for all flavors"
	@echo
	@echo "Examples to build:"
	@echo "  $(PROGSLIST)"

endif

$(PM2_MAK_DIR)/apps-config.mak: $(APP_STAMP_FLAVOR)
	$(COMMON_HIDE) $(PM2_GEN_MAK) apps

ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
$(PM2_MAK_DIR)/apps-libs.mak:  $(APP_STAMP_FLAVOR)
	$(COMMON_HIDE) mkdir -p `dirname $@`
	$(COMMON_HIDE) echo "CONFIG_MODULES= " `$(PM2_CONFIG) --modules` > $@
endif
