#include<ming.h>
#include "../NMadState/nmad_state.h"
#include "Anim.h"

void preProcess(nmad_state_t *status);

void updateList(SWFMovie movie, nmad_packet_list_t list,  
		SWFpacketlist *listrepr, unsigned int nframes);

void initializePacket(SWFMovie movie, nmad_packet_t packet);

void destroyPacket(SWFMovie movie, nmad_packet_t packet);

void splitPacket(SWFMovie movie, nmad_packet_t packet, nmad_packet_t new_packet, int nframes);

void ackPacket(SWFMovie movie, nmad_packet_t packet);

void agregPacket(SWFMovie movie, nmad_packet_t host_packet, nmad_packet_t packet, int nframes);

