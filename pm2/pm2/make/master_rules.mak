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

COMMON_MAKEFILES +=   $(PM2_ROOT)/make/master_rules.mak
EMPTY :=
SPACE :=$(EMPTY) $(EMPTY)
OPT_FILE    :=  $(GEN_DIR)/.opt_$(subst $(SPACE),_,$(COMMON_OPTIONS))
COMMON_DEPS :=  $(COMMON_MAKEFILES) $(OPT_FILE)
COMMON_LDFLAGS += -L$(GEN_LIB)

# Regles principales
.PHONY: default
default: $(COMMON_DEFAULT_TARGET) $(GEN_DIR)/user.mak

.PHONY: clean app_clean
clean: app_clean
	$(COMMON_HIDE) rm -f $(wildcard \
                                 $(GEN_ASM)/*/*.s \
                                 $(GEN_INC)/*/*.h \
                                 $(GEN_SRC)/*/*.c \
                                 $(GEN_DEP)/*/*.d \
                                 $(GEN_OBJ)/*/*.o \
                                 $(GEN_LIB)/*     \
                                 $(GEN_BIN)/*/*   \
                                 $(GEN_BIN)/*     \
                                 $(GEN_DIR)/user.mak)

ifdef SRC_DIR
app_clean:
	$(COMMON_HIDE) rm -f $(wildcard \
                                 $(APP_DEP)/*/*.d \
                                 $(APP_OBJ)/*/*.o \
                                 $(APP_BIN)/*/*   \
                                 $(APP_BIN)/*)
else
app_clean: 
endif

.PHONY: distclean
distclean:
	$(COMMON_HIDE) rm -rf $(PM2_ROOT)/$(GEN) \
	$(COMMON_HIDE) rm -rf $(wildcard $(PM2_ROOT)/examples/*/$(GEN)) \
	$(COMMON_HIDE) rm -rf $(MAD1_ROOT)/$(GEN) \
	$(COMMON_HIDE) rm -rf $(wildcard $(MAD1_ROOT)/examples/*/$(GEN)) \
	$(COMMON_HIDE) rm -rf $(MAD2_ROOT)/$(GEN) \
	$(COMMON_HIDE) rm -rf $(wildcard $(MAD2_ROOT)/test/$(GEN)) \
	$(COMMON_HIDE) rm -rf $(TBX_ROOT)/$(GEN) \
	$(COMMON_HIDE) rm -rf $(NTBX_ROOT)/$(GEN) \
	$(COMMON_HIDE) rm -rf $(MARCEL_ROOT)/$(GEN) \
	$(COMMON_HIDE) rm -rf $(wildcard $(MARCEL_ROOT)/examples/$(GEN)) \
	$(COMMON_HIDE) rm -rf $(DSM_ROOT)/$(GEN)


$(OPT_FILE):
	$(COMMON_HIDE) rm -f $(GEN_DIR)/.opt*
	$(COMMON_HIDE) cp /dev/null $(OPT_FILE)

$(GEN_DIR)/user.mak: $(COMMON_DEPS)
	$(COMMON_HIDE) rm -f $(GEN_DIR)/user.mak 
	$(COMMON_HIDE) echo $(MASTER_MODULE)_CFLAGS = $(COMMON_CFLAGS) > $(GEN_DIR)/user.mak
	$(COMMON_HIDE) echo $(MASTER_MODULE)_LDFLAGS = $(COMMON_LDFLAGS) >> $(GEN_DIR)/user.mak
	$(COMMON_HIDE) echo $(MASTER_MODULE)_LLIBS = $(COMMON_LLIBS) >> $(GEN_DIR)/user.mak

##############################################################################
######################## Applications ########################

ifdef SRC_DIR

# Sources, objets et dependances
SOURCES :=  $(wildcard $(SRC_DIR)/*.c)
OBJECTS :=  $(patsubst %.c,%.o,$(subst $(SRC_DIR),$(APP_OBJ),$(SOURCES)))
DEPENDS :=  $(patsubst %.c,%.d,$(subst $(SRC_DIR),$(APP_DEP),$(SOURCES)))

# Convertisseurs utiles
DEP_TO_OBJ =  $(APP_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(wildcard $(DEPENDS)),)
include $(wildcard $(DEPENDS))
endif
endif


$(DEPENDS): $(COMMON_DEPS)
$(OBJECTS): $(APP_OBJ)/%.o: $(APP_DEP)/%.d $(COMMON_DEPS)


$(APP_OBJ)/%.o: $(SRC_DIR)/%.c
	$(COMMON_PREFIX) $(COMMON_CC) $(COMMON_CFLAGS) $(APP_CFLAGS) -c $< -o $@

$(DEPENDS): $(APP_DEP)/%.d: $(SRC_DIR)/%.c
	$(COMMON_PREFIX) $(SHELL) -ec '$(COMMON_CC) -MM $(COMMON_CFLAGS) $(APP_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(APP_BIN)/%: $(APP_OBJ)/%.o $(COMMON_LLIBS)
	$(COMMON_PREFIX) $(COMMON_CC) $(COMMON_CFLAGS) $(APP_LDFLAGS) $^ -o $@ $(COMMON_LDFLAGS) $(COMMON_LLIBS)

%: $(APP_BIN)/% ;

endif

