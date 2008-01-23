#include "NMadState_ops.h"

//extern void updateList(SWFMovie *movie, nmad_packet_list_t list,  
//                SWFpacketlist *listrepr, unsigned int nframes);


/*
 * A tool to find a packet in a packet list according to its wrapper pointer
 */

static nmad_packet_t find_packet_in_list(uintptr_t wrapper, nmad_packet_list_t list)
{
	nmad_packet_itor_t i;

	/* scan the whole list for the (first) element that matches */
	for (i  = nmad_packet_list_begin(list);
	     i != nmad_packet_list_end(list);
	     i  = nmad_packet_list_next(i))
	{
		if (i->wrapper == wrapper)
			return i;
	}

	/* no such packet was found */
	return NULL;
}

void create_packet(nmad_state_t *nmad_status,
		   uintptr_t wrapper,
		   unsigned long length,
		   unsigned tag,
		   unsigned seq,
		   unsigned is_a_control_packet)
{
	nmad_packet_t new_packet;

	new_packet = nmad_packet_new();

	/* fill up the packet fields */
	new_packet->wrapper = wrapper;
	new_packet->length  = length;
	new_packet->tag     = tag;
	new_packet->seq     = seq;
	new_packet->is_a_control_packet
			    = is_a_control_packet;

	/* insert that packet into the list of new packets */
	nmad_packet_list_push_front(nmad_status->newpackets, new_packet);

	/* create the flash representation of that packet */
	initializePacket(nmad_status->movie, new_packet);
	updateList(nmad_status->movie, nmad_status->newpackets,
	                   &nmad_status->newpackets_repr, NFRAMESPERMOVE);
}

static nmad_packet_t create_volatile_packet(nmad_state_t *nmad_status,
		   uintptr_t wrapper,
		   unsigned long length,
		   unsigned tag,
		   unsigned seq,
		   unsigned is_a_control_packet)
{
	nmad_packet_t new_packet;

	new_packet = nmad_packet_new();

	/* fill up the packet fields */
	new_packet->wrapper = wrapper;
	new_packet->length  = length;
	new_packet->tag     = tag;
	new_packet->seq     = seq;
	new_packet->is_a_control_packet
			    = is_a_control_packet;

	return new_packet;
}

void insert_packet(nmad_state_t *nmad_status,
	           uintptr_t wrapper,
		   unsigned gateid, 
		   unsigned inputlist)
{	
	nmad_packet_t packet;

	/* we first identify the corresponding packet */
	packet = find_packet_in_list(wrapper, nmad_status->newpackets);

	if (!packet) {
		fprintf(stderr, "insert packet failed : could not find packet %p\n", (void *)wrapper);
		return;
	}

	/* it is removed from the new packet list */
	nmad_packet_list_erase(nmad_status->newpackets, packet); 

	/* it is put in the corresponding gate input list */
	nmad_packet_list_push_front(nmad_status->gates[gateid].inputlists[inputlist], packet);

	updateList(nmad_status->movie, nmad_status->gates[gateid].inputlists[inputlist],
  		   &nmad_status->gates[gateid].inputlists_repr[inputlist], NFRAMESPERMOVE);
	updateList(nmad_status->movie, nmad_status->newpackets,
  		   &nmad_status->newpackets_repr, NFRAMESPERMOVE);
}

void in_to_out_ops(nmad_state_t *nmad_status,
		   uintptr_t wrapper,
		   unsigned gateid,
		   unsigned inputlist,
		   unsigned outputlist) 
{
	nmad_packet_t packet;
	nmad_packet_list_t outlist;

	/* we first identify the corresponding packet */
	packet = find_packet_in_list(wrapper, nmad_status->gates[gateid].inputlists[inputlist]);

	if (!packet) {
		fprintf(stderr, "in_to_out_ops failed : could not find packet %p\n", (void *)wrapper);
		return;
	}

	/* take the packet from the input to the output list */
	nmad_packet_list_erase(nmad_status->gates[gateid].inputlists[inputlist], packet);

	/* it may be a control packet ... then put it at the back of the list,
	 * else at the front */
	outlist = nmad_status->gates[gateid].outputlists[outputlist];
	if (packet->is_a_control_packet) {
		nmad_packet_list_push_back(outlist, packet);
	} else {
		nmad_packet_list_push_front(outlist, packet);
	}

	/* update the flash representation */
	updateList(nmad_status->movie, outlist,
		   &nmad_status->gates[gateid].outputlists_repr[outputlist], NFRAMESPERMOVE);
	updateList(nmad_status->movie, nmad_status->gates[gateid].inputlists[inputlist],
  		   &nmad_status->gates[gateid].inputlists_repr[inputlist], NFRAMESPERMOVE);
	
}

void in_to_out_ops_agreg(nmad_state_t *nmad_status,
			 uintptr_t wrapper_in,
		         uintptr_t wrapper_out,
		         unsigned gateid,
		         unsigned inputlist,
		         unsigned outputlist) 
{
	nmad_packet_t packet_in, packet_out;

	/* we first identify the corresponding packets */
	packet_in = find_packet_in_list(wrapper_in,
			nmad_status->gates[gateid].inputlists[inputlist]);
	packet_out = find_packet_in_list(wrapper_out,
			nmad_status->gates[gateid].outputlists[outputlist]);

	if (!packet_in || !packet_out) {
		fprintf(stderr, "in_to_out agreg failed : packet_in = %p packet_out %p\n", (void *)packet_in, (void *)packet_out);
		return;
	}

	/* take the packet from the input to the output list */
	nmad_packet_list_erase(nmad_status->gates[gateid].inputlists[inputlist], packet_in);

	/* call the flash animation to aggreg : merge packet_in with packet_out */
	agregPacket(nmad_status->movie, packet_out , packet_in, NFRAMESPERMOVE);

}

