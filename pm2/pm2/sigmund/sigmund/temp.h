#ifndef TEMP_H
#define TEMP_H

#include "fkt.h"

struct code_name	fkt_code_table[] =
{
  {FKT_SETUP_CODE,				"fkt_setup"},
  {FKT_KEYCHANGE_CODE,			"fkt_keychange"},
  {FKT_RESET_CODE,				"fkt_reset"},
  {FKT_CALIBRATE0_CODE,			"fkt_calibrate0"},
  {FKT_CALIBRATE1_CODE,			"fkt_calibrate1"},
  {FKT_CALIBRATE2_CODE,			"fkt_calibrate2"},
  {FKT_RET_FROM_SYS_CALL_CODE,	"ret_from_sys_call"},
  
  //			{FKT_EXT2_WRITE_ENTRY_CODE,		"ext2_file_write_entry"},
  //			{FKT_EXT2_WRITE_EXIT_CODE,		"ext2_file_write_exit"},
  {FKT_LL_RW_BLOCK_ENTRY_CODE,	"ll_rw_block_entry"},
  {FKT_LL_RW_BLOCK_EXIT_CODE,		"ll_rw_block_exit"},
  {FKT_EXT2_GETBLK_ENTRY_CODE,	"ext2_getblk_entry"},
  {FKT_EXT2_GETBLK_EXIT_CODE,		"ext2_getblk_exit"},
  //			{FKT_INODE_GETBLK_ENTRY_CODE,	"inode_getblk_entry"},
  //			{FKT_INODE_GETBLK_EXIT_CODE,	"inode_getblk_exit"},
  //			{FKT_BLOCK_GETBLK_ENTRY_CODE,	"block_getblk_entry"},
  //			{FKT_BLOCK_GETBLK_EXIT_CODE,	"block_getblk_exit"},
  {FKT_SWITCH_TO_CODE,			"switch_to"},
  
  /* sadhna start */
  
  /* socket interface routines */
  {FKT_SOCKET_ENTRY_CODE,			"socket_entry"},
  {FKT_SOCKET_EXIT_CODE,			"socket_exit"},
  {FKT_BIND_ENTRY_CODE,			"bind_entry"},
  {FKT_BIND_EXIT_CODE,			"bind_exit"},
  {FKT_LISTEN_ENTRY_CODE,			"listen_entry"},
  {FKT_LISTEN_EXIT_CODE,			"listen_exit"},
  {FKT_ACCEPT_ENTRY_CODE,			"accept_entry"},
  {FKT_ACCEPT_EXIT_CODE,			"accept_exit"},
  {FKT_CONNECT_ENTRY_CODE,		"connect_entry"},
  {FKT_CONNECT_EXIT_CODE,			"connect_exit"},
  {FKT_SEND_ENTRY_CODE,			"send_entry"},
  {FKT_SEND_EXIT_CODE,			"send_exit"},
  {FKT_SENDTO_ENTRY_CODE,			"sendto_entry"},
  {FKT_SENDTO_EXIT_CODE,			"sendto_exit"},
  {FKT_RECVFROM_ENTRY_CODE,		"recvfrom_entry"},
  {FKT_RECVFROM_EXIT_CODE,		"recvfrom_exit"},
  {FKT_SHUTDOWN_ENTRY_CODE,		"shutdown_entry"},
  {FKT_SHUTDOWN_EXIT_CODE,		"shutdown_exit"},
  
