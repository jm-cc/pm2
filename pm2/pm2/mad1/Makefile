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
#
# The expansion of the macro MAD_OPTIONS may include the following
# definitions :
#
#	-DMAD_TIMING
#		Sets the "trace mode" on.
#
#	-DUSE_SAFE_MALLOC
#		Uses a small "safe malloc" library that trys to
#		verify bad malloc/free usage.
#

MAD_OPTIONS		=	

MAD_STANDALONE_OPTIONS	=	-Wall -O6 # -DMAD_TIMING -DUSE_SAFE_MALLOC

################### END OF CUSTOMIZATION SECTION #################

ifndef MAD1_ROOT
MAD1_ROOT	=	$(PM2_ROOT)/mad1
endif

ifndef PM2_ARCH_SYS
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

include $(MAD1_ROOT)/make/custom/options.mak

include $(MAD1_ROOT)/make/archdep/$(shell basename $(PM2_SYS) _SYS).inc

MAD_MAKEFILE	=	$(MAD1_ROOT)/Makefile \
			$(MAD1_ROOT)/make/custom/options.mak

ifdef PM2_FLAGS
MAD_MAKEFILE	+=	$(MAD1_ROOT)/.mad_pm2
else
MAD_MAKEFILE	+=	$(MAD1_ROOT)/.mad_standalone
endif

MAD_INC		:=	$(MAD1_ROOT)/include
MAD_SRC		:=	$(MAD1_ROOT)/source
MAD_DEP		:=	$(MAD_SRC)/depend
MAD_OBJ		:=	$(MAD_SRC)/obj/$(PM2_ARCH_SYS)
MAD_LIBD	:=	$(MAD1_ROOT)/lib/$(PM2_ARCH_SYS)

ifneq ($(MAKECMDGOALS),distclean) # avoid auto-building of directories
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

