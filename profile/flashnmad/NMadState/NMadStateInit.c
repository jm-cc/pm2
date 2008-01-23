#include "NMadStateInit.h"
#include "FlashEngineInit.h"

static void nmad_init_core(nmad_state_t *nmad_status)
{
	nmad_status->newpackets = nmad_packet_list_new();
}

static void nmad_state_init_gate(nmad_state_t *nmad_status, unsigned gateid)
{
	int inlist, outlist;
	nmad_gate_t *gate = &nmad_status->gates[gateid];
	
	for(inlist = 0; inlist < gate->ninputlists ; inlist++) 
	{
		gate->inputlists[inlist] = nmad_packet_list_new();
	}

	for(outlist = 0; outlist < gate->noutputlists ; outlist++) 
	{
		gate->outputlists[outlist] = nmad_packet_list_new();
	}
}

static void nmad_state_init_driver(nmad_state_t *nmad_status, unsigned driverid)
{
	int inlist, outlist;
	nmad_driver_t *driver = &nmad_status->drivers[driverid];
	
	for(inlist = 0; inlist < driver->ntracks_pre ; inlist++) 
	{
		driver->pre_tracks[inlist] = nmad_packet_list_new();
	}

	for(outlist = 0; outlist < driver->ntracks_post ; outlist++) 
	{
		driver->post_tracks[outlist] = nmad_packet_list_new();
	}

}

void nmad_state_init(nmad_state_t *nmad_status)
{
	int gate;
	int driver;

	/* we go through the nmad_status which was partially filled during the 
	 * first parsing of the log for the preprocessing */

	/* init the core itself */
	nmad_init_core(nmad_status);

	/* init gates */
	for (gate = 0; gate < nmad_status->ngates; gate++)
	{
		nmad_state_init_gate(nmad_status, gate);
	}
	
	/* init drivers */
	for (driver = 0; driver < nmad_status->ndrivers; driver++)
	{
		nmad_state_init_driver(nmad_status, driver);
	}

	/* now pass this to the Flash Engine which can accordingly display it */
	flash_engine_init(nmad_status);
}