  /* lamp routines */
  {FKT_LAMP_CREATE_ENTRY_CODE,	"lamp_create_entry"},
  {FKT_LAMP_CREATE_EXIT_CODE,		"lamp_create_exit"},
  {FKT_LAMP_BIND_ENTRY_CODE,		"lamp_bind_entry"},
  {FKT_LAMP_BIND_EXIT_CODE,		"lamp_bind_exit"},
  {FKT_LAMP_LISTEN_ENTRY_CODE,	"lamp_listen_entry"},
  {FKT_LAMP_LISTEN_EXIT_CODE,		"lamp_listen_exit"},
  {FKT_LAMP_ACCEPT_ENTRY_CODE,	"lamp_accept_entry"},
  {FKT_LAMP_ACCEPT_EXIT_CODE,		"lamp_accept_exit"},
  {FKT_LAMP_CONNECT_ENTRY_CODE,	"lamp_connect_entry"},
  {FKT_LAMP_CONNECT_EXIT_CODE,	"lamp_connect_exit"},
  {FKT_LAMP_SENDMSG_ENTRY_CODE,	"lamp_sendmsg_entry"},
  {FKT_LAMP_SENDMSG_EXIT_CODE,	"lamp_sendmsg_exit"},
  {FKT_LAMP_RECVMSG_ENTRY_CODE,	"lamp_recvmsg_entry"},
  {FKT_LAMP_RECVMSG_EXIT_CODE,	"lamp_recvmsg_exit"},
  {FKT_LAMP_DATA_RCV_ENTRY_CODE,	"lamp_data_rcv_entry"},
  {FKT_LAMP_DATA_RCV_EXIT_CODE,	"lamp_data_rcv_exit"},
  {FKT_LAMP_SIGNAL_RCV_ENTRY_CODE,"lamp_signal_rcv_entry"},
  {FKT_LAMP_SIGNAL_RCV_EXIT_CODE,	"lamp_signal_rcv_exit"},
  {FKT_LAMP_SHUTDOWN_ENTRY_CODE,	"lamp_shutdown_entry"},
  {FKT_LAMP_SHUTDOWN_EXIT_CODE,	"lamp_shutdown_exit"},
  {FKT_LAMP_RECVMSG_SCHEDI_CODE,	"lamp_recvmsg_sched_entry"},
  {FKT_LAMP_RECVMSG_SCHEDO_CODE,	"lamp_recvmsg_sched_exit"},
  
  /*	inet routines */
  {FKT_INET_CREATE_ENTRY_CODE,	"inet_create_entry"},
  {FKT_INET_CREATE_EXIT_CODE,		"inet_create_exit"},
  {FKT_INET_BIND_ENTRY_CODE,		"inet_bind_entry"},
  {FKT_INET_BIND_EXIT_CODE,		"inet_bind_exit"},
  {FKT_INET_LISTEN_ENTRY_CODE,	"inet_listen_entry"},
  {FKT_INET_LISTEN_EXIT_CODE,		"inet_listen_exit"},
  {FKT_INET_ACCEPT_ENTRY_CODE,	"inet_accept_entry"},
  {FKT_INET_ACCEPT_EXIT_CODE,		"inet_accept_exit"},
  {FKT_INET_STREAM_CONNECT_ENTRY_CODE,"inet_stream_connect_entry"},
  {FKT_INET_STREAM_CONNECT_EXIT_CODE,	"inet_stream_connect_exit"},
  {FKT_INET_SENDMSG_ENTRY_CODE,	"inet_sendmsg_entry"},
  {FKT_INET_SENDMSG_EXIT_CODE,	"inet_sendmsg_exit"},
  {FKT_INET_RECVMSG_ENTRY_CODE,	"inet_recvmsg_entry"},
  {FKT_INET_RECVMSG_EXIT_CODE,	"inet_recvmsg_exit"},
  {FKT_INET_SHUTDOWN_ENTRY_CODE,	"inet_shutdown_entry"},
  {FKT_INET_SHUTDOWN_EXIT_CODE,	"inet_shutdown_exit"},
  
  /*	net routines */
  {FKT_NET_RX_ACTION_ENTRY_CODE,	"net_rx_action_entry"},
  {FKT_NET_RX_ACTION_EXIT_CODE,	"net_rx_action_exit"},
  //			{FKT_NET_BH_QUEUE_ENTRY_CODE, 	   "net_bh_queue_entry"},
  //			{FKT_NET_BH_QUEUE_EXIT_CODE, 	   "net_bh_queue_exit"},
  {FKT_NETIF_RX_ENTRY_CODE,          "netif_rx_entry"},
  {FKT_NETIF_RX_EXIT_CODE,           "netif_rx_exit"},
  {FKT_NET_RX_DEQUEUE_ENTRY_CODE,    "net_rx_dequeue_entry"},
  {FKT_NET_RX_DEQUEUE_EXIT_CODE, 	   "net_rx_dequeue_exit"},
  
  /* sadhna end */
  
  /* miru start */
  
