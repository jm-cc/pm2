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
# The expansion of the macro OPTIONS may include the following
# definitions :
#
#	-DMINIMAL_PREEMPTION
#		Sets up a 'minimal preemption' mechanism that will
#		only give hand (from time to time) to a special
#		'message-receiver' thread.
#		This option *must* be set as a GLOBAL_OPTION.
#
#	-DPM2_TIMING
#		Sets the "profiling mode" on.
#
#	-DMIGRATE_IN_HEADER
#		Private Flag. Should *not* be used in regular
#		applications.
#
#	-DUSE_SAFE_MALLOC
#		Uses a small "safe malloc" library that trys to
#		verify bad malloc/free usage.
#
# And also traditional gcc compiling options such as -g, -p, -DNDEBUG,
# etc.
#
# The options set in `GLOBAL_OPTIONS' will also be used by MARCEL and
# MADELEINE.
#

GLOBAL_OPTIONS	=	-Wall -O6 #-DMAR_TRACE #-DSTANDARD_MAIN -g -DUSE_SAFE_MALLOC

SMP		=	no
TOOLBOX         =       no
NETTOOLBOX      =       no
DSM		=	yes

OPTIONS		=	#-DMIGRATE_IN_HEADER # -DPM2_TIMING


################### END OF CUSTOMIZATION SECTION #################

ifndef MARCEL_ROOT
MARCEL_ROOT	:=	$(PM2_ROOT)/marcel
endif

ifndef MAD1_ROOT
MAD1_ROOT	:=	$(PM2_ROOT)/mad1
endif

ifndef MAD2_ROOT
MAD2_ROOT	:=	$(PM2_ROOT)/mad2
endif

ifndef DSM_ROOT
DSM_ROOT	:=	$(PM2_ROOT)/dsm
endif

ifndef PM2_ARCH_SYS
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

PM2_ARCH_SYM	:=	$(subst /,_,$(PM2_ARCH_SYS))

PM2_FLAGS	=	-DPM2=\"$(PM2_ARCH_SYM)\" -D$(PM2_SYS) -D$(PM2_ARCH) $(GLOBAL_OPTIONS)

ifeq ($(SMP),yes)
PM2_FLAGS += -DSMP -D_REENTRANT
endif

ifeq ($(DSM),yes)
PM2_FLAGS += -DDSM
endif

include $(PM2_ROOT)/make/mad.mak

ifeq ($(MAD2),yes)
TOOLBOX    = yes
NETTOOLBOX = yes
PM2_FLAGS += -DMAD2
else
PM2_FLAGS += -DMAD1
endif

ifeq ($(TOOLBOX),yes)
PM2_FLAGS += -DTBX
endif

ifeq ($(NETTOOLBOX),yes)
PM2_FLAGS += -DNTBX -DNTBX_TCP
endif


PM2_SRC		:=	$(PM2_ROOT)/source
PM2_DEP		:=	$(PM2_SRC)/depend
PM2_OBJ		:=	$(PM2_SRC)/obj/$(PM2_ARCH_SYS)
PM2_INC		:=	$(PM2_ROOT)/include
PM2_LIBD	:=	$(PM2_ROOT)/lib/$(PM2_ARCH_SYS)

ifneq ($(MAKECMDGOALS),distclean) # avoid auto-building of directories
ifneq ($(wildcard $(PM2_DEP)),$(PM2_DEP))
DUMMY_BUILD	:=	$(shell mkdir -p $(PM2_DEP))
endif

ifneq ($(wildcard $(PM2_OBJ)),$(PM2_OBJ))
DUMMY_BUILD	:=	$(shell mkdir -p $(PM2_OBJ))
endif

ifneq ($(wildcard $(PM2_LIBD)),$(PM2_LIBD))
DUMMY_BUILD	:=	$(shell mkdir -p $(PM2_LIBD))
endif
endif

PM2_LIB		=	$(PM2_LIBD)/libpm2.a
PM2_LLIB	=	

