.PHONY: default all
default: all

SRC_DIR :=      .

include ../../Makefile

PM2_APP_CFLAGS  =
PM2_APP_LDFLAGS =
PM2_APP_LLIBS   =

all:    $(BIN_DIR)/attach $(BIN_DIR)/slots

$(BIN_DIR)/slots: $(OBJ_DIR)/rpc_defs.o
$(BIN_DIR)/attach: $(OBJ_DIR)/rpc_defs.o