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

MAD_OPTIONS		=  # -DDEBUG

MAD_STANDALONE_OPTIONS	=	-g -Wall # -O6 -DMAD_TIMING -DUSE_SAFE_MALLOC

################### END OF CUSTOMIZATION SECTION #################

ifndef MAD2_ROOT
MAD2_ROOT	=	$(PM2_ROOT)/mad2
endif

ifndef PM2_ARCH_SYS
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

MAD_MAKEFILE	=	$(MAD2_ROOT)/Makefile

MAD_SRC		:=	$(MAD2_ROOT)/src
MAD_DEP		:=	$(MAD_SRC)/depend
MAD_OBJ		:=	$(MAD2_ROOT)/obj/$(PM2_ARCH_SYS)
MAD_INC		:=	$(MAD2_ROOT)/include
MAD_CI_INC	:=	$(MAD_INC)/connection_interfaces
MAD_CI_SRC	:=	$(MAD_SRC)/connection_interfaces
MAD_LIBD	=	$(MAD2_ROOT)/lib/$(PM2_ARCH_SYS)


ifneq ($(wildcard $(MAD_DEP)),$(MAD_DEP))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAD_DEP))
endif

ifneq ($(wildcard $(MAD_OBJ)),$(MAD_OBJ))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAD_OBJ))
endif

ifneq ($(wildcard $(MAD_LIBD)),$(MAD_LIBD))
DUMMY_BUILD	:=	$(shell mkdir -p $(MAD_LIBD))
endif

MAD_NET_CFLAGS	=

ifdef PM2_FLAGS
MAD2_SUB_MAKEFILES := $(MAD2_ROOT)/make/custom/options.mak
else 
MAD2_SUB_MAKEFILES := $(foreach SUB_MAKEFILE, $(MAD2_PROTOCOLS), $(MAD2_ROOT)/make/$(SUB_MAKEFILE)/options.mak)
endif
include $(MAD2_SUB_MAKEFILES)

MAD_MAKEFILE	+=	$(MAD2_SUB_MAKEFILES)

MAD_REG_SOURCES	:=	$(wildcard $(MAD_SRC)/*.c)
# MAD_NET_SOURCES is defined in ./make/custom/options.mak

MAD_REG_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(MAD_SRC),$(MAD_OBJ),$(MAD_REG_SOURCES)))
MAD_NET_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(MAD_CI_SRC),$(MAD_OBJ),$(MAD_NET_SOURCES)))
MAD_OBJECTS	:=	$(MAD_REG_OBJECTS) $(MAD_NET_OBJECTS)

MAD_LIB		=	$(MAD_LIBD)/libmad2.a
MAD_LLIB	=	$(NET_LLIBS) $(ARCHDLIB)

MAD_CC		=	gcc

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
$(MAD_DEPENDS): $(PM2_MAKEFILE)

.PHONY: mad_default
mad_default: tbx_default ntbx_default $(MAD_LIB) 

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

ifdef PM2_FLAGS
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
clean:
	@ echo "Cleaning Madeleine II"
	@ rm -f $(wildcard $(MAD_LIBD)/*.a $(MAD_OBJ)/*.o $(MAD_DEP)/*.d) \

distclean:
	rm -rf $(wildcard $(MAD2_ROOT)/lib \
		$(MAD2_ROOT)/obj \
		$(MAD2_ROOT)/source/depend)
endif

