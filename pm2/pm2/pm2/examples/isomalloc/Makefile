
export FLAVOR_DEFAULT:=pm2

PROGS := attach slots

include $(PM2_ROOT)/make/apps.mak

$(APP_BIN)/slots$(APP_EXT): $(APP_OBJ)/rpc_defs$(APP_EXT).o
$(APP_BIN)/attach$(APP_EXT): $(APP_OBJ)/rpc_defs$(APP_EXT).o

