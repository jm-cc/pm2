
LIBRARY=mad1
LIBNAME=mad

#marcel_CC=gcc
#marcel_AS=gcc

include $(PM2_ROOT)/make/libs-vars.mak

ifeq (,$(findstring _$(MAKECMDGOALS)_,_clean_ $(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/mad1-specific.mak
endif

LIB_C_SOURCES_ADDED := $(LIB_SRC)/$(LIB_PROT)/netinterf.c
LIB_C_OBJECTS_ADDED := $(LIB_GEN_OBJ)/netinterf$(LIB_EXT).o
LIB_C_PICS_ADDED := $(LIB_GEN_OBJ)/netinterf$(LIB_EXT).pic
LIB_C_DEPENDS_ADDED := $(LIB_GEN_DEP)/netinterf$(LIB_EXT).d
LIB_OBJECTS += $(LIB_C_OBJECTS_ADDED)
LIB_PICS += $(LIB_C_PICS_ADDED)

include $(PM2_ROOT)/make/libs-rules.mak

$(LIB_C_OBJECTS_ADDED): $(LIB_GEN_OBJ)/%$(LIB_EXT).o: $(LIB_SRC)/$(LIB_PROT)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -c $< -o $@

$(LIB_C_PICS_ADDED): $(LIB_GEN_OBJ)/%$(LIB_EXT).pic: $(LIB_SRC)/$(LIB_PROT)/%.c
	$(LIB_PREFIX) $(CC) $(CFLAGS) -fPIC -c $< -o $@

$(LIB_C_DEPENDS_ADDED): $(LIB_GEN_DEP)/%$(LIB_EXT).d: $(LIB_SRC)/$(LIB_PROT)/%.c
	$(LIB_PREFIX) $(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(LIB_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(PM2_MAK_DIR)/mad1-specific.mak: $(LIB_STAMP_FLAVOR)
	@_PM2_OUTPUT=`$(PM2_CONFIG) --protocols $(LIBRARY)`;\
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
	echo "LIB_PROT := $$_PM2_OUTPUT" > $@ ;\
	fi;
