/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#ifndef NM_LOG_H
#define NM_LOG_H

#define NM_DISPF(str, ...)		fprintf(stderr, "%s, " str "\n", __TBX_FUNCTION__ , ## __VA_ARGS__)
#define NM_DISP_IN()			fprintf(stderr, "%s, : -->\n", __TBX_FUNCTION__)
#define NM_DISP_OUT()			fprintf(stderr, "%s, : <--\n", __TBX_FUNCTION__)
#define NM_DISP_VAL(str, val)		fprintf(stderr, "%s, " str " = %d\n", __TBX_FUNCTION__ , (int)(val))
#define NM_DISP_CHAR(val)		fprintf(stderr, "%s, %c", __TBX_FUNCTION__ , (char)(val))
#define NM_DISP_PTR(str, ptr)		fprintf(stderr, "%s, " str " = %p\n", __TBX_FUNCTION__ , (void *)(ptr))
#define NM_DISP_STR(str, str2)		fprintf(stderr, "%s, " str ": %s\n, __TBX_FUNCTION__" , (char *)(str2))

#ifdef CONFIG_LOG
#define NM_LOGF(str, ...)		fprintf(stderr, "%s, " str "\n", __TBX_FUNCTION__ , ## __VA_ARGS__)
#define NM_LOG_IN()			fprintf(stderr, "%s, : -->\n", __TBX_FUNCTION__)
#define NM_LOG_OUT()			fprintf(stderr, "%s, : <--\n", __TBX_FUNCTION__)
#define NM_LOG_VAL(str, val)		fprintf(stderr, "%s, " str " = %d\n", __TBX_FUNCTION__ , (int)(val))
#define NM_LOG_CHAR(val)		fprintf(stderr, "%s, %c" ,, __TBX_FUNCTION__ (char)(val))
#define NM_LOG_PTR(str, ptr)		fprintf(stderr, "%s, " str " = %p\n", __TBX_FUNCTION__ , (void *)(ptr))
#define NM_LOG_STR(str, str2)		fprintf(stderr,"%s, "  str ": %s\n", __TBX_FUNCTION__ , (char *)(str2))
#else
#define NM_LOGF(str, ...)     (void)(0)
#define NM_LOG_IN()           (void)(0)
#define NM_LOG_OUT()          (void)(0)
#define NM_LOG_CHAR(val)      (void)(0)
#define NM_LOG_VAL(str, val)  (void)(0)
#define NM_LOG_PTR(str, ptr)  (void)(0)
#define NM_LOG_STR(str, str2) (void)(0)
#endif

#ifdef CONFIG_TRACE
#define NM_TRACEF(str, ...)		fprintf(stderr, "%s, " str "\n", __TBX_FUNCTION__ , ## __VA_ARGS__)
#define NM_TRACE_IN()			fprintf(stderr, "%s, : -->\n", __TBX_FUNCTION__)
#define NM_TRACE_OUT()			fprintf(stderr, "%s, : <--\n", __TBX_FUNCTION__)
#define NM_TRACE_VAL(str, val)		fprintf(stderr, "%s, " str " = %d\n", __TBX_FUNCTION__ , (int)(val))
#define NM_TRACE_CHAR(val)		fprintf(stderr, "%s, %c" , (char)(val))
#define NM_TRACE_PTR(str, ptr)		fprintf(stderr, "%s, " str " = %p\n", __TBX_FUNCTION__ , (void *)(ptr))
#define NM_TRACE_STR(str, str2)		fprintf(stderr, "%s, " str ": %s\n", __TBX_FUNCTION__ , (char *)(str2))
#else
#define NM_TRACEF(str, ...)     (void)(0)
#define NM_TRACE_IN()           (void)(0)
#define NM_TRACE_OUT()          (void)(0)
#define NM_TRACE_CHAR(val)      (void)(0)
#define NM_TRACE_VAL(str, val)  (void)(0)
#define NM_TRACE_PTR(str, ptr)  (void)(0)
#define NM_TRACE_STR(str, str2) (void)(0)
#endif

/* Profiling/post-portem analysis
 */
#ifdef PROFILE
#  define FUT_NMAD_CODE			0xfe00

