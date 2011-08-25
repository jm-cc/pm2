#ifndef        NMAD_STATE_H_
#define        NMAD_STATE_H_

#include <stdint.h>
#include <stdlib.h>
#include "list.h"
#include "Anim.h"

#define NFRAMESPERMOVE 10

/*
 * These are purely arbitrary values ... 
 */
#define NMAX_DRIVERS	6
#define NMAX_TRACKS	2
#define NMAX_GATES	12
#define MAXURLLENGTH	200

#define NMAX_LISTS	6

/* 
 * These structures aim at describing the overall state of NewMadeleine 
 *
 *   			  |--|	*******      		***|**	
 *	***************   |  | 	                 	***|**
 *			  |  |	*******      
 * 	***************	  |  |	                	***|**
 *			  |--|	*******      		***|**
 *
 *
 *	Input lists	      Output lists     	(pre|post)tracks
 */

LIST_TYPE(nmad_packet, 
	uintptr_t wrapper;
	unsigned long length;
	unsigned is_a_control_packet;
	unsigned acked;
	unsigned tag;
	unsigned seq;
	SWFpacket repr;
	char *name; /* useful ? */
);

typedef struct nmad_gate {
	int ninputlists;
	int noutputlists;
	nmad_packet_list_t inputlists[NMAX_LISTS];
	nmad_packet_list_t outputlists[NMAX_LISTS];
	SWFpacketlist inputlists_repr[NMAX_LISTS];
	SWFpacketlist outputlists_repr[NMAX_LISTS];
} nmad_gate_t;

typedef struct nmad_driver {
	int ntracks_pre;
	int ntracks_post;
	nmad_packet_list_t pre_tracks[NMAX_TRACKS];
	nmad_packet_list_t post_tracks[NMAX_TRACKS];
	SWFpacketlist pre_tracks_repr[NMAX_TRACKS];
	SWFpacketlist post_tracks_repr[NMAX_TRACKS];
	char driverurl[MAXURLLENGTH];
	SWFnic repr;
} nmad_driver_t;



typedef struct nmad_state {
	int ngates;
	int ndrivers;
	nmad_gate_t gates[NMAX_GATES];
	nmad_driver_t drivers[NMAX_DRIVERS];
	nmad_packet_list_t newpackets; /* for brand new packets */
	SWFpacketlist newpackets_repr;
	SWFMovie movie;
} nmad_state_t;

#endif /* NMAD_STATE_H */
