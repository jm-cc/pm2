#ifndef TRACEBUFFER
#define TRACEBUFFER

#include "sigmund.h"

#define FUT_SETUP_CODE				0x210
#define FUT_KEYCHANGE_CODE			0x211
#define FUT_RESET_CODE				0x212
#define FUT_CALIBRATE0_CODE			0x220
#define FUT_CALIBRATE1_CODE			0x221
#define FUT_CALIBRATE2_CODE			0x222
#define FUT_SWITCH_TO_CODE			0x230
#define FUT_MAIN_ENTRY_CODE			0x240
#define FUT_MAIN_EXIT_CODE			0x340

void init_trace_file(char *supertrace);

void init();

int get_next_trace(trace *tr);

void close_trace_file(void);

#endif