MAD_REG_SOURCES	:=	$(wildcard $(MAD_SRC)/*.c)
MAD_NET_SOURCES	:=	$(wildcard $(MAD_SRC)/$(NET_INTERF)/*.c)

MAD_REG_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(MAD_SRC),$(MAD_OBJ),$(MAD_REG_SOURCES)))
MAD_NET_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(MAD_SRC)/$(NET_INTERF),$(MAD_OBJ),$(MAD_NET_SOURCES)))
MAD_OBJECTS	:=	$(MAD_REG_OBJECTS) $(MAD_NET_OBJECTS)

MAD_LIB		=	$(MAD_LIBD)/libmad.a
MAD_LLIB	=	$(NET_LLIBS) $(ARCHDLIB)

MAD_CC		=	gcc

MAD_CFLAGS	=	$(MAD_OPTIONS) -I$(MAD_INC)
MAD_FLAGS	=	-DNET_ARCH=\"$(NET_INTERF)\" $(NET_INIT) \
			$(NET_CFLAGS)

ifdef PM2_FLAGS
MAD_CFLAGS	+=	$(PM2_FLAGS)
MAD_FLAGS	+=	-I$(MAR_INC)
else
MAD_CFLAGS	+=	-fomit-frame-pointer $(MAD_STANDALONE_OPTIONS)
endif

MAD_LDFLAGS	=	$(NET_LFLAGS)

MAD_REG_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(MAD_OBJ),$(MAD_DEP),$(MAD_REG_OBJECTS)))
MAD_NET_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(MAD_OBJ),$(MAD_DEP),$(MAD_NET_OBJECTS)))
MAD_DEPENDS	:=	($strip $(MAD_REG_DEPENDS) $(MAD_NET_DEPENDS))

MAD_DEP_TO_OBJ	=	$(MAD_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

.PHONY: mad_default
mad_default: $(MAD_LIB)

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(MAD_DEPENDS)),$(MAD_DEPENDS))
include $(MAD_DEPENDS)
endif
endif

ifdef PM2_FLAGS
THE_MAKEFILES	=	$(PM2_MAKEFILE)
else
THE_MAKEFILES	=	$(MAD_MAKEFILE)
endif


$(MAD_OBJECTS): $(MAD_OBJ)/%.o: $(MAD_DEP)/%.d
$(MAD_DEPENDS): $(THE_MAKEFILES)

$(MAD_LIB): $(MAD_OBJECTS)
ifdef PM2_FLAGS
	rm -f $(MAD1_ROOT)/.mad_standalone
else
	rm -f $(MAD1_ROOT)/.mad_pm2
endif
	rm -f $(MAD_LIB)
	ar cr $(MAD_LIB) $(MAD_OBJECTS)

$(MAD1_ROOT)/.mad_pm2:
	cp /dev/null $(MAD1_ROOT)/.mad_pm2

$(MAD1_ROOT)/.mad_standalone:
	cp /dev/null $(MAD1_ROOT)/.mad_standalone

$(MAD_REG_OBJECTS): $(MAD_OBJ)/%.o: $(MAD_SRC)/%.c
	$(MAD_CC) $(MAD_CFLAGS) $(MAD_FLAGS) -c $< -o $@

$(MAD_REG_DEPENDS): $(MAD_DEP)/%.d: $(MAD_SRC)/%.c
	$(SHELL) -ec '$(MAD_CC) -MM $(MAD_CFLAGS) $(MAD_FLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(MAD_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(MAD_NET_OBJECTS): $(MAD_OBJ)/%.o: $(MAD_SRC)/$(NET_INTERF)/%.c
	$(MAD_CC) $(MAD_CFLAGS) $(MAD_FLAGS) -c $< -o $@

$(MAD_NET_DEPENDS): $(MAD_DEP)/%.d: $(MAD_SRC)/$(NET_INTERF)/%.c
	$(SHELL) -ec '$(MAD_CC) -MM $(MAD_CFLAGS) $(MAD_FLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(MAD_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


ifdef PM2_FLAGS
.PHONY: madclean
madclean:
	rm -f $(wildcard $(MAD_LIBD)/*.a $(MAD_OBJ)/*.o $(MAD_DEP)/*.d \
		$(MAD1_ROOT)/examples/depend/*.d \
		$(MAD1_ROOT)/examples/obj/$(PM2_ARCH_SYS)/*.o \
		$(MAD1_ROOT)/examples/bin/$(PM2_ARCH_SYS)/* \
		$(MAD1_ROOT)/.mad_*)

maddistclean:
	rm -rf $(wildcard $(MAD1_ROOT)/lib \
		$(MAD1_ROOT)/source/obj \
		$(MAD1_ROOT)/source/depend \
		$(MAD1_ROOT)/examples/depend \
		$(MAD1_ROOT)/examples/obj \
		$(MAD1_ROOT)/examples/bin \
		$(MAD1_ROOT)/.mad_*)
else
.PHONY: clean distclean
clean:
	rm -f $(wildcard $(MAD_LIBD)/*.a $(MAD_OBJ)/*.o $(MAD_DEP)/*.d \
		$(MAD1_ROOT)/examples/depend/*.d \
		$(MAD1_ROOT)/examples/obj/$(PM2_ARCH_SYS)/*.o \
		$(MAD1_ROOT)/examples/bin/$(PM2_ARCH_SYS)/* \
		$(MAD1_ROOT)/.mad_*)

distclean:
	rm -rf $(wildcard $(MAD1_ROOT)/lib \
		$(MAD1_ROOT)/source/obj \
		$(MAD1_ROOT)/source/depend \
		$(MAD1_ROOT)/examples/depend \
		$(MAD1_ROOT)/examples/obj \
		$(MAD1_ROOT)/examples/bin \
		$(MAD1_ROOT)/.mad_*)
endif

######################## Applications ########################

ifdef MAD_EX_DIR

ifeq ($(MAD_EX_DIR),.)
SRC_DIR		:=	./
else
SRC_DIR		:=	$(MAD_EX_DIR)
endif

THIS_MAKEFILE	:=	$(SRC_DIR)/Makefile

DEP_DIR	=	$(SRC_DIR)/depend
OBJ_DIR	=	$(SRC_DIR)/obj/$(PM2_ARCH_SYS)
BIN_DIR	=	$(SRC_DIR)/bin/$(PM2_ARCH_SYS)

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
	$(MAD_CC) $(MAD_CFLAGS) -c $< -o $@

$(DEPENDS): $(DEP_DIR)/%.d: $(SRC_DIR)/%.c
	$(SHELL) -ec '$(MAD_CC) -MM $(MAD_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(MAD_LIB)
	$(MAD_CC) $(MAD_CFLAGS) $(MAD_LDFLAGS) $^ -o $@ $(MAD_LLIB)

%: $(BIN_DIR)/% ;

endif