void in_to_out_ops_split(nmad_state_t *nmad_status,
			 uintptr_t wrapper,
		         uintptr_t new_wrapper,
		         unsigned gateid,
		         unsigned outputlist,
			 unsigned long length_new) 
{
	nmad_packet_t packet, new_packet;
	nmad_packet_list_t outlist;

	/* we first identify the corresponding packets */
	packet = find_packet_in_list(wrapper,
			nmad_status->gates[gateid].outputlists[outputlist]);

	if (!packet) {
		fprintf(stderr, "in_to_out split : could not find packet %p\n", (void *)wrapper);
		return;
	}

	/* we created a new packet ! */
	new_packet = create_volatile_packet(nmad_status,
                   		new_wrapper, length_new,
                   	        packet->tag, packet->seq,
                   		packet->is_a_control_packet);

	packet->length -= length_new; 


	/* put the new packet into the output list */
	outlist = nmad_status->gates[gateid].outputlists[outputlist];

	/* that's a little dirty since we must put both at the end ... */
	nmad_packet_list_erase(outlist, packet);
	nmad_packet_list_push_back(outlist, packet);
	nmad_packet_list_push_back(outlist, new_packet);

	/* call the flash animation to split */
	splitPacket(nmad_status->movie, packet, new_packet, NFRAMESPERMOVE);
	updateList(nmad_status->movie, outlist,
		   &nmad_status->gates[gateid].outputlists_repr[outputlist], NFRAMESPERMOVE);
	
}

void gate_to_track_ops(nmad_state_t *nmad_status,
			uintptr_t wrapper,
			unsigned gateid,
			unsigned driverid,
			unsigned trackid)
{
	nmad_packet_t packet;

	/* search for the packet into each of the output lists */
	int out;
	for (out = 0; out < nmad_status->gates[gateid].noutputlists; out++) 
	{
		packet = find_packet_in_list(wrapper, nmad_status->gates[gateid].outputlists[out]);
		if (packet) break;
	}

	if (!packet) {
		fprintf(stderr, "gate_to_track_ops could not find packet %p\n", (void *)wrapper);
		return;
	}
 
	/* refresh nmad_status->gates[gateid].outputlists[out] */

	/* take the packet to the corresponding track */
	nmad_packet_list_erase(nmad_status->gates[gateid].outputlists[out], packet);
	nmad_packet_list_push_front(nmad_status->drivers[driverid].pre_tracks[trackid], packet);

	updateList(nmad_status->movie, nmad_status->drivers[driverid].pre_tracks[trackid],
  		   &nmad_status->drivers[driverid].pre_tracks_repr[trackid], NFRAMESPERMOVE);
	updateList(nmad_status->movie, nmad_status->gates[gateid].outputlists[out],
		   &nmad_status->gates[gateid].outputlists_repr[out], NFRAMESPERMOVE);
	
}

void track_to_driver_ops(nmad_state_t *nmad_status,
			  uintptr_t wrapper,
			  unsigned driverid,
			  unsigned trackid)
{
	nmad_packet_t packet;

	packet = find_packet_in_list(wrapper, nmad_status->drivers[driverid].pre_tracks[trackid]);
	
	if (!packet) {
		fprintf(stderr, "track_to_driver could not find packet %p\n", (void *)wrapper);
		return;
	}


	nmad_packet_list_erase(nmad_status->drivers[driverid].pre_tracks[trackid], packet);
	nmad_packet_list_push_front(nmad_status->drivers[driverid].post_tracks[trackid], packet);

	updateList(nmad_status->movie, nmad_status->drivers[driverid].post_tracks[trackid],
  		   &nmad_status->drivers[driverid].post_tracks_repr[trackid], NFRAMESPERMOVE);
	updateList(nmad_status->movie, nmad_status->drivers[driverid].pre_tracks[trackid],
  		   &nmad_status->drivers[driverid].pre_tracks_repr[trackid], NFRAMESPERMOVE);

}

void send_packet(nmad_state_t *nmad_status,
		 uintptr_t wrapper,
		 unsigned driverid,
		 unsigned trackid)
{
	nmad_packet_t packet;

	packet = find_packet_in_list(wrapper, nmad_status->drivers[driverid].post_tracks[trackid]);

	if (!packet) {
		fprintf(stderr, "send packet could not find packet %p\n", (void *)wrapper);
		return;
	}

	nmad_packet_list_erase(nmad_status->drivers[driverid].post_tracks[trackid], packet);
	
	/* tell flash to destroy that packet */
	destroyPacket(nmad_status->movie, packet);
	updateList(nmad_status->movie, nmad_status->drivers[driverid].post_tracks[trackid],
  		   &nmad_status->drivers[driverid].post_tracks_repr[trackid], NFRAMESPERMOVE);

}

void acknowledge_packet(nmad_state_t *nmad_status,
			uintptr_t wrapper,
			unsigned gateid,
			unsigned outputlist)
{
	nmad_packet_t packet;

        /* we first identify the corresponding packet */
        packet = find_packet_in_list(wrapper, nmad_status->gates[gateid].outputlists[outputlist]);

	if (!packet) {
		fprintf(stderr, "acknoledge packet  could not find packet %p\n", (void *)wrapper);
		return;
	}

	packet->acked = 1;

	ackPacket(nmad_status->movie, packet);
}

