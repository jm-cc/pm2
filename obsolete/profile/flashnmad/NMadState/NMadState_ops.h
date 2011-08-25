#include <stdio.h>
#include "nmad_state.h"
#include "Interface.h"


void create_packet(nmad_state_t *nmad_status,
		   uintptr_t wrapper,
		   unsigned long length,
		   unsigned tag,
		   unsigned seq,
		   unsigned is_a_control_packet);

void insert_packet(nmad_state_t *nmad_status,
	           uintptr_t wrapper,
		   unsigned gateid, 
		   unsigned inputlist);

void in_to_out_ops(nmad_state_t *nmad_status,
		   uintptr_t wrapper,
		   unsigned gateid,
		   unsigned inputlist,
		   unsigned outputlist); 

void in_to_out_ops_agreg(nmad_state_t *nmad_status,
			 uintptr_t wrapper_in,
		         uintptr_t wrapper_out,
		         unsigned gateid,
		         unsigned inputlist,
		         unsigned outputlist);

void in_to_out_ops_split(nmad_state_t *nmad_status,
			 uintptr_t wrapper,
		         uintptr_t new_wrapper,
		         unsigned gateid,
		         unsigned outputlist,
			 unsigned long length_new);

void gate_to_track_ops(nmad_state_t *nmad_status,
			uintptr_t wrapper,
			unsigned gateid,
			unsigned driverid,
			unsigned trackid);

void track_to_driver_ops(nmad_state_t *nmad_status,
			  uintptr_t wrapper,
			  unsigned driverid,
			  unsigned trackid);

void send_packet(nmad_state_t *nmad_status,
		 uintptr_t wrapper,
		 unsigned driverid,
		 unsigned trackid);

void acknowledge_packet(nmad_state_t *nmad_status,
			uintptr_t wrapper,
			unsigned gateid,
			unsigned outputlist);

