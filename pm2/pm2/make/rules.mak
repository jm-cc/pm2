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

# CFLAGS et LDFLAGS
PM2_CC      :=  $(COMMON_CC)
PM2_AS      :=  $(COMMON_AS)
PM2_CFLAGS  :=  $(COMMON_CFLAGS)
PM2_KCFLAGS :=  $(PM2_CFLAGS) -DPM2_KERNEL 
PM2_LDFLAGS :=  $(COMMON_LDFLAGS)
PM2_ASFLAGS :=  $(COMMON_ASFLAGS)

# Target subdirectories
ifneq ($(MAKECMDGOALS),distclean)
DUMMY_BUILD :=  $(shell mkdir -p $(PM2_GEN_DEP))
DUMMY_BUILD :=  $(shell mkdir -p $(PM2_GEN_ASM))
DUMMY_BUILD :=  $(shell mkdir -p $(PM2_GEN_OBJ))
endif

# Sources
PM2_C_SOURCES :=  $(wildcard $(PM2_SRC)/*.c)
PM2_S_SOURCES :=  $(wildcard $(PM2_SRC)/*.S)

# Objets
PM2_C_OBJECTS :=  $(patsubst %.c,%.o,$(subst $(PM2_SRC),$(PM2_GEN_OBJ),$(PM2_C_SOURCES)))
PM2_S_OBJECTS :=  $(patsubst %.S,%.o,$(subst $(PM2_SRC),$(PM2_GEN_OBJ),$(PM2_S_SOURCES)))
PM2_OBJECTS   :=  $(PM2_C_OBJECTS) $(PM2_S_OBJECTS)

# Dependances
PM2_C_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(PM2_GEN_OBJ),$(PM2_GEN_DEP),$(PM2_C_OBJECTS)))
PM2_S_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(PM2_GEN_OBJ),$(PM2_GEN_DEP),$(PM2_S_OBJECTS)))
PM2_DEPENDS   :=  $(strip $(PM2_C_DEPENDS) $(PM2_S_DEPENDS))

# "Convertisseurs" utiles
PM2_DEP_TO_OBJ =  $(PM2_GEN_OBJ)/$(patsubst %.d,%.o,$(notdir $@))
PM2_OBJ_TO_S   =  $(PM2_GEN_ASM)/$(patsubst %.o,%.s,$(notdir $@))

ifeq ($(MAK_VERB),quiet)
PM2_PREFIX =  @ echo "   PM2: building" $(@F) ;
else
PM2_PREFIX =  $(COMMON_PREFIX)
endif

.PHONY: pm2_default
pm2_default: $(PM2_LIB) marcel_default dsm_default mad_default tbx_default ntbx_default

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(PM2_DEPENDS)),$(PM2_DEPENDS))
include $(PM2_DEPENDS)
endif
endif

$(PM2_DEPENDS): $(COMMON_DEPS)
$(PM2_OBJECTS): $(PM2_GEN_OBJ)/%.o: $(PM2_GEN_DEP)/%.d $(COMMON_DEPS)

$(PM2_LIB_A): $(PM2_OBJECTS)
	$(COMMON_HIDE) rm -f $(PM2_LIB_A)
	$(PM2_PREFIX) ar cr $(PM2_LIB_A) $(PM2_OBJECTS)

$(PM2_LIB_SO): $(PM2_OBJECTS)
	$(COMMON_HIDE) rm -f $(PM2_LIB_SO)
	$(PM2_PREFIX) ld -Bdynamic -shared -o $(PM2_LIB_SO) $(PM2_OBJECTS)

$(PM2_C_OBJECTS): $(PM2_GEN_OBJ)/%.o: $(PM2_SRC)/%.c
	$(PM2_PREFIX) $(PM2_CC) $(PM2_KCFLAGS) -c $< -o $@

$(PM2_C_DEPENDS): $(PM2_GEN_DEP)/%.d: $(PM2_SRC)/%.c
	$(PM2_PREFIX) $(SHELL) -ec '$(PM2_CC) -MM $(PM2_KCFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PM2_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(PM2_S_OBJECTS): $(PM2_GEN_OBJ)/%.o: $(PM2_SRC)/%.S
	$(COMMON_HIDE) $(PM2_CC) -E -P $(PM2_ASFLAGS) $< > $(PM2_OBJ_TO_S)
	$(PM2_PREFIX) $(PM2_AS) $(PM2_ASFLAGS) -c $(PM2_OBJ_TO_S) -o $@

$(PM2_S_DEPENDS): $(PM2_GEN_DEP)/%.d: $(PM2_SRC)/%.S
	$(PM2_PREFIX) $(SHELL) -ec '$(PM2_CC) -MM $(PM2_KCFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PM2_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


##############################################################################
