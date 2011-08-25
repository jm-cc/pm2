/*
 * This code will be used to parse the log corresponding to the actual packet
 * moves
 */

#include "parsing.h"

fxt_t fut;
struct fxt_ev_64 ev;
fxt_blockev_t block;

void process_log(nmad_state_t *nmad_status, char *pathtolog)
{
	int fd;
	int ret;
	int nevents = 0;

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
		ret = fxt_next_ev(block, FXT_EV_TYPE_64, (struct fxt_ev *)&ev);
		if (ret != FXT_EV_OK) {
			printf("no more block ...\n");
			break;
		}
		
		nevents++;

		//int nbparam = ev.nb_params;
		//printf("process ev = %x\n", ev.code);

		uintptr_t wrapper, new_wrapper;
		unsigned long length, length_new;
		unsigned tag;
		unsigned seq;
		unsigned is_a_control_packet;
		unsigned gateid, driverid;
		unsigned inlist, outlist;
		unsigned trackid;

		switch (ev.code) {
			case FUT_NMAD_GATE_OPS_CREATE_PACKET:
				wrapper = ev.param[0];
				tag = ev.param[1];
				seq = ev.param[2]; 
				length = ev.param[3];
				is_a_control_packet = 0;
				//printf("PACKET %p (length = %d)\n", wrapper, length);
				create_packet(nmad_status, wrapper, length, tag, seq, is_a_control_packet);
				break;
			case FUT_NMAD_GATE_OPS_INSERT_PACKET:
				gateid = ev.param[0];
				inlist = ev.param[1];
				wrapper = ev.param[2];
				insert_packet(nmad_status, wrapper, gateid, inlist);
				break;
			case FUT_NMAD_GATE_OPS_IN_TO_OUT:
				gateid = ev.param[0];
				inlist = ev.param[1];
				outlist = ev.param[2];
				wrapper = ev.param[3];
				in_to_out_ops(nmad_status, wrapper, gateid, inlist, outlist);
				break;
			case FUT_NMAD_GATE_OPS_OUT_TO_TRACK:
				/* XXX not used since FUT_NMAD_NIC_OPS_GATE_TO_TRACK is equivalent */
				printf("WARNING !\n");
				break;
			case FUT_NMAD_NIC_OPS_GATE_TO_TRACK:
				gateid = ev.param[0];
				wrapper = ev.param[1];
				driverid = ev.param[2];
				trackid = ev.param[3];
				//printf("GATE TO TRACK gate = %d driver = %d track = %d\n", gateid, driverid, trackid);
				gate_to_track_ops(nmad_status, wrapper, gateid, driverid, trackid);
				break;
			case FUT_NMAD_NIC_OPS_SEND_PACKET:
				wrapper = ev.param[0];
				driverid = ev.param[1];
				inlist = ev.param[2];
				//printf("SEND track %d driver %d \n", inlist, driverid);
				send_packet(nmad_status, wrapper, driverid, inlist);
				break;
			case FUT_NMAD_NIC_RECV_ACK_RNDV:
				wrapper = ev.param[0];
				gateid = ev.param[1];
				outlist = ev.param[2];
				acknowledge_packet(nmad_status, wrapper, gateid, outlist);
				break;
			case FUT_NMAD_GATE_OPS_CREATE_CTRL_PACKET:
				wrapper = ev.param[0];
				tag = ev.param[1];
				seq = ev.param[2]; 
				length = ev.param[3];
				is_a_control_packet = 1;
				//printf("PACKET CTRL %p\n", wrapper);
				create_packet(nmad_status, wrapper, length, tag, seq, is_a_control_packet);
				break;
			case FUT_NMAD_GATE_OPS_IN_TO_OUT_AGREG:
				gateid = ev.param[0];
				inlist = ev.param[1];
				outlist = ev.param[2];
				wrapper = ev.param[3];
				new_wrapper = ev.param[4];
				//printf("AGREG !\n");
				in_to_out_ops_agreg(nmad_status, wrapper, new_wrapper, gateid, inlist, outlist);
				break;
			case FUT_NMAD_NIC_OPS_TRACK_TO_DRIVER:
				wrapper = ev.param[0];
				driverid = ev.param[1];
				inlist = ev.param[2];
				//printf("TRACK (%d) TO DRIVER (%d)\n", inlist, driverid);
				track_to_driver_ops(nmad_status, wrapper, driverid, inlist);
				break;
			case FUT_NMAD_GATE_OPS_IN_TO_OUT_SPLIT:
				//printf("SPLIT\n");
				wrapper = ev.param[0];
				new_wrapper = ev.param[1];
				length_new = ev.param[2];
				gateid = ev.param[3];
				outlist = ev.param[4];
				in_to_out_ops_split(nmad_status, wrapper, new_wrapper, gateid, outlist, length_new);
				break;
			default:
				break;
		}

	}

	printf("there was %d events\n", nevents);
	close(fd);
}
