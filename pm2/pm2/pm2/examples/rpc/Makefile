
export FLAVOR_DEFAULT:=pm2

PROGS := ping simple

include $(PM2_ROOT)/make/apps.mak

$(APP_BIN)/simple$(APP_EXT): $(APP_OBJ)/rpc_defs$(APP_EXT).o
$(APP_BIN)/ping$(APP_EXT): $(APP_OBJ)/rpc_defs$(APP_EXT).o