  /*	tcp routines */
  {FKT_TCP_SENDMSG_ENTRY_CODE,      "tcp_sendmsg_entry"},
  {FKT_TCP_SENDMSG_EXIT_CODE,       "tcp_sendmsg_exit"},
  {FKT_TCP_SEND_SKB_ENTRY_CODE,     "tcp_send_skb_entry"},
  {FKT_TCP_SEND_SKB_EXIT_CODE,      "tcp_send_skb_exit"},
  {FKT_TCP_TRANSMIT_SKB_ENTRY_CODE, "tcp_transmit_skb_entry"},
  {FKT_TCP_TRANSMIT_SKB_EXIT_CODE,  "tcp_transmit_skb_exit"},
  {FKT_TCP_V4_RCV_ENTRY_CODE,		"tcp_v4_rcv_entry"},
  {FKT_TCP_V4_RCV_EXIT_CODE,		"tcp_v4_rcv_exit"},
  //			{FKT_TCP_RECVMSG_SCHEDI_CODE,	"tcp_data_wait_entry"},
//			{FKT_TCP_RECVMSG_SCHEDO_CODE,	"tcp_data_wait_exit"},
  {FKT_TCP_RCV_ESTABLISHED_ENTRY_CODE,"tcp_rcv_established_entry"},
  {FKT_TCP_RCV_ESTABLISHED_EXIT_CODE,"tcp_rcv_established_exit"},
  {FKT_TCP_DATA_ENTRY_CODE,          "tcp_data_entry"},
  {FKT_TCP_DATA_EXIT_CODE,           "tcp_data_exit"},
  {FKT_TCP_ACK_ENTRY_CODE,           "tcp_ack_entry"},
  {FKT_TCP_ACK_EXIT_CODE,            "tcp_ack_exit"},
  
  {FKT_TCP_RECVMSG_ENTRY_CODE,       "tcp_recvmsg_entry"},
  {FKT_TCP_RECVMSG_EXIT_CODE,        "tcp_recvmsg_exit"},
  //			{FKT_TCP_V4_RCV_CSUM_ENTRY_CODE,   "tcp_v4_checksum_init_entry"},
  //			{FKT_TCP_V4_RCV_CSUM_EXIT_CODE,    "tcp_v4_checksum_init_exit"},
  {FKT_TCP_RECVMSG_OK_ENTRY_CODE,    "tcp_recvmsg_ok_entry"},
  {FKT_TCP_RECVMSG_OK_EXIT_CODE,     "tcp_recvmsg_ok_exit"},
  {FKT_TCP_SEND_ACK_ENTRY_CODE,      "tcp_send_ack_entry"},
  {FKT_TCP_SEND_ACK_EXIT_CODE,       "tcp_send_ack_exit"},
  //			{FKT_TCP_V4_RCV_HW_ENTRY_CODE,     "tcp_v4_rcv_hw_entry"},
  //			{FKT_TCP_V4_RCV_HW_EXIT_CODE,      "tcp_v4_rcv_hw_exit"},
  //			{FKT_TCP_V4_RCV_NONE_ENTRY_CODE,   "tcp_v4_rcv_none_entry"},
  //			{FKT_TCP_V4_RCV_NONE_EXIT_CODE,    "tcp_v4_rcv_none_exit"},
  {FKT_WAIT_FOR_TCP_MEMORY_ENTRY_CODE,"wait_for_tcp_memory_entry"},
  {FKT_WAIT_FOR_TCP_MEMORY_EXIT_CODE,"wait_for_tcp_memory_exit"},
  //			{FKT_TCP_RECVMSG_CSUM_ENTRY_CODE,  "tcp_recvmsg_csum_entry"},
  //			{FKT_TCP_RECVMSG_CSUM_EXIT_CODE,   "tcp_recvmsg_csum_exit"},
  //			{FKT_TCP_RECVMSG_CSUM2_ENTRY_CODE, "tcp_recvmsg_csum2_entry"},
  //			{FKT_TCP_RECVMSG_CSUM2_EXIT_CODE,  "tcp_recvmsg_csum2_exit"},
  {FKT_TCP_SYNC_MSS_ENTRY_CODE,	   "tcp_sync_mss_entry"},
  {FKT_TCP_SYNC_MSS_EXIT_CODE,	   "tcp_sync_mss_exit"},
  
