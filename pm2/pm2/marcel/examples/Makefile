
export FLAVOR_DEFAULT:=marcel

PROGS := active	cleanup	contsw create daube delay derout eclone force_raise \
	kbd keys matrix once poll raise sample stack_jump sumtime timed \
	timeout tsp tube usersched userspace gthreads

include $(PM2_ROOT)/make/apps.mak


ifdef APP_RECURSIF


$(APP_BIN)/eclone$(APP_EXT): $(APP_OBJ)/clone$(APP_EXT).o

$(APP_OBJ)/eclone$(APP_EXT).o: $(SRC_DIR)/eclone.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -O0 -c $< -o $@

$(APP_BIN)/tsp$(APP_EXT): $(APP_OBJ)/tsp-main$(APP_EXT).o $(APP_OBJ)/tsp-job$(APP_EXT).o -lm

$(APP_OBJ)/tsp$(APP_EXT).o: $(SRC_DIR)/tsp.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -DMT -c $< -o $@

$(APP_OBJ)/tsp-main$(APP_EXT).o: $(SRC_DIR)/tsp-main.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -DMT -c $< -o $@

$(APP_OBJ)/tsp-job$(APP_EXT).o: $(SRC_DIR)/tsp-job.c
	$(COMMON_PREFIX) $(CC) $(CFLAGS) -DMT -c $< -o $@

endif

