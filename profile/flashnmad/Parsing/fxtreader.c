#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fxt/fut.h>
#include <fxt/fxt.h>

#include "nmad_fxt.h"

fxt_t fut;

struct fxt_ev_64 ev;


int main(int argc, char **argv)
{
	char *filename;
	int ret;
	int fd;

	if (argc != 2) {
		printf("Usage : %s filename\n", argv[0]);
		exit(-1);
	}

	filename = argv[1];

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror("open failed :");
		exit(-1);
	}

	fut = fxt_fdopen(fd);
	if (!fut) {
		perror("fxt_fdopen :");
		exit(-1);
	}

	fxt_blockev_t block;
	block = fxt_blockev_enter(fut);
	hcreate(1024);


	while(1) {
		ret = fxt_next_ev(block, FXT_EV_TYPE_64, (struct fxt_ev *)&ev);
		if (ret != FXT_EV_OK) {
			printf("no more block ...\n");
			break;
		}

		int nbparam = ev.nb_params;

		switch (ev.code) {
			case FUT_NMAD_INIT_CORE:
				/* FUT_DO_PROBE0(FUT_NMAD_INIT); */
				printf("Init core\n");
				break;
			case FUT_NMAD_INIT_GATE:
				/* FUT_DO_PROBE1(FUT_NMAD_GATE_INIT, p_gate->id); */
				printf("Init gate %d\n", ev.param[0]);
				break;
			case FUT_NMAD_INIT_NIC:
				/* FUT_DO_PROBE2(FUT_NMAD_DRIVER_DRIVER_INIT, id, p_drv); */
				printf("Init driver %p (id = %d)\n", ev.param[1], ev.param[0]);	
				break;
			case FUT_NMAD_NIC_NEW_INPUT_LIST:
				/* FUT_DO_PROBE3(FUT_NMAD_TRACK_INIT, p_trk->p_drv, p_trk->id, p_drv->nb_tracks); */
				printf("Init track %d for driver %p : id = %d\n", ev.param[1], ev.param[0], ev.param[2]);
				break;
			case FUT_NMAD_GATE_OPS_CREATE_PACKET:
				/*   FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_PACKET, p_so_pw, tag, seq, len);*/
				printf("create packet (wrapper = %p, tag = %d, seq = %d, len = %d)\n",  ev.param[0], ev.param[1], ev.param[2], ev.param[3]);
				break;
			case FUT_NMAD_GATE_OPS_INSERT_PACKET:
				 /*     FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, p_so_pw); */
				printf("insert packet (gate = %d, in list = %d, wrapper = %p)\n", ev.param[0], ev.param[1], ev.param[2]);
				break;
			case FUT_NMAD_GATE_OPS_IN_TO_OUT:
				/*   FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_IN_TO_OUT, p_gate->id, 0, 1, p_so_pw); */
				printf("in to out (gate = %d, in = %d, out = %d, wrapper = %p)\n", ev.param[0], ev.param[1], ev.param[2], ev.param[3]);
				break;
			case FUT_NMAD_NIC_OPS_SEND_PACKET:
				/* FUT_DO_PROBE3(FUT_NMAD_NIC_OPS_SEND_PACKET, p_pw, p_pw->p_drv->id, p_pw->p_trk->id); */
				printf("nic sends packet %p\n", ev.param[0]);
				break;
			case FUT_NMAD_GATE_OPS_IN_TO_OUT_AGREG:
				/*  FUT_DO_PROBE5(FUT_NMAD_GATE_OPS_IN_TO_OUT_AGREG, p_gate->id, 0, 0, NULL, p_so_pw); */
				printf("agregate (input packet = %p with packet= %p, gate = %d, in list = %d, out list = %d)\n", ev.param[3], ev.param[4], ev.param[0], ev.param[1], ev.param[2]);
				break;
			case FUT_NMAD_GATE_OPS_CREATE_CTRL_PACKET:
				/* FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_CTRL_PACKET, NULL, 0, 0, 0); */
				printf("create ctrl packet (wrapper = %p, tag = %d, seq = %d, len = %d)\n",  ev.param[0], ev.param[1], ev.param[2], ev.param[3]);
				break;
			case FUT_NMAD_NIC_OPS_GATE_TO_TRACK:
				/*   FUT_DO_PROBE5(FUT_NMAD_NIC_OPS_GATE_TO_TRACK, p_gate->id,
				 *       0 small packet list, p_so_pw, drv_id, TRK_SMALL );*/
				printf("put packet %p from gate %d to track %d of driver %d \n", ev.param[1], ev.param[0], ev.param[3], ev.param[2]);
				break;
			case  FUT_NMAD_NIC_OPS_TRACK_TO_DRIVER:
				/* FUT_DO_PROBE3(FUT_NMAD_NIC_OPS_TRACK_TO_DRIVER, p_pw, p_pw->p_drv->id, p_pw->p_trk->id); */
				printf("transmit packet %p from track to driver\n", ev.param[0]);
				break;
			case FUT_NMAD_GATE_OPS_IN_TO_OUT_SPLIT:
				/* FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_IN_TO_OUT_SPLIT, p_so_large_pw, p_so_large_pw2, chunk_len);*/
				printf("split packet %p and create packet %p (length = %d)\n", ev.param[0], ev.param[1], ev.param[2]);
				break;
			case FUT_NMAD_NIC_RECV_ACK_RNDV:
				/*       FUT_DO_PROBE2(FUT_NMAD_NIC_RECV_ACK_RNDV, p_so_large_pw,
				 *                                               p_gate->id, 1 large output list);
				 */
				printf("ack packet %p gate_id = %d\n", ev.param[0], ev.param[1]);
				break;
			default:
				break;
		}

//		printf("grabbed a block :\n");
	}

	return 0;
}