#  define FUT_NMAD_EVENT0_CODE		FUT_NMAD_CODE + 0x00
#  define FUT_NMAD_EVENT1_CODE		FUT_NMAD_CODE + 0x01
#  define FUT_NMAD_EVENT2_CODE		FUT_NMAD_CODE + 0x02
#  define FUT_NMAD_EVENTSTR_CODE	FUT_NMAD_CODE + 0x03

#  define FUT_NMAD_EVENT_CONFIG_CODE		FUT_NMAD_CODE + 0x10
#  define FUT_NMAD_EVENT_DRV_ID_CODE		FUT_NMAD_CODE + 0x11
#  define FUT_NMAD_EVENT_DRV_NAME_CODE		FUT_NMAD_CODE + 0x12
#  define FUT_NMAD_EVENT_CNX_CONNECT_CODE	FUT_NMAD_CODE + 0x13
#  define FUT_NMAD_EVENT_CNX_ACCEPT_CODE	FUT_NMAD_CODE + 0x14
#  define FUT_NMAD_EVENT_NEW_TRK_CODE		FUT_NMAD_CODE + 0x15
#  define FUT_NMAD_EVENT_SND_START_CODE		FUT_NMAD_CODE + 0x16
#  define FUT_NMAD_EVENT_RCV_END_CODE		FUT_NMAD_CODE + 0x17

#  define FUT_NMAD_EVENT_ANNOTATE_CODE		FUT_NMAD_CODE + 0x20

#  define FUT_NMAD_INIT_NIC			FUT_NMAD_CODE + 0x40
#  define FUT_NMAD_INIT_GATE			FUT_NMAD_CODE + 0x41
#  define FUT_NMAD_INIT_CORE			FUT_NMAD_CODE + 0x42
#  define FUT_NMAD_INIT_SCHED			FUT_NMAD_CODE + 0x43

#  define FUT_NMAD_GATE_NEW_INPUT_LIST		FUT_NMAD_CODE + 0x44
#  define FUT_NMAD_GATE_NEW_OUTPUT_LIST		FUT_NMAD_CODE + 0x45


#  define FUT_NMAD_NIC_NEW_INPUT_LIST		FUT_NMAD_CODE + 0x46
#  define FUT_NMAD_NIC_NEW_OUTPUT_LIST		FUT_NMAD_CODE + 0x47

#  define FUT_NMAD_GATE_OPS_CREATE_PACKET	FUT_NMAD_CODE + 0x48
#  define FUT_NMAD_GATE_OPS_INSERT_PACKET	FUT_NMAD_CODE + 0x49
#  define FUT_NMAD_GATE_OPS_IN_TO_OUT		FUT_NMAD_CODE + 0x4a
#  define FUT_NMAD_GATE_OPS_OUT_TO_TRACK	FUT_NMAD_CODE + 0x4b

#  define FUT_NMAD_NIC_OPS_GATE_TO_TRACK	FUT_NMAD_CODE + 0x4c
#  define FUT_NMAD_NIC_OPS_SEND_PACKET		FUT_NMAD_CODE + 0x4d

#  define FUT_NMAD_NIC_RECV_ACK_RNDV		FUT_NMAD_CODE + 0x4e

#  define FUT_NMAD_GATE_OPS_CREATE_CTRL_PACKET	FUT_NMAD_CODE + 0x4f
#  define FUT_NMAD_GATE_OPS_IN_TO_OUT_AGREG	FUT_NMAD_CODE + 0x50
#  define FUT_NMAD_NIC_OPS_TRACK_TO_DRIVER	FUT_NMAD_CODE + 0x51
#  define FUT_NMAD_GATE_OPS_IN_TO_OUT_SPLIT	FUT_NMAD_CODE + 0x52

#  define FUT_NMAD_INIT_NIC_URL			FUT_NMAD_CODE + 0x53

#  define NMAD_EVENT0()					\
 PROF_START						\
      FUT_DO_PROBE0(FUT_NMAD_EVENT0_CODE);		\
 PROF_END

#  define NMAD_EVENT1(a)				\
 PROF_START						\
      FUT_DO_PROBE1(FUT_NMAD_EVENT1_CODE, a);		\
 PROF_END

#  define NMAD_EVENT2(a, b)				\
 PROF_START						\
      FUT_DO_PROBE2(FUT_NMAD_EVENT2_CODE, a, b);	\
 PROF_END

