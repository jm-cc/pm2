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

include $(PM2_ROOT)/make/common-vars.mak

libs:

ifndef LIBNAME
LIBNAME := $(LIBRARY)
endif

LIB_SRC := source

-include $(PM2_MAK_DIR)/$(LIBRARY)-config.mak

ifneq ($($(LIBRARY)_CC),)
CC := $($(LIBRARY)_CC)
endif

ifneq ($($(LIBRARY)_AS),)
AS := $($(LIBRARY)_AS)
endif

LIB_REP_TO_BUILD := $(LIB_GEN_LIB) $(LIB_GEN_OBJ) $(LIB_GEN_ASM) \
	$(LIB_GEN_DEP) $(LIB_GEN_STAMP) $(LIB_GEN_CPP)

# Bibliothèques
LIB_LIB_A=$(LIB_GEN_LIB)/lib$(LIBNAME)$(LIB_EXT).a
LIB_LIB_SO=$(LIB_GEN_LIB)/lib$(LIBNAME)$(LIB_EXT).so
ifeq ($(BUILD_STATIC_LIBS),true)
LIB_LIB += $(LIB_LIB_A)
endif
ifeq ($(BUILD_DYNAMIC_LIBS),true)
LIB_LIB += $(LIB_LIB_SO)
endif

# Sources
LIB_C_SOURCES :=  $(wildcard $(LIB_SRC)/*.c)
LIB_S_SOURCES :=  $(wildcard $(LIB_SRC)/*.S)

# Objets
LIB_C_OBJECTS :=  $(patsubst %.c,%$(LIB_EXT).o,$(subst $(LIB_SRC),$(LIB_GEN_OBJ),$(LIB_C_SOURCES)))
LIB_S_OBJECTS :=  $(patsubst %.S,%$(LIB_EXT).o,$(subst $(LIB_SRC),$(LIB_GEN_OBJ),$(LIB_S_SOURCES)))
LIB_OBJECTS   :=  $(LIB_C_OBJECTS) $(LIB_S_OBJECTS)

LIB_C_PICS :=  $(patsubst %.c,%$(LIB_EXT).pic,$(subst $(LIB_SRC),$(LIB_GEN_OBJ),$(LIB_C_SOURCES)))
LIB_S_PICS :=  $(patsubst %.S,%$(LIB_EXT).pic,$(subst $(LIB_SRC),$(LIB_GEN_OBJ),$(LIB_S_SOURCES)))
LIB_PICS   :=  $(LIB_C_PICS) $(LIB_S_PICS)

# Preprocs
LIB_C_PREPROC :=  $(patsubst %.c,%$(LIB_EXT).i,$(subst $(LIB_SRC),$(LIB_GEN_CPP),$(LIB_C_SOURCES)))
LIB_S_PREPROC :=  $(patsubst %.S,%$(LIB_EXT).si,$(subst $(LIB_SRC),$(LIB_GEN_CPP),$(LIB_S_SOURCES)))
LIB_PREPROC   :=  $(LIB_C_PREPROC) $(LIB_S_PREPROC)

# FUT entries
LIB_C_FUT :=  $(patsubst %.i,%.fut,$(LIB_C_PREPROC))
LIB_FUT   :=  $(LIB_C_FUT)

# Dependances
LIB_C_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(LIB_GEN_OBJ),$(LIB_GEN_DEP),$(LIB_C_OBJECTS)))
LIB_S_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(LIB_GEN_OBJ),$(LIB_GEN_DEP),$(LIB_S_OBJECTS)))
LIB_DEPENDS   :=  $(strip $(LIB_C_DEPENDS) $(LIB_S_DEPENDS))

# "Convertisseurs" utiles
LIB_DEP_TO_OBJ =  $(LIB_GEN_OBJ)/$(patsubst %.d,%.o,$(notdir $@))
LIB_OBJ_TO_S   =  $(LIB_GEN_ASM)/$(patsubst %.o,%.s,$(notdir $@))
LIB_PIC_TO_S   =  $(LIB_GEN_ASM)/$(patsubst %.pic,%.s,$(notdir $@))

COMMON_DEPS += $(LIB_STAMP_FLAVOR) $(MAKEFILE_FILE) 

$(PM2_MAK_DIR)/$(LIBRARY)-config.mak: $(LIB_STAMP_FLAVOR)
	@$(PM2_CONFIG) --gen_mak $(LIBRARY)
