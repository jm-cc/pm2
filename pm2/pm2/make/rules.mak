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

include $(MARCEL_ROOT)/make/rules.mak

ifeq ($(MAD2),yes)
$(error Not yet implemented)
else
include $(MAD1_ROOT)/make/rules.mak
endif

ifeq ($(PM2_USE_DSM),yes)
include $(DSM_ROOT)/make/rules.mak
endif

# Sources
PM2_C_SOURCES	:=	$(wildcard $(PM2_SRC)/*.c)
PM2_S_SOURCES	:=	$(wildcard $(PM2_SRC)/*.S)

# Objets
PM2_C_OBJECTS	:=	$(patsubst %.c,%$(COMMON_EXT).o,$(subst $(PM2_SRC),$(PM2_OBJ),$(PM2_C_SOURCES)))
PM2_S_OBJECTS	:=	$(patsubst %.S,%$(COMMON_EXT).o,$(subst $(PM2_SRC),$(PM2_OBJ),$(PM2_S_SOURCES)))
PM2_OBJECTS	:=	$(PM2_C_OBJECTS) $(PM2_S_OBJECTS)

# Dependances
PM2_C_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(PM2_OBJ),$(PM2_DEP),$(PM2_C_OBJECTS)))
PM2_S_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(PM2_OBJ),$(PM2_DEP),$(PM2_S_OBJECTS)))
PM2_DEPENDS	:=	$(strip $(PM2_C_DEPENDS) $(PM2_S_DEPENDS))

# "Convertisseurs" utiles
PM2_DEP_TO_OBJ	=	$(PM2_OBJ)/$(patsubst %.d,%.o,$(notdir $@))
PM2_OBJ_TO_S	=	$(PM2_SRC)/$(patsubst %$(COMMON_EXT).o,%.s,$(notdir $@))

# Affichage
ifeq ($(PM2_MAK_VERB),verbose)
PM2_PREFIX	:=	
PM2_HIDE	:=	
else
ifeq ($(PM2_MAK_VERB),normal)
PM2_PREFIX	:=	
PM2_HIDE	=	@
else
ifeq ($(PM2_MAK_VERB),quiet)
PM2_PREFIX	=	@ echo "   PM2: building" $(@F) ;
PM2_HIDE	:=	@
else  # silent
PM2_PREFIX	=	@
PM2_HIDE	:=	@
endif
endif
endif

.PHONY: pm2_default
pm2_default: $(PM2_LIB) $(PM2_USER_MAK)

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(PM2_DEPENDS)),$(PM2_DEPENDS))
include $(PM2_DEPENDS)
endif
endif

$(PM2_USER_MAK):
	$(PM2_HIDE) rm -f $(PM2_ROOT)/make/user*.mak
	$(PM2_HIDE) echo PM2_CFLAGS = $(COMMON_CFLAGS) > $(PM2_USER_MAK)
	$(PM2_HIDE) echo PM2_LDFLAGS = $(COMMON_LDFLAGS) >> $(PM2_USER_MAK)

$(PM2_EXTRA_DEP_FILE):
	$(PM2_HIDE) rm -f $(PM2_ROOT)/.opt*
	$(PM2_HIDE) cp /dev/null $(PM2_EXTRA_DEP_FILE)

$(PM2_OBJECTS): $(PM2_OBJ)/%.o: $(PM2_DEP)/%.d $(COMMON_MAKEFILES)
$(PM2_DEPENDS): $(COMMON_MAKEFILES)

$(PM2_LIB_A): $(PM2_OBJECTS)
	$(PM2_HIDE) rm -f $(PM2_LIB_A)
	$(PM2_HIDE) rm -f $(PM2_LIB_SO)
	$(PM2_PREFIX) ar cr $(PM2_LIB_A) $(PM2_OBJECTS)

$(PM2_LIB_SO): $(PM2_OBJECTS)
	$(PM2_HIDE) rm -f $(PM2_LIB_A)
	$(PM2_HIDE) rm -f $(PM2_LIB_SO)
	$(PM2_PREFIX) ld -Bdynamic -shared -o $(PM2_LIB_SO) $(PM2_OBJECTS)

$(PM2_C_OBJECTS): $(PM2_OBJ)/%$(COMMON_EXT).o: $(PM2_SRC)/%.c
	$(PM2_PREFIX) $(PM2_CC) $(PM2_KCFLAGS) -c $< -o $@

$(PM2_C_DEPENDS): $(PM2_DEP)/%$(COMMON_EXT).d: $(PM2_SRC)/%.c
	$(PM2_PREFIX) $(SHELL) -ec '$(PM2_CC) -MM $(PM2_KCFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PM2_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(PM2_S_OBJECTS): $(PM2_OBJ)/%$(COMMON_EXT).o: $(PM2_SRC)/%.S
	$(PM2_HIDE) $(PM2_CC) -E -P $(PM2_ASFLAGS) $< > $(PM2_OBJ_TO_S)
	$(PM2_PREFIX) $(PM2_AS) $(PM2_ASFLAGS) -c $(PM2_OBJ_TO_S) -o $@
	$(PM2_HIDE) rm -f $(PM2_OBJ_TO_S)

$(PM2_S_DEPENDS): $(PM2_DEP)/%$(COMMON_EXT).d: $(PM2_SRC)/%.S
	$(PM2_PREFIX) $(SHELL) -ec '$(PM2_CC) -MM $(PM2_KCFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PM2_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


.PHONY: pm2clean pm2distclean
pm2clean:
		$(PM2_HIDE) rm -f $(wildcard $(PM2_LIBD)/*.a $(PM2_OBJ)/*.o \
		$(PM2_DEP)/*.d \
		$(PM2_ROOT)/examples/*/depend/*.d \
		$(PM2_ROOT)/examples/*/obj/$(PM2_ARCH_SYS)/*.o \
		$(PM2_ROOT)/examples/*/bin/$(PM2_ARCH_SYS)/* \
		$(PM2_ROOT)/console/bin/$(PM2_ARCH_SYS)/* \
		$(PM2_ROOT)/console/text/obj/$(PM2_ARCH_SYS)/*.o \
		$(PM2_ROOT)/console/graphic/obj/$(PM2_ARCH_SYS)/*.o \
		$(PM2_ROOT)/.opt* \
		$(PM2_ROOT)/make/user*.mak)

pm2distclean:
		$(PM2_HIDE) rm -rf $(wildcard $(PM2_ROOT)/lib \
		$(PM2_ROOT)/source/obj \
		$(PM2_ROOT)/source/depend \
		$(PM2_ROOT)/examples/*/depend \
		$(PM2_ROOT)/examples/*/obj \
		$(PM2_ROOT)/examples/*/bin \
		$(PM2_ROOT)/console/bin \
		$(PM2_ROOT)/console/text/obj \
		$(PM2_ROOT)/console/graphic/obj \
		$(PM2_ROOT)/.opt* \
		$(PM2_ROOT)/make/user*.mak)