  /* udp routines */
  {FKT_UDP_SENDMSG_ENTRY_CODE,       "udp_sendmsg_entry"},
  {FKT_UDP_SENDMSG_EXIT_CODE,        "udp_sendmsg_exit"},
  {FKT_UDP_QUEUE_RCV_SKB_ENTRY_CODE, "udp_queue_rcv_skb_entry"},
  {FKT_UDP_QUEUE_RCV_SKB_EXIT_CODE,  "udp_queue_rcv_skb_exit"},
  {FKT_UDP_GETFRAG_ENTRY_CODE,       "udp_getfrag_entry"},
  {FKT_UDP_GETFRAG_EXIT_CODE,        "udp_getfrag_exit"},
  {FKT_UDP_RECVMSG_ENTRY_CODE,       "udp_recvmsg_entry"},
  {FKT_UDP_RECVMSG_EXIT_CODE,        "udp_recvmsg_exit"},
  {FKT_UDP_RCV_ENTRY_CODE,           "udp_rcv_entry"},
  {FKT_UDP_RCV_EXIT_CODE,            "udp_rcv_exit"},
  
  /*	ip routines */
  {FKT_IP_RCV_ENTRY_CODE,			"ip_rcv_entry"},
  {FKT_IP_RCV_EXIT_CODE,			"ip_rcv_exit"},
  {FKT_IP_QUEUE_XMIT_ENTRY_CODE,    "ip_queue_xmit_entry"},
  {FKT_IP_QUEUE_XMIT_EXIT_CODE,     "ip_queue_xmit_exit"},
  {FKT_IP_BUILD_XMIT_ENTRY_CODE,     "ip_build_xmit_entry"},
  {FKT_IP_BUILD_XMIT_EXIT_CODE,      "ip_build_xmit_exit"},
  {FKT_IP_LOCAL_DELIVER_ENTRY_CODE,  "ip_local_deliver_entry"},
  {FKT_IP_LOCAL_DELIVER_EXIT_CODE,   "ip_local_deliver_exit"},
  {FKT_IP_DEFRAG_ENTRY_CODE,         "ip_defrag_entry"},
  {FKT_IP_DEFRAG_EXIT_CODE,          "ip_defrag_exit"},
  {FKT_IP_FRAG_REASM_ENTRY_CODE,     "ip_frag_reasm_entry"},
  {FKT_IP_FRAG_REASM_EXIT_CODE,      "ip_frag_reasm_exit"},
  
  /*	ethernet cards */
  {FKT_TULIP_START_XMIT_ENTRY_CODE,    "tulip_start_xmit_entry"},
  {FKT_TULIP_START_XMIT_EXIT_CODE,     "tulip_start_xmit_exit"},
  {FKT_TULIP_INTERRUPT_ENTRY_CODE,     "tulip_interrupt_entry"},
  {FKT_TULIP_INTERRUPT_EXIT_CODE,      "tulip_interrupt_exit"},
  {FKT_TULIP_RX_ENTRY_CODE,            "tulip_rx_entry"},
  {FKT_TULIP_RX_EXIT_CODE,             "tulip_rx_exit"},
  {FKT_TULIP_TX_ENTRY_CODE,            "tulip_tx_entry"},
  {FKT_TULIP_TX_EXIT_CODE,             "tulip_tx_exit"},
  
  {FKT_VORTEX_INTERRUPT_ENTRY_CODE,    "vortex_interrupt_entry"},
  {FKT_VORTEX_INTERRUPT_EXIT_CODE,     "vortex_interrupt_exit"},
  {FKT_BOOMERANG_INTERRUPT_ENTRY_CODE, "boomerang_interrupt_entry"},
  {FKT_BOOMERANG_INTERRUPT_EXIT_CODE,  "boomerang_interrupt_exit"},
  {FKT_BOOMERANG_START_XMIT_ENTRY_CODE,"boomerang_start_xmit_entry"},
  {FKT_BOOMERANG_START_XMIT_EXIT_CODE, "boomerang_start_xmit_exit"},
  {FKT_BOOMERANG_RX_ENTRY_CODE,        "boomerang_rx_entry"},
  {FKT_BOOMERANG_RX_EXIT_CODE,         "boomerang_rx_exit"},
  {FKT_BOOMERANG_TX_ENTRY_CODE,        "boomerang_tx_entry"},
  {FKT_BOOMERANG_TX_EXIT_CODE,         "boomerang_tx_exit"},
  
  {FKT_DO_SOFTIRQ_ENTRY_CODE,          "do_softirq_entry"},
  {FKT_DO_SOFTIRQ_EXIT_CODE,           "do_softirq_exit"},
  
