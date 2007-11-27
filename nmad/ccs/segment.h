#ifndef SEGMENT_H
#define SEGMENT_H

#include "ccsi.h"
#include "ccs_dataloop.h"

/* CCSI_Segment_piece_params
 *
 * This structure is used to pass function-specific parameters into our 
 * segment processing function.  This allows us to get additional parameters
 * to the functions it calls without changing the prototype.
 */
struct CCSI_Segment_piece_params {
    union {
        struct {
            char *pack_buffer;
        } pack;
        struct {
            DLOOP_VECTOR *vectorp;
            int index;
            int length;
        } pack_vector;
        struct {
	    int64_t *offp;
	    int *sizep; /* see notes in Segment_flatten header */
            int index;
            int length;
        } flatten;
	struct {
	    char *last_loc;
	    int count;
	} contig_blocks;
        struct {
            char *unpack_buffer;
        } unpack;
        struct {
            int stream_off;
        } print;
	struct
	{
	    struct CCSI_Segment segment;
	    int first;
	    unsigned local_buf;
	    unsigned rank;
	    unsigned node_id;
	    unsigned port_id;
	    struct CCSI_dev_callback_rec *callback_rec;
	} putget;
    } u;
};

void CCSI_Segment_pack(struct DLOOP_Segment *segp, DLOOP_Offset first, DLOOP_Offset *lastp, void *pack_buffer);
void CCSI_Segment_unpack(struct DLOOP_Segment *segp, DLOOP_Offset first, DLOOP_Offset *lastp, const DLOOP_Buffer unpack_buffer);

void CCSI_Segment_count_contig_blocks(struct DLOOP_Segment *segp, DLOOP_Offset first, DLOOP_Offset *lastp, int *countp);
void CCSI_Segment_pack_vector(struct DLOOP_Segment *segp, DLOOP_Offset first, DLOOP_Offset *lastp, DLOOP_VECTOR *vectorp, int *lengthp);


#endif
