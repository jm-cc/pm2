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

###################### CUSTOMIZATION SECTION #####################

MAD_OPTIONS		= # -DMAD_NEXUS  # -DDEBUG

MAD_STANDALONE_CC       = gcc #egcc

# MAD_STANDALONE_OPTIONS	= -Di686 -fexpensive-optimizations -finline-functions -Dpentiumpro -mcpu=pentiumpro -march=pentiumpro -O6 -fomit-frame-pointer -Wall
MAD_STANDALONE_OPTIONS	=	-g -Wall -Werror # -O6 -DMAD_TIMING -DUSE_SAFE_MALLOC

################### END OF CUSTOMIZATION SECTION #################

ifndef MAD2_ROOT
MAD2_ROOT	=	$(PM2_ROOT)/mad2
endif

ifndef PM2_ARCH_SYS
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

MAD_MAKEFILE	:=	$(MAD2_ROOT)/Makefile

MAD_SRC		:=	$(MAD2_ROOT)/src
MAD_DEP		:=	$(MAD_SRC)/depend
MAD_OBJ		:=	$(MAD2_ROOT)/obj/$(PM2_ARCH_SYS)
MAD_INC		:=	$(MAD2_ROOT)/include
MAD_CI_INC	:=	$(MAD_INC)/connection_interfaces
MAD_CI_SRC	:=	$(MAD_SRC)/connection_interfaces
MAD_LIBD	=	$(MAD2_ROOT)/lib/$(PM2_ARCH_SYS)

ifndef PM2_FLAGS
MAD_OPTIONS    += $(MAD_STANDALONE_OPTIONS) -DNTBX -DNTBX_TCP -DTBX -DMAD2
MAD_ALONE       :=  true
endif

ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(wildcard $(MAD_DEP)),$(MAD_DEP))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAD_DEP))
endif

ifneq ($(wildcard $(MAD_OBJ)),$(MAD_OBJ))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAD_OBJ))
endif

ifneq ($(wildcard $(MAD_LIBD)),$(MAD_LIBD))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAD_LIBD))
endif
endif

MAD_NET_CFLAGS	:=
MAD_NET_SOURCES :=

ifdef PM2_FLAGS
MAD2_SUB_MAKEFILES := $(MAD2_ROOT)/make/custom/options.mak
else 
MAD_MAKEFILE	+= $(MAD2_ROOT)/make/protocols.mak
include $(MAD2_ROOT)/make/protocols.mak
MAD2_SUB_MAKEFILES := $(foreach SUB_MAKEFILE, $(MAD2_PROTOCOLS), $(MAD2_ROOT)/make/$(SUB_MAKEFILE)/options.mak)
ifndef TBX_ROOT
TBX_ROOT        := $(MAD2_ROOT)/../toolbox
endif
ifndef NTBX_ROOT
NTBX_ROOT       := $(TBX_ROOT)/net
endif
MAD2_SUB_MAKEFILES += $(TBX_ROOT)/Makefile $(NTBX_ROOT)/Makefile
endif

MAD_MAKEFILE	+=	$(MAD2_SUB_MAKEFILES)
MAD_LIB		=	$(MAD_LIBD)/libmad2.a



.PHONY: mad_default
mad_default: tbx_default ntbx_default $(MAD_LIB) 

include $(MAD2_SUB_MAKEFILES)

