#include <stdlib.h>
#include <ming.h>
#include <nmad_state.h>
#include "DrawShape.h"

void flash_engine_init(nmad_state_t *nmad_status);

void flash_engine_generate_output(nmad_state_t *nmad_status, char *path);

void addTimeControl(SWFMovie* movie,int x,int y,int width,int height);

void initialise_machine_newlist(nmad_state_t *nmad_status, int xpos, int ypos);

void initialise_gates(nmad_state_t *nmad_status,int xpos,int ypos);

void initialise_drivers(nmad_state_t *nmad_status,int xpos,int ypos);
