.PHONY: default all
default: all

SRC_DIR     :=  ./
APP_CFLAGS  :=
APP_LDFLAGS :=
APP_LLIBS   :=

include ../../Makefile

all:    $(APP_BIN)/attach\
        $(APP_BIN)/slots

$(APP_BIN)/slots: $(APP_OBJ)/rpc_defs.o
$(APP_BIN)/attach: $(APP_OBJ)/rpc_defs.o