.PHONY: default
ifeq ($(TOOLBOX),yes)
CLEAN_DEP       =       marclean madclean dsmclean tbxclean ntbxclean
DISTCLEAN_DEP   =       mardistclean maddistclean dsmdistclean tbxdistclean ntbxdistclean
default: pm2_default mad_default mar_default dsm_default tbx_default ntbx_default
else
CLEAN_DEP       =       marclean madclean dsmclean
DISTCLEAN_DEP   =       mardistclean maddistclean dsmdistclean
default: pm2_default mad_default mar_default dsm_default
endif

PM2_MAKEFILE	=	$(PM2_ROOT)/Makefile \
			$(PM2_ROOT)/make/mad.mak \
			$(MAR_MAKEFILE) \
			$(MAD_MAKEFILE) \
			$(DSM_MAKEFILE) \
                        $(TBX_MAKEFILE) \
                        $(NTBX_MAKEFILE)

include $(MARCEL_ROOT)/Makefile

ifeq ($(MAD2),yes)
 include $(MAD2_ROOT)/Makefile
else
 include $(MAD1_ROOT)/Makefile
endif

ifeq ($(DSM),yes)
include $(DSM_ROOT)/Makefile
endif

ifeq ($(TOOLBOX),yes)
include $(PM2_ROOT)/toolbox/Makefile
endif

ifeq ($(NETTOOLBOX),yes)
include $(PM2_ROOT)/toolbox/net/Makefile
endif

PM2_CC		=	gcc

PM2_AS		:=	$(PM2_CC)
PM2_ASFLAGS	=	-D$(PM2_ARCH) -D$(PM2_SYS)

PM2_CFLAGS	=	-fomit-frame-pointer $(PM2_FLAGS) \
			-I$(PM2_INC) -I$(MAD_INC) -I$(MAR_INC) 
PM2_LDFLAGS  	=	$(MAD_LDFLAGS) $(MAR_LDFLAGS)

ifeq ($(TOOLBOX),yes)
PM2_CFLAGS	+=	-I$(TBX_INC)
endif

ifeq ($(NETTOOLBOX),yes)
PM2_CFLAGS	+=	-I$(NTBX_INC)
endif

ifeq ($(DSM),yes)
PM2_CFLAGS	+=	-I$(DSM_INC)
PM2_LFLAGS	+=	$(DSM_LFLAGS)
endif

ifeq ($(MAD2),yes)
OPTIONS		+=	$(NET_CFLAGS)
endif

PM2_LIBS	:=	$(PM2_LIB) $(MAD_LIB) $(MAR_LIB) $(DSM_LIB) $(TBX_LIB) $(NTBX_LIB)
PM2_LLIBS	:=	$(PM2_LLIB) $(MAD_LLIB) $(MAR_LLIB) $(DSM_LLIB) $(TBX_LLIB) $(NTBX_LLIB)