  {FKT_ACE_INTERRUPT_ENTRY_CODE,  "ace_interrupt_entry"},
  {FKT_ACE_INTERRUPT_EXIT_CODE,   "ace_interrupt_exit"},
  {FKT_ACE_RX_INT_ENTRY_CODE,     "ace_rx_int_entry"},
  {FKT_ACE_RX_INT_EXIT_CODE,      "ace_rx_int_exit"},
  {FKT_ACE_TX_INT_ENTRY_CODE,     "ace_tx_int_entry"},
  {FKT_ACE_TX_INT_EXIT_CODE,      "ace_tx_int_exit"},
  {FKT_ACE_START_XMIT_ENTRY_CODE, "ace_start_xmit_entry"},
  {FKT_ACE_START_XMIT_EXIT_CODE,  "ace_start_xmit_exit"},
  
  /* miru end */
  
  
  /*	the sys_xxx functions */
  /* achadda start */
  {FKT_READ_ENTRY_CODE,			"sys_read_entry"},
  {FKT_READ_EXIT_CODE,			"sys_read_exit"},
  
  /* vs start */
  {FKT_WRITE_ENTRY_CODE,			"sys_write_entry"},
  {FKT_WRITE_EXIT_CODE,			"sys_write_exit"},
  
  /* rdr */
  {FKT_LSEEK_ENTRY_CODE,			"sys_lseek_entry"},
  {FKT_LSEEK_EXIT_CODE,			"sys_lseek_exit"},
  
  /* vs start */
  {FKT_FSYNC_ENTRY_CODE,			"sys_fsync_entry"},
  {FKT_FSYNC_EXIT_CODE,			"sys_fsync_exit"},
  {FKT_NANOSLEEP_ENTRY_CODE,		"sys_nanosleep_entry"},
  {FKT_NANOSLEEP_EXIT_CODE,		"sys_nanosleep_exit"},

  
  /* rdr start */
  //			{FKT_DO_SD_REQUEST_ENTRY_CODE,	"do_sd_request_entry"},
  //			{FKT_DO_SD_REQUEST_EXIT_CODE,	"do_sd_request_exit"},
  //			{FKT_REQUEUE_SD_REQUEST_ENTRY_CODE, "requeue_sd_request_entry"},
  //			{FKT_REQUEUE_SD_REQUEST_EXIT_CODE, "requeue_sd_request_exit"},
  {FKT_SD_OPEN_ENTRY_CODE,		"sd_open_entry"},
  {FKT_SD_OPEN_EXIT_CODE,			"sd_open_exit"},
  {FKT_SD_RELEASE_ENTRY_CODE,		"sd_release_entry"},
  {FKT_SD_RELEASE_EXIT_CODE,		"sd_release_exit"},
  {FKT_SD_IOCTL_ENTRY_CODE,		"sd_ioctl_entry"},
  {FKT_SD_IOCTL_EXIT_CODE,		"sd_ioctl_exit"},
  {FKT_SCSI_DO_CMD_ENTRY_CODE,	"scsi_do_cmd_entry"},
  {FKT_SCSI_DO_CMD_EXIT_CODE,		"scsi_do_cmd_exit"},
  {FKT_SCSI_IOCTL_ENTRY_CODE,		"scsi_ioctl_entry"},
  {FKT_SCSI_IOCTL_EXIT_CODE,		"scsi_ioctl_exit"},
  {FKT_SCSI_DONE_ENTRY_CODE,		"scsi_done_entry"},
  {FKT_SCSI_DONE_EXIT_CODE,		"scsi_done_exit"},
  {FKT_SCSI_DISPATCH_CMD_ENTRY_CODE,	"scsi_dispatch_cmd_entry"},
  {FKT_SCSI_DISPATCH_CMD_EXIT_CODE,	"scsi_dispatch_cmd_exit"},
  {FKT_SCSI_RETRY_COMMAND_ENTRY_CODE,	"scsi_retry_command_entry"},
  {FKT_SCSI_RETRY_COMMAND_EXIT_CODE,	"scsi_retry_command_exit"},
  {FKT_SG_OPEN_ENTRY_CODE,		"sg_open_entry"},
  {FKT_SG_OPEN_EXIT_CODE,			"sg_open_exit"},
  {FKT_SG_RELEASE_ENTRY_CODE,		"sg_release_entry"},
  {FKT_SG_RELEASE_EXIT_CODE,		"sg_release_exit"},
  {FKT_SG_IOCTL_ENTRY_CODE,		"sg_ioctl_entry"},
  {FKT_SG_IOCTL_EXIT_CODE,		"sg_ioctl_exit"},
  {FKT_SG_READ_ENTRY_CODE,		"sg_read_entry"},
  {FKT_SG_READ_EXIT_CODE,			"sg_read_exit"},
  {FKT_SG_NEW_READ_ENTRY_CODE,	"sg_new_read_entry"},
  {FKT_SG_NEW_READ_EXIT_CODE,		"sg_new_read_exit"},
  {FKT_SG_WRITE_ENTRY_CODE,		"sg_write_entry"},
  {FKT_SG_WRITE_EXIT_CODE,		"sg_write_exit"},
  {FKT_SG_NEW_WRITE_ENTRY_CODE,	"sg_new_write_entry"},
  {FKT_SG_NEW_WRITE_EXIT_CODE,	"sg_new_write_exit"},
  {FKT_SG_GET_RQ_MARK_ENTRY_CODE,	"sg_get_rq_mark_entry"},
  {FKT_SG_GET_RQ_MARK_EXIT_CODE,	"sg_get_rq_mark_exit"},
  {FKT_SG_CMD_DONE_BH_ENTRY_CODE,	"sg_cmd_done_bh_entry"},
  {FKT_SG_CMD_DONE_BH_EXIT_CODE,	"sg_cmd_done_bh_exit"},
  {FKT_SCSI_SEP_IOCTL_ENTRY_CODE,	"scsi_sep_ioctl_entry"},
  {FKT_SCSI_SEP_IOCTL_EXIT_CODE,	"scsi_sep_ioctl_exit"},
  {FKT_SCSI_SEP_QUEUECOMMAND_ENTRY_CODE,"scsi_sep_queuecommand_entry"},
  {FKT_SCSI_SEP_QUEUECOMMAND_EXIT_CODE,"scsi_sep_queuecommand_exit"},
  /* rdr end */
  