MAD_REG_SOURCES	:=	$(wildcard $(MAD_SRC)/*.c)
# MAD_NET_SOURCES is defined in ./make/custom/options.mak

MAD_REG_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(MAD_SRC),$(MAD_OBJ),$(MAD_REG_SOURCES)))
MAD_NET_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(MAD_CI_SRC),$(MAD_OBJ),$(MAD_NET_SOURCES)))
MAD_OBJECTS	:=	$(MAD_REG_OBJECTS) $(MAD_NET_OBJECTS)
MAD_LLIB	=	$(NET_LLIBS) $(ARCHDLIB)

MAD_LIBS	:=	$(MAD_LIB) $(TBX_LIB) $(NTBX_LIB)
MAD_LLIBS	:=	$(MAD_LLIB) $(TBX_LLIB) $(NTBX_LLIB) 

ifdef MAD_ALONE
MAD_CC          =       $(MAD_STANDALONE_CC)
else
MAD_CC		=	gcc
endif

MAD_CFLAGS	=	$(MAD_OPTIONS) $(MAD_NET_CFLAGS) $(PM2_FLAGS) \
			-I$(MAD_INC) -I$(TBX_ROOT)/include \
			-I$(NTBX_ROOT)/include \
			-I$(MAR_INC) -I$(PM2_ROOT)/include

MAD_LDFLAGS	=	$(NET_LFLAGS)

MAD_REG_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(MAD_OBJ),$(MAD_DEP),$(MAD_REG_OBJECTS)))
MAD_NET_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(MAD_OBJ),$(MAD_DEP),$(MAD_NET_OBJECTS)))
MAD_DEPENDS	:=	($strip $(MAD_REG_DEPENDS) $(MAD_NET_DEPENDS))

MAD_DEP_TO_OBJ	=	$(MAD_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(MAD_DEPENDS)),$(MAD_DEPENDS))
include $(MAD_DEPENDS)
endif
endif

$(MAD_OBJECTS): $(MAD_OBJ)/%.o: $(MAD_DEP)/%.d
ifdef $(MAD_ALONE)
$(MAD_DEPENDS): $(MAD_MAKEFILE)
else
$(MAD_DEPENDS): $(PM2_MAKEFILE)
endif

$(MAD_LIB): $(MAD_OBJECTS)
	@ rm -f $(MAD_LIB)
	@ echo "  -  $(@F)"
	@ ar cr $(MAD_LIB) $(MAD_OBJECTS)

$(MAD_REG_OBJECTS): $(MAD_OBJ)/%.o: $(MAD_SRC)/%.c
	@ echo "  -  $(@F)"
	@ $(MAD_CC) $(MAD_CFLAGS) $(MAD_FLAGS) -c $< -o $@

$(MAD_REG_DEPENDS): $(MAD_DEP)/%.d: $(MAD_SRC)/%.c
	@ echo "  -  $(@F)"
	@ $(SHELL) -ec '$(MAD_CC) -MM $(MAD_CFLAGS) $(MAD_FLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(MAD_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(MAD_NET_OBJECTS): $(MAD_OBJ)/%.o: $(MAD_CI_SRC)/%.c
	@ echo "  -  $(@F)"
	@ $(MAD_CC) $(MAD_CFLAGS) $(MAD_FLAGS) -c $< -o $@

$(MAD_NET_DEPENDS): $(MAD_DEP)/%.d: $(MAD_CI_SRC)/%.c
	@ echo "  -  $(@F)"
	@ $(SHELL) -ec '$(MAD_CC) -MM $(MAD_CFLAGS) $(MAD_FLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(MAD_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

ifndef MAD_ALONE
.PHONY: madclean 
madclean: 
	@ echo "Cleaning Madeleine II"
	@ rm -f $(wildcard $(MAD_LIBD)/*.a $(MAD_OBJ)/*.o $(MAD_DEP)/*.d)

maddistclean:
	rm -rf $(wildcard $(MAD2_ROOT)/lib \
		$(MAD2_ROOT)/obj \
		$(MAD2_ROOT)/source/depend)
else
.PHONY: clean distclean
clean: tbxclean ntbxclean
	@ echo "Cleaning Madeleine II"
	@ rm -f $(wildcard $(MAD_LIBD)/*.a $(MAD_OBJ)/*.o $(MAD_DEP)/*.d) \

distclean: tbxdistclean ntbxdistclean
	rm -rf $(wildcard $(MAD2_ROOT)/lib \
		$(MAD2_ROOT)/obj \
		$(MAD2_ROOT)/source/depend)

######################## Applications ########################

ifdef SRC_DIR

ifeq ($(SRC_DIR),.)
SRC_DIR		:=	./
endif

THIS_MAKEFILE	:=	$(SRC_DIR)/Makefile

DEP_DIR	=	$(SRC_DIR)/depend
OBJ_DIR	=	$(SRC_DIR)/obj/$(PM2_ARCH_SYS)

ifndef BIN_DIR
BIN_DIR	=	$(SRC_DIR)/bin/$(PM2_ARCH_SYS)
endif

ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(wildcard $(DEP_DIR)),$(DEP_DIR))
DUMMY_BUILD	:=	$(shell mkdir -p $(DEP_DIR))
endif

ifneq ($(wildcard $(OBJ_DIR)),$(OBJ_DIR))
DUMMY_BUILD	:=	$(shell mkdir -p $(OBJ_DIR))
endif

ifneq ($(wildcard $(BIN_DIR)),$(BIN_DIR))
DUMMY_BUILD	:=	$(shell mkdir -p $(BIN_DIR))
endif
endif

SOURCES	:=	$(wildcard $(SRC_DIR)/*.c)
OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(SRC_DIR),$(OBJ_DIR),$(SOURCES)))
DEPENDS	:=	$(patsubst %.c,%.d,$(subst $(SRC_DIR),$(DEP_DIR),$(SOURCES)))

DEP_TO_OBJ	=	$(OBJ_DIR)/$(patsubst %.d,%.o,$(notdir $@))

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(wildcard $(DEPENDS)),)
include $(wildcard $(DEPENDS))
endif
endif

$(OBJECTS): $(OBJ_DIR)/%.o: $(DEP_DIR)/%.d
$(DEPENDS): $(THIS_MAKEFILE) $(MAD_MAKEFILE)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@ echo "  -  $(@F)"
	@ $(MAD_CC) $(MAD_CFLAGS) $(MAD_APP_CFLAGS) -c $< -o $@

$(DEPENDS): $(DEP_DIR)/%.d: $(SRC_DIR)/%.c
	@ echo "  -  $(@F)"
	@ $(SHELL) -ec '$(MAD_CC) -MM $(MAD_CFLAGS) $(MAD_APP_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(MAD_LIBS)
	@ echo "  -  $(@F)"
	@ $(MAD_CC) $(MAD_CFLAGS) $(MAD_LDFLAGS) $(MAD_APP_LDFLAGS) $^ -o $@ $(MAD_LLIBS) $(MAD_APP_LLIBS)

%: $(BIN_DIR)/% ;

endif

endif

