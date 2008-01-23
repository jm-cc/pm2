/*
 * That code will be used in order to parse the log a first time : that way, we
 * should determine how many gates, how many queues per gates, ... were created
 * at the initialization so that it will then be possible to render packets 
 * during a second (rather independant) parsing.
 */

#include "parsing.h"
#include "NMadStateInit.h"

fxt_t fut;
struct fxt_ev_64 ev;
struct fxt_ev_64 ev_aux;
fxt_blockev_t block;

/* FUT_NMAD_INIT_CORE */
static void init_core(nmad_state_t *nmad_status)
{
	nmad_status->ngates = 0;
	nmad_status->ndrivers = 0;
}

/* FUT_NMAD_INIT_GATE */
static void init_gate(nmad_state_t *nmad_status, unsigned gateid)
{
	nmad_status->ngates++;
//	printf("NEW GATE ID = %d (nmad_status->gates = %p)\n", gateid, nmad_status->gates);

	nmad_gate_t *gate = &nmad_status->gates[gateid];
		gate->ninputlists = 0;
		gate->noutputlists = 0;
}

/* FUT_NMAD_GATE_NEW_INPUT_LIST */
static void new_gate_input_list(nmad_state_t *nmad_status, unsigned gateid, unsigned inputlistid)
{
//	printf("NEW GATE INPUT LIST ID = %d (gate %d) \n", inputlistid, gateid);
	nmad_gate_t *gate = &nmad_status->gates[gateid];

	gate->ninputlists++;
}

/* FUT_NMAD_GATE_NEW_OUTPUT_LIST */
static void new_gate_output_list(nmad_state_t *nmad_status, unsigned gateid, unsigned outputlistid)
{
//	printf("NEW GATE OUTPUT LIST ID = %d (gate %d) \n", outputlistid, gateid);
	nmad_gate_t *gate = &nmad_status->gates[gateid];

	gate->noutputlists++;
}

/* FUT_NMAD_INIT_NIC */
static void init_nic(nmad_state_t *nmad_status, unsigned driverid, char *driverurl)
{
	nmad_status->ndrivers++;
//	printf("NEW DRIVER ID = %d %s\n", driverid, driverurl);

	nmad_driver_t *driver = &nmad_status->drivers[driverid];
		//driver->ntracks_pre = 0;
		//driver->ntracks_post = 0;
		strcpy(driver->driverurl, driverurl);
}

/* FUT_NMAD_NIC_NEW_INPUT_LIST */
static void new_nic_input_list(nmad_state_t *nmad_status, unsigned driverid, unsigned inputlistid)
{
	nmad_driver_t *driver = &nmad_status->drivers[driverid];

	driver->ntracks_pre++;
}

/* FUT_NMAD_NIC_NEW_OUTPUT_LIST */
static void new_nic_output_list(nmad_state_t *nmad_status, unsigned driverid, unsigned outputlistid)
{
	nmad_driver_t *driver = &nmad_status->drivers[driverid];

	driver->ntracks_post++;
}

void preprocess_log(nmad_state_t *nmad_status, char *pathtolog)
{
	int fd;

	/* first open the log file */
	fd = open(pathtolog, O_RDONLY);
	if (fd < 0) {
		perror("open failed :");
		exit(-1);
	}

	fut = fxt_fdopen(fd);
	if (!fut) {
		perror("fxt_fdopen :");
		exit(-1);
	}

	block = fxt_blockev_enter(fut);
	hcreate(1024);

	while(1) {
		int ret;

		ret = fxt_next_ev(block, FXT_EV_TYPE_64, (struct fxt_ev *)&ev);
		if (ret != FXT_EV_OK) {
			break;
		}
		
		//int nbparam = ev.nb_params;

		unsigned gateid, driverid;
		unsigned inputlistid, outputlistid;
		char *driverurl;
		
 		//printf("code = %x \n", ev.code);

		switch (ev.code) {
			case FUT_NMAD_INIT_CORE:
				init_core(nmad_status);
				break;
			case FUT_NMAD_INIT_SCHED:
				break;
		
			/* gates */
			case FUT_NMAD_INIT_GATE:
				gateid = ev.param[0];
				init_gate(nmad_status, gateid);
				break;
			case FUT_NMAD_GATE_NEW_INPUT_LIST:
				gateid =  ev.param[1];
				inputlistid = ev.param[0];
				new_gate_input_list(nmad_status, gateid, inputlistid);
				break;
			case FUT_NMAD_GATE_NEW_OUTPUT_LIST:
				gateid =  ev.param[1];
				outputlistid = ev.param[0];
				new_gate_output_list(nmad_status, gateid, outputlistid);
				break;

			/* drivers/nic */
			case FUT_NMAD_INIT_NIC:
				/* ok this is an exception to our model ! */
				ret = fxt_next_ev(block, FXT_EV_TYPE_64,
					(struct fxt_ev *)&ev_aux);
				if (ret != FXT_EV_OK) {
					printf("failed with nic url !\n");
					break;
				}
				driverurl = (char *)&ev_aux.param[0];
				driverid = ev.param[0];	
				init_nic(nmad_status, driverid, driverurl);
				break;
			case FUT_NMAD_NIC_NEW_INPUT_LIST:
				driverid = ev.param[0];
				inputlistid = ev.param[1];
				new_nic_input_list(nmad_status, driverid, inputlistid);
				break;
			case FUT_NMAD_NIC_NEW_OUTPUT_LIST:
				driverid = ev.param[0];
				inputlistid = ev.param[1];
				new_nic_output_list(nmad_status, driverid, outputlistid);
				break;
			case FUT_NMAD_GATE_OPS_CREATE_PACKET:
				/* just an heuristic ... XXX */
				goto init;
				break;
			default:
				break;
		}

	}
	/*
	 * Pass this structure to the State manager so that it
	 * can initialize everything and then give it to the flash engine
	 */
init:
	nmad_state_init(nmad_status);

	close(fd);
}