  /* vs start */
  //			{FKT_SEAGATE_INTERNAL_CMD_ENTRY_CODE, "seagate_internal_cmd_entry"},
  //			{FKT_SEAGATE_INTERNAL_CMD_EXIT_CODE, "seagate_internal_cmd_exit"},
  //			{FKT_QLOGIC_QCMND_ENTRY_CODE,	"qlogic_qcmnd_entry"},
  //			{FKT_QLOGIC_QCMND_EXIT_CODE,	"qlogic_qcmnd_exit"},
  //			{FKT_ISP_RET_STATUS_ENTRY_CODE,	"qlogic_ret_status_entry"},
  //			{FKT_ISP_RET_STATUS_EXIT_CODE,	"qlogic_ret_status_exit"},
  //			{AM53C974_QCMD_ENTRY_CODE,		"am53c974_qcmd_entry"},
  //			{AM53C974_QCMD_EXIT_CODE,		"am53c974_qcmd_exit"},
  
  /*****  vs: old codes, need to be reassigned if used again *****
	  {0x581, "ext2_sync_file_entry"},
	  {0x681, "ext2_sync_file_exit"},
	  {0x582, "sync_tind_entry"},
	  {0x682, "sync_tind_exit"},
	  {0x583, "sync_dind_entry"},
	  {0x683, "sync_dind_exit"},
	  {0x584, "sync_indirect_entry"},
	  {0x684, "sync_indirect_exit"},
	  {0x585, "sync_direct_entry"},
	  {0x685, "sync_direct_exit"},
	  {0x586, "sync_iblock_entry"},
	  {0x686, "sync_iblock_exit"},
	  {0x587, "sync_block_entry"},
	  {0x687, "sync_block_exit"},
  *****/
  
  //			{REQUE_DEVM_DEV_BLK_CODE,		"reque_prior_devm_dev_blk"},
  //			{REQUE_BLK_CODE,				"reque_after_blk"},
  /* vs end */
  
  {0, NULL}
};

#define	NTRAPS	20