PM2_C_SOURCES	:=	$(wildcard $(PM2_SRC)/*.c)
PM2_S_SOURCES	:=	$(wildcard $(PM2_SRC)/*.S)

PM2_C_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(PM2_SRC),$(PM2_OBJ),$(PM2_C_SOURCES)))
PM2_S_OBJECTS	:=	$(patsubst %.S,%.o,$(subst $(PM2_SRC),$(PM2_OBJ),$(PM2_S_SOURCES)))
PM2_OBJECTS	:=	$(PM2_C_OBJECTS) $(PM2_S_OBJECTS)

PM2_C_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(PM2_OBJ),$(PM2_DEP),$(PM2_C_OBJECTS)))
PM2_S_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(PM2_OBJ),$(PM2_DEP),$(PM2_S_OBJECTS)))
PM2_DEPENDS	:=	$(strip $(PM2_C_DEPENDS) $(PM2_S_DEPENDS))

PM2_DEP_TO_OBJ	=	$(PM2_OBJ)/$(patsubst %.d,%.o,$(notdir $@))
PM2_OBJ_TO_S	=	$(PM2_SRC)/$(patsubst %.o,%.s,$(notdir $@))

.PHONY: pm2_default
pm2_default: $(PM2_LIB)

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(PM2_DEPENDS)),$(PM2_DEPENDS))
include $(PM2_DEPENDS)
endif
endif

$(PM2_OBJECTS): $(PM2_OBJ)/%.o: $(PM2_DEP)/%.d $(PM2_MAKEFILE)
$(PM2_DEPENDS): $(PM2_MAKEFILE)


$(PM2_LIB): $(PM2_OBJECTS)
	rm -f $(PM2_LIB)
	ar cr $(PM2_LIB) $(PM2_OBJECTS)

$(PM2_C_OBJECTS): $(PM2_OBJ)/%.o: $(PM2_SRC)/%.c
	$(PM2_CC) $(PM2_CFLAGS) $(OPTIONS) -c $< -o $@

$(PM2_C_DEPENDS): $(PM2_DEP)/%.d: $(PM2_SRC)/%.c
	$(SHELL) -ec '$(PM2_CC) -MM $(PM2_CFLAGS) $(OPTIONS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PM2_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(PM2_S_OBJECTS): $(PM2_OBJ)/%.o: $(PM2_SRC)/%.S
	$(PM2_CC) -E -P $(PM2_ASFLAGS) $< > $(PM2_OBJ_TO_S)
	$(PM2_AS) $(PM2_ASFLAGS) -c $(PM2_OBJ_TO_S) -o $@
	rm -f $(PM2_OBJ_TO_S)

$(PM2_S_DEPENDS): $(PM2_DEP)/%.d: $(PM2_SRC)/%.S
	$(SHELL) -ec '$(PM2_CC) -MM $(PM2_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(PM2_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


.PHONY: clean distclean
clean: $(CLEAN_DEP)
	rm -f $(wildcard $(PM2_LIBD)/*.a $(PM2_OBJ)/*.o $(PM2_DEP)/*.d \
		$(PM2_ROOT)/examples/*/depend/*.d \
		$(PM2_ROOT)/examples/*/obj/$(PM2_ARCH_SYS)/*.o \
		$(PM2_ROOT)/examples/*/bin/$(PM2_ARCH_SYS)/* \
		$(PM2_ROOT)/console/bin/$(PM2_ARCH_SYS)/* \
		$(PM2_ROOT)/console/text/obj/$(PM2_ARCH_SYS)/*.o \
		$(PM2_ROOT)/console/graphic/obj/$(PM2_ARCH_SYS)/*.o)

distclean: $(DISTCLEAN_DEP)
	rm -rf $(wildcard $(PM2_ROOT)/lib \
		$(PM2_ROOT)/source/obj \
		$(PM2_ROOT)/source/depend \
		$(PM2_ROOT)/examples/*/depend \
		$(PM2_ROOT)/examples/*/obj \
		$(PM2_ROOT)/examples/*/bin \
		$(PM2_ROOT)/console/bin \
		$(PM2_ROOT)/console/text/obj \
		$(PM2_ROOT)/console/graphic/obj)

######################## Applications ########################

ifdef SRC_DIR

ifeq ($(SRC_DIR),.)
SRC_DIR		:=	./
endif

PM2_MAKEFILE	+=	$(SRC_DIR)/Makefile

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

$(OBJECTS): $(OBJ_DIR)/%.o: $(DEP_DIR)/%.d $(PM2_MAKEFILE)
$(DEPENDS): $(PM2_MAKEFILE)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(PM2_CC) $(PM2_CFLAGS) $(PM2_APP_CFLAGS) -c $< -o $@

$(DEPENDS): $(DEP_DIR)/%.d: $(SRC_DIR)/%.c
	$(SHELL) -ec '$(PM2_CC) -MM $(PM2_CFLAGS) $(PM2_APP_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(PM2_LIBS)
	$(PM2_CC) $(PM2_CFLAGS) $(PM2_LDFLAGS) $(PM2_APP_LDFLAGS) $^ -o $@ $(PM2_LLIBS) $(PM2_APP_LLIBS)

%: $(BIN_DIR)/% ;

endif
