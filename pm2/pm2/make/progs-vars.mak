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

# needed at least by PM2_CONFIG --gen_mak progs
export PROGRAM

include $(PM2_ROOT)/make/common-vars.mak

-include $(PM2_MAK_DIR)/progs-libs.mak

$(PROGRAM):

ifndef PROGNAME
PROGNAME := $(PROGRAM)
endif

PRG_SRC := source

-include $(PM2_MAK_DIR)/progs-config.mak

ifneq ($($(PROGRAM)_CC),)
CC := $($(PROGRAM)_CC)
endif

ifneq ($($(PROGRAM)_AS),)
AS := $($(PROGRAM)_AS)
endif

PRG_REP_TO_BUILD := $(PRG_GEN_BIN) $(PRG_GEN_OBJ) $(PRG_GEN_ASM) \
	$(PRG_GEN_DEP) $(PRG_GEN_SRC) $(PRG_GEN_INC) $(PRG_GEN_STAMP)

# Program
PRG_PRG := $(PRG_GEN_BIN)/$(PROGNAME)$(PRG_EXT)

# Sources
PRG_L_SOURCES :=  $(wildcard $(PRG_SRC)/*.l)
PRG_Y_SOURCES :=  $(wildcard $(PRG_SRC)/*.y)
PRG_C_SOURCES :=  $(wildcard $(PRG_SRC)/*.c)
PRG_S_SOURCES :=  $(wildcard $(PRG_SRC)/*.S)

# Generated sources
PRG_GEN_C_L_SOURCES :=  $(patsubst %.l,%$(PRG_EXT).c,$(subst $(PRG_SRC),$(PRG_GEN_SRC),$(PRG_L_SOURCES)))
PRG_GEN_C_Y_SOURCES :=  $(patsubst %.y,%$(PRG_EXT).c,$(subst $(PRG_SRC),$(PRG_GEN_SRC),$(PRG_Y_SOURCES)))
PRG_GEN_C_SOURCES   :=  $(PRG_GEN_C_L_SOURCES) $(PRG_GEN_C_Y_SOURCES)

# Generated includes
PRG_GEN_C_Y_INC :=  $(patsubst %.y,%$(PRG_EXT).h,$(subst $(PRG_SRC),$(PRG_GEN_INC),$(PRG_Y_SOURCES)))
PRG_GEN_C_INC :=  $(PRG_GEN_C_Y_INC)

# Objets
PRG_C_OBJECTS :=  $(patsubst %.c,%$(PRG_EXT).o,$(subst $(PRG_SRC),$(PRG_GEN_OBJ),$(PRG_C_SOURCES)))
PRG_S_OBJECTS :=  $(patsubst %.S,%$(PRG_EXT).o,$(subst $(PRG_SRC),$(PRG_GEN_OBJ),$(PRG_S_SOURCES)))
PRG_GEN_C_OBJECTS :=  $(patsubst %.c,%$(PRG_EXT).o,$(subst $(PRG_GEN_SRC),$(PRG_GEN_OBJ),$(PRG_GEN_C_SOURCES)))
PRG_OBJECTS   :=  $(PRG_C_OBJECTS) $(PRG_S_OBJECTS) $(PRG_GEN_C_OBJECTS)

# Dependances
PRG_C_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(PRG_GEN_OBJ),$(PRG_GEN_DEP),$(PRG_C_OBJECTS)))
PRG_S_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(PRG_GEN_OBJ),$(PRG_GEN_DEP),$(PRG_S_OBJECTS)))
PRG_GEN_C_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(PRG_GEN_OBJ),$(PRG_GEN_DEP),$(PRG_GEN_C_OBJECTS)))
PRG_DEPENDS   :=  $(strip $(PRG_C_DEPENDS) $(PRG_S_DEPENDS) $(PRG_GEN_C_DEPENDS))

# "Convertisseurs" utiles
PRG_DEP_TO_OBJ   =  $(PRG_GEN_OBJ)/$(patsubst %.d,%.o,$(notdir $@))
PRG_OBJ_TO_S     =  $(PRG_GEN_ASM)/$(patsubst %.o,%.s,$(notdir $@))
PRG_PIC_TO_S     =  $(PRG_GEN_ASM)/$(patsubst %.pic,%.s,$(notdir $@))
PRG_GEN_C_TO_H   =  $(PRG_GEN_INC)/$(patsubst %.c,%.h,$(notdir $@))
PRG_GEN_H_TO_C   =  $(PRG_GEN_INC)/$(patsubst %.h,%.c,$(notdir $@))

COMMON_DEPS += $(PRG_STAMP_FLAVOR) $(PRG_STAMP_FILES) $(MAKEFILE_FILE) 

$(PM2_MAK_DIR)/progs-config.mak: $(PRG_STAMP_FLAVOR)
	@$(PM2_CONFIG) --gen_mak progs

$(PM2_MAK_DIR)/progs-libs.mak: $(PRG_STAMP_FLAVOR)
	@mkdir -p `dirname $@`
	@echo "CONFIG_MODULES= " `$(PM2_CONFIG) --modules` > $@