char	*traps[NTRAPS+1] =	{
  "divide_error",						/*	 0 */
  "debug",							/*	 1 */
  "nmi",								/*	 2 */
  "int3",								/*	 3 */
  "overflow",							/*	 4 */
  "bounds",							/*	 5 */
  "invalid_op",						/*	 6 */
  "device_not_available",				/*	 7 */
  "double_fault",						/*	 8 */
  "coprocessor_segment_overrun",		/*	 9 */
  "invalid_TSS",						/*	10 */
  "segment_not_present",				/*	11 */
  "stack_segment",					/*	12 */
  "general_protection",				/*	13 */
  "page_fault",						/*	14 */
  "spurious_interrupt_bug",			/*	15 */
  "coprocessor_error",				/*	16 */
  "alignment_check",					/*	17 */
  "reserved",							/*	18 */
  "simd_coprocessor_error",			/*	19 */
  "machine_check"						/*	20 */
};


#define NSYS_CALLS		232

char	*sys_calls[NSYS_CALLS+1] = {
  "sys_setup",					/* 0 */
  "sys_exit",	
  "sys_fork",	
  "sys_read",	
  "sys_write",	
  "sys_open",						/* 5 */
  "sys_close",	
  "sys_waitpid",	
  "sys_creat",	
  "sys_link",	
  "sys_unlink",					/* 10 */
  "sys_execve",	
  "sys_chdir",	
  "sys_time",	
  "sys_mknod",	
  "sys_chmod",					/* 15 */
  "sys_chown",	
  "sys_break",	
  "sys_stat",	
  "sys_lseek",	
  "sys_getpid",					/* 20 */
  "sys_mount",	
  "sys_umount",	
  "sys_setuid",	
  "sys_getuid",	
  "sys_stime",					/* 25 */
  "sys_ptrace",	
  "sys_alarm",	
  "sys_fstat",	
  "sys_pause",	
  "sys_utime",					/* 30 */
  "sys_stty",	
  "sys_gtty",	
  "sys_access",	
  "sys_nice",	
  "sys_ftime",					/* 35 */
  "sys_sync",	
  "sys_kill",	
  "sys_rename",	
  "sys_mkdir",	
  "sys_rmdir",					/* 40 */
  "sys_dup",	
  "sys_pipe",	
  "sys_times",	
  "sys_prof",	
  "sys_brk",						/* 45 */
  "sys_setgid",	
  "sys_getgid",	
  "sys_signal",	
  "sys_geteuid",	
  "sys_getegid",					/* 50 */
  "sys_acct",	
  "sys_phys",	
  "sys_lock",	
  "sys_ioctl",	
  "sys_fcntl",					/* 55 */
  "sys_mpx",	
  "sys_setpgid",	
  "sys_ulimit",	
  "sys_olduname",	
  "sys_umask",					/* 60 */
  "sys_chroot",	
  "sys_ustat",	
  "sys_dup2",	
  "sys_getppid",	
  "sys_getpgrp",					/* 65 */
  "sys_setsid",	
  "sys_sigaction",	
  "sys_sgetmask",	
  "sys_ssetmask",	
  "sys_setreuid",					/* 70 */
  "sys_setregid",	
  "sys_sigsuspend",	
  "sys_sigpending",	
  "sys_sethostname",	
  "sys_setrlimit",				/* 75 */
  "sys_getrlimit",	
  "sys_getrusage",	
  "sys_gettimeofday",	
  "sys_settimeofday",	
  "sys_getgroups",				/* 80 */
  "sys_setgroups",	
  "old_select",	
  "sys_symlink",	
  "sys_lstat",	
  "sys_readlink",					/* 85 */
  "sys_uselib",	
  "sys_swapon",	
  "sys_reboot",	
  "old_readdir",	
  "old_mmap",						/* 90 */
  "sys_munmap",	
  "sys_truncate",	
  "sys_ftruncate",	
  "sys_fchmod",	
  "sys_fchown",					/* 95 */
  "sys_getpriority",	
  "sys_setpriority",	
  "sys_profil",	
  "sys_statfs",	
  "sys_fstatfs",					/* 100 */
  "sys_ioperm",	
  "sys_socketcall",	
  "sys_syslog",	
  "sys_setitimer",	
  "sys_getitimer",				/* 105 */
  "sys_newstat",	
  "sys_newlstat",	
  "sys_newfstat",	
  "sys_uname",	
  "sys_iopl",						/* 110 */
  "sys_vhangup",	
  "sys_idle",	
  "sys_vm86",	
  "sys_wait4",	
  "sys_swapoff",					/* 115 */
  "sys_sysinfo",	
  "sys_ipc",	
  "sys_fsync",	
  "sys_sigreturn",	
  "sys_clone",					/* 120 */
  "sys_setdomainname",	
  "sys_newuname",	
  "sys_modify_ldt",	
  "sys_adjtimex",	
  "sys_mprotect",					/* 125 */
  "sys_sigprocmask",	
  "sys_create_module",	
  "sys_init_module",	
  "sys_delete_module",	
  "sys_get_kernel_syms",			/* 130 */
  "sys_quotactl",	
  "sys_getpgid",	
  "sys_fchdir",	
  "sys_bdflush",	
  "sys_sysfs",					/* 135 */
  "sys_personality",	
  "afs_syscall",
  "sys_setfsuid",	
  "sys_setfsgid",	
  "sys_llseek",					/* 140 */
  "sys_getdents",	
  "sys_select",	
  "sys_flock",	
  "sys_msync",	
  "sys_readv",					/* 145 */
  "sys_writev",	
  "sys_getsid",	
  "sys_fdatasync",	
  "sys_sysctl",	
  "sys_mlock",					/* 150 */
  "sys_munlock",	
  "sys_mlockall",	
  "sys_munlockall",	
  "sys_sched_setparam",	
  "sys_sched_getparam",			/* 155 */
  "sys_sched_setscheduler",	
  "sys_sched_getscheduler",	
  "sys_sched_yield",	
  "sys_sched_get_priority_max",	
  "sys_sched_get_priority_min",	/* 160 */
  "sys_sched_rr_get_interval",	
  "sys_nanosleep",	
  "sys_mremap",	
  "sys_setresuid",
  "sys_getresuid",				/* 165 */
  "sys_vm86",
  "sys_query_module",
  "sys_poll",
  "sys_nfsservctl",
  "sys_setresgid",				/* 170 */
  "sys_getresgid",
  "sys_prctl",
  "sys_rt_sigreturn",
  "sys_rt_sigaction",
  "sys_rt_sigprocmask",			/* 175 */
  "sys_rt_sigpending",
  "sys_rt_sigtimedwait",
  "sys_rt_sigqueueinfo",
  "sys_rt_sigsuspend",
  "sys_pread",					/* 180 */
  "sys_pwrite",
  "sys_chown",
  "sys_getcwd",
  "sys_capget",
  "sys_capset",					/* 185 */
  "sys_sigaltstack",
  "sys_sendfile",
  "sys_ni_syscall",				/* streams1 */
  "sys_ni_syscall",				/* streams2 */
  "sys_vfork",		            /* 190 */
  "sys_getrlimit",
  "sys_mmap2",
  "sys_truncate64",
  "sys_ftruncate64",
  "sys_stat64",					/* 195 */
  "sys_lstat64",
  "sys_fstat64",
  "sys_lchown",
  "sys_getuid",
  "sys_getgid",					/* 200 */
  "sys_geteuid",
  "sys_getegid",
  "sys_setreuid",
  "sys_setregid",
  "sys_getgroups",				/* 205 */
  "sys_setgroups",
  "sys_fchown",
  "sys_setresuid",
  "sys_getresuid",
  "sys_setresgid",				/* 210 */
  "sys_getresgid",
  "sys_chown",
  "sys_setuid",
  "sys_setgid",
  "sys_setfsuid",					/* 215 */
  "sys_setfsgid",
  "sys_pivot_root",
  "sys_mincore",
  "sys_madvise",
  "sys_getdents64",				/* 220 */
  "sys_fcntl64",
  "sys_ni_syscall",				/* reserved for TUX */
  "sys_ni_syscall",									   /* rdr */
  "sys_ni_syscall",									   /* rdr */
  "sys_fkt_setup",				/* 225 */			   /* rdr */
  "sys_fkt_endup",									   /* rdr */
  "sys_fkt_keychange",								   /* rdr */
  "sys_fkt_reset",									   /* rdr */
  "sys_fkt_getcopy",									   /* rdr */
  "sys_fkt_probe0",				/* 230 */			   /* rdr */
  "sys_fkt_probe1",									   /* rdr */
  "sys_fkt_probe2"				/* 232 */			   /* rdr */
};

#endif
