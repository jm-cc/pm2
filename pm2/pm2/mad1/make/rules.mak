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
MAD_CC      :=  $(COMMON_CC)
MAD_CFLAGS  :=  $(COMMON_CFLAGS)
MAD_KCFLAGS :=  $(MAD_CFLAGS) -DMAD_KERNEL 
MAD_LDFLAGS :=  $(COMMON_LDFLAGS)

# Target subdirectories
ifneq ($(MAKECMDGOALS),distclean)
DUMMY_BUILD :=  $(shell mkdir -p $(MAD_GEN_DEP))
DUMMY_BUILD :=  $(shell mkdir -p $(MAD_GEN_OBJ))
endif

# Sources
MAD_REG_SOURCES :=  $(wildcard $(MAD_SRC)/*.c)
MAD_NET_SOURCES :=  $(wildcard $(MAD_NET_SRC)/*.c)

DUM	:=	$(shell echo "["$(MAD_NET_SRC)"]" > /dev/tty)

# Objets
MAD_REG_OBJECTS :=  $(patsubst %.c,%.o,$(subst $(MAD_SRC),$(MAD_GEN_OBJ),$(MAD_REG_SOURCES)))
MAD_NET_OBJECTS :=  $(patsubst %.c,%.o,$(subst $(MAD_NET_SRC),$(MAD_GEN_OBJ),$(MAD_NET_SOURCES)))
MAD_OBJECTS     :=  $(MAD_REG_OBJECTS) $(MAD_NET_OBJECTS)

# Dependances
MAD_REG_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(MAD_GEN_OBJ),$(MAD_GEN_DEP),$(MAD_REG_OBJECTS)))
MAD_NET_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(MAD_GEN_OBJ),$(MAD_GEN_DEP),$(MAD_NET_OBJECTS)))
MAD_DEPENDS     :=  $(strip $(MAD_REG_DEPENDS) $(MAD_NET_DEPENDS))

# "Convertisseurs" utiles
MAD_DEP_TO_OBJ   =  $(MAD_GEN_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

ifeq ($(MAK_VERB),quiet)
MAD_PREFIX  =  @ echo "   MADELEINE: building" $(@F) ;
else
MAD_PREFIX  =  $(COMMON_PREFIX)
endif

.PHONY: mad_default
mad_default: $(MAD_LIB) tbx_default ntbx_default

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(MAD_DEPENDS)),$(MAD_DEPENDS))
include $(MAD_DEPENDS)
endif
endif

$(MAD_DEPENDS): $(COMMON_DEPS)
$(MAD_OBJECTS): $(MAD_GEN_OBJ)/%.o: $(MAD_GEN_DEP)/%.d $(COMMON_DEPS)

$(MAD_LIB_A): $(MAD_OBJECTS)
	$(COMMON_HIDE) rm -f $(MAD_LIB_A)
	$(MAD_PREFIX) ar cr $(MAD_LIB_A) $(MAD_OBJECTS)

$(MAD_LIB_SO): $(MAD_OBJECTS)
	$(COMMON_HIDE) rm -f $(MAD_LIB_SO)
	$(MAD_PREFIX) ld -Bdynamic -shared -o $(MAD_LIB_SO) $(MAD_OBJECTS)

$(MAD_REG_OBJECTS): $(MAD_GEN_OBJ)/%.o: $(MAD_SRC)/%.c
	$(MAD_PREFIX) $(MAD_CC) $(MAD_KCFLAGS) -c $< -o $@

$(MAD_REG_DEPENDS): $(MAD_GEN_DEP)/%.d: $(MAD_SRC)/%.c
	$(MAD_PREFIX) $(SHELL) -ec '$(MAD_CC) -MM $(MAD_KCFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(MAD_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(MAD_NET_OBJECTS): $(MAD_GEN_OBJ)/%.o: $(MAD_NET_SOURCES)
	$(MAD_PREFIX) $(MAD_CC) $(MAD_KCFLAGS) -c $< -o $@

$(MAD_NET_DEPENDS): $(MAD_GEN_DEP)/%.d: $(MAD_NET_SOURCES)
	$(MAD_PREFIX) $(SHELL) -ec '$(MAD_CC) -MM $(MAD_KCFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(MAD_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

##############################################################################