######################## Applications ########################

ifdef SRC_DIR

# Sources, objets et dependances
SOURCES	:=	$(wildcard $(SRC_DIR)/*.c)
OBJECTS	:=	$(patsubst %.c,%$(COMMON_EXT).o,$(subst $(SRC_DIR),$(OBJ_DIR),$(SOURCES)))
DEPENDS	:=	$(patsubst %.c,%$(COMMON_EXT).d,$(subst $(SRC_DIR),$(DEP_DIR),$(SOURCES)))

# Convertisseurs utiles
DEP_TO_OBJ	=	$(OBJ_DIR)/$(patsubst %.d,%.o,$(notdir $@))

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(wildcard $(DEPENDS)),)
include $(wildcard $(DEPENDS))
endif
endif


$(DEPENDS): $(COMMON_MAKEFILES)
$(OBJECTS): $(OBJ_DIR)/%.o: $(DEP_DIR)/%.d $(COMMON_MAKEFILES)


$(OBJ_DIR)/%$(COMMON_EXT).o: $(SRC_DIR)/%.c
	$(PM2_PREFIX) $(PM2_CC) $(PM2_CFLAGS) $(PM2_APP_CFLAGS) -c $< -o $@

$(DEPENDS): $(DEP_DIR)/%$(COMMON_EXT).d: $(SRC_DIR)/%.c
	$(PM2_PREFIX) $(SHELL) -ec '$(PM2_CC) -MM $(PM2_CFLAGS) $(PM2_APP_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(BIN_DIR)/%: $(OBJ_DIR)/%$(COMMON_EXT).o $(COMMON_LIBS)
	$(PM2_PREFIX) $(PM2_CC) $(PM2_CFLAGS) $(PM2_APP_LDFLAGS) $^ -o $@$(COMMON_EXT) $(PM2_LDFLAGS)

%: $(BIN_DIR)/% ;

endif