#  define NMAD_EVENTSTR(s, ...)							\
 PROF_START									\
      FUT_DO_PROBESTR(FUT_NMAD_EVENTSTR_CODE, s , ## __VA_ARGS__);		\
 PROF_END

#  define NMAD_EVENT_CONFIG(size, rank)				\
 PROF_START							\
      FUT_DO_PROBE2(FUT_NMAD_EVENT_CONFIG_CODE, size, rank);	\
 PROF_END

#  define NMAD_EVENT_DRV_ID(drv_id)				\
 PROF_START							\
      FUT_DO_PROBE1(FUT_NMAD_EVENT_DRV_ID_CODE, drv_id);	\
 PROF_END

#  define NMAD_EVENT_DRV_NAME(drv_name)				\
 PROF_START							\
      FUT_DO_PROBESTR(FUT_NMAD_EVENT_DRV_NAME_CODE, drv_name);	\
 PROF_END

#  define NMAD_EVENT_CNX_CONNECT(remote_rank, gate_id, drv_id)				\
 PROF_START										\
      FUT_DO_PROBE3(FUT_NMAD_EVENT_CNX_CONNECT_CODE, remote_rank, gate_id, drv_id);	\
 PROF_END

#  define NMAD_EVENT_CNX_ACCEPT(remote_rank, gate_id, drv_id)				\
 PROF_START										\
      FUT_DO_PROBE3(FUT_NMAD_EVENT_CNX_ACCEPT_CODE, remote_rank, gate_id, drv_id);	\
 PROF_END

#  define NMAD_EVENT_NEW_TRK(gate_id, drv_id, trk_id)					\
 PROF_START										\
      FUT_DO_PROBE3(FUT_NMAD_EVENT_NEW_TRK_CODE, gate_id, drv_id, trk_id);		\
 PROF_END

#  define NMAD_EVENT_SND_START(gate_id, drv_id, trk_id, length)				\
 PROF_START										\
      FUT_DO_PROBE4(FUT_NMAD_EVENT_SND_START_CODE, gate_id, drv_id, trk_id, length);	\
 PROF_END

#  define NMAD_EVENT_RCV_END(gate_id, drv_id, trk_id, length)				\
 PROF_START										\
      FUT_DO_PROBE4(FUT_NMAD_EVENT_RCV_END_CODE, gate_id, drv_id, trk_id, length);	\
 PROF_END

#  define NMAD_EVENT_ANNOTATE(str, ...)							\
 PROF_START										\
      FUT_DO_PROBESTR(FUT_NMAD_EVENT_ANNOTATE_CODE, str , ## __VA_ARGS__);		\
 PROF_END

#else
#  define NMAD_EVENT0()				(void)0
#  define NMAD_EVENT1(a)			(void)0
#  define NMAD_EVENT2(a, b)			(void)0
#  define NMAD_EVENTSTR(s, ...)			(void)0
#  define NMAD_EVENT_CONFIG(size, rank)		(void)0
#  define NMAD_EVENT_DRV_ID(drv_id)		(void)0
#  define NMAD_EVENT_DRV_NAME(drv_name)		(void)0
#  define NMAD_EVENT_CNX_CONNECT(remote_rank, gate_id, drv_id)		(void)0
#  define NMAD_EVENT_CNX_ACCEPT(remote_rank, gate_id, drv_id)		(void)0
#  define NMAD_EVENT_NEW_TRK(gate_id, drv_id, trk_id)			(void)0
#  define NMAD_EVENT_SND_START(gate_id, drv_id, trk_id, length)		(void)0
#  define NMAD_EVENT_RCV_END(gate_id, drv_id, trk_id, length)		(void)0
#  define NMAD_EVENT_ANNOTATE(str, ...)		(void)0
#  define FUT_DO_PROBE0(a)			(void)0
#  define FUT_DO_PROBE1(a, b)			(void)0
#  define FUT_DO_PROBE2(a, b, c)		(void)0
#  define FUT_DO_PROBE3(a, b, c, d)		(void)0
#  define FUT_DO_PROBE4(a, b, c, d, e)		(void)0
#  define FUT_DO_PROBE5(a, b, c, d, e, f)	(void)0
#endif

#endif /* NM_LOG_H */
