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

# Sources
MAD_REG_SOURCES	:=	$(wildcard $(MAD_SRC)/*.c)
MAD_NET_SOURCES	:=	$(wildcard $(MAD_SRC)/$(NET_INTERF)/*.c)

# Objets
MAD_REG_OBJECTS	:=	$(patsubst %.c,%$(COMMON_EXT).o,$(subst $(MAD_SRC),$(MAD_OBJ),$(MAD_REG_SOURCES)))
MAD_NET_OBJECTS	:=	$(patsubst %.c,%$(COMMON_EXT).o,$(subst $(MAD_SRC)/$(NET_INTERF),$(MAD_OBJ),$(MAD_NET_SOURCES)))
MAD_OBJECTS	:=	$(MAD_REG_OBJECTS) $(MAD_NET_OBJECTS)

# Dependances
MAD_REG_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(MAD_OBJ),$(MAD_DEP),$(MAD_REG_OBJECTS)))
MAD_NET_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(MAD_OBJ),$(MAD_DEP),$(MAD_NET_OBJECTS)))
MAD_DEPENDS	:=	($strip $(MAD_REG_DEPENDS) $(MAD_NET_DEPENDS))

# "Convertisseurs" utiles
MAD_DEP_TO_OBJ	=	$(MAD_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

# Affichage
ifeq ($(MAD_MAK_VERB),verbose)
MAD_PREFIX	:=	
MAD_HIDE	:=	
else
ifeq ($(MAD_MAK_VERB),normal)
MAD_PREFIX	:=	
MAD_HIDE	=	@
else
ifeq ($(MAD_MAK_VERB),quiet)
MAD_PREFIX	=	@ echo "   MADELEINE: building" $(@F) ;
MAD_HIDE	:=	@
else  # silent
MAD_PREFIX	=	@
MAD_HIDE	:=	@
endif
endif
endif

.PHONY: mad_default
mad_default: $(MAD_LIB)

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(MAD_DEPENDS)),$(MAD_DEPENDS))
include $(MAD_DEPENDS)
endif
endif

$(MAD_DEPENDS): $(COMMON_MAKEFILES)
$(MAD_OBJECTS): $(MAD_OBJ)/%.o: $(MAD_DEP)/%.d $(COMMON_MAKEFILES)

$(MAD_LIB): $(MAD_OBJECTS)
ifdef PM2
	$(MAD_HIDE) rm -f $(MAD1_ROOT)/.mad_standalone
else
	$(MAD_HIDE) rm -f $(MAD1_ROOT)/.mad_pm2
endif
	$(MAD_HIDE) rm -f $(MAD_LIB)
	$(MAD_PREFIX) ar cr $(MAD_LIB) $(MAD_OBJECTS)
	$(MAD_HIDE) rm -f $(MAD1_ROOT)/make/user.mak
	$(MAD_HIDE) echo MAD_CFLAGS = $(COMMON_CFLAGS) > $(MAD1_ROOT)/make/user.mak
	$(MAD_HIDE) echo MAD_LDFLAGS = $(COMMON_LDFLAGS) >> $(MAD1_ROOT)/make/user.mak


$(MAD1_ROOT)/.mad_pm2:
	$(MAD_HIDE) cp /dev/null $(MAD1_ROOT)/.mad_pm2

$(MAD1_ROOT)/.mad_standalone:
	$(MAD_HIDE) cp /dev/null $(MAD1_ROOT)/.mad_standalone


$(MAD_REG_OBJECTS): $(MAD_OBJ)/%$(COMMON_EXT).o: $(MAD_SRC)/%.c
	$(MAD_PREFIX) $(MAD_CC) $(MAD_KCFLAGS) -c $< -o $@

$(MAD_REG_DEPENDS): $(MAD_DEP)/%$(COMMON_EXT).d: $(MAD_SRC)/%.c
	$(MAD_PREFIX) $(SHELL) -ec '$(MAD_CC) -MM $(MAD_KCFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(MAD_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(MAD_NET_OBJECTS): $(MAD_OBJ)/%$(COMMON_EXT).o: $(MAD_SRC)/$(NET_INTERF)/%.c
	$(MAD_PREFIX) $(MAD_CC) $(MAD_KCFLAGS) -c $< -o $@

$(MAD_NET_DEPENDS): $(MAD_DEP)/%$(COMMON_EXT).d: $(MAD_SRC)/$(NET_INTERF)/%.c
	$(MAD_PREFIX) $(SHELL) -ec '$(MAD_CC) -MM $(MAD_KCFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(MAD_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


.PHONY: madclean
madclean:
	$(MAD_HIDE) rm -f $(wildcard $(MAD_LIBD)/*.a $(MAD_OBJ)/*.o \
		$(MAD_DEP)/*.d \
		$(MAD1_ROOT)/examples/depend/*.d \
		$(MAD1_ROOT)/examples/obj/$(PM2_ARCH_SYS)/*.o \
		$(MAD1_ROOT)/examples/bin/$(PM2_ARCH_SYS)/* \
		$(MAD1_ROOT)/.mad_*)

maddistclean:
	$(MAD_HIDE) rm -rf $(wildcard $(MAD1_ROOT)/lib \
		$(MAD1_ROOT)/source/obj \
		$(MAD1_ROOT)/source/depend \
		$(MAD1_ROOT)/examples/depend \
		$(MAD1_ROOT)/examples/obj \
		$(MAD1_ROOT)/examples/bin \
		$(MAD1_ROOT)/.mad_*)

######################## Applications ########################

ifdef MAD_EX_DIR

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
	$(MAD_PREFIX) $(MAD_CC) $(MAD_CFLAGS) -c $< -o $@

$(DEPENDS): $(DEP_DIR)/%$(COMMON_EXT).d: $(SRC_DIR)/%.c
	$(MAD_PREFIX) $(SHELL) -ec '$(MAD_CC) -MM $(MAD_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(COMMON_LIBS)
	$(MAD_PREFIX) $(MAD_CC) $(MAD_CFLAGS) $^ -o $@$(COMMON_EXT) $(MAD_LDFLAGS)

%: $(BIN_DIR)/% ;

endif
