{FKT_SETUP_CODE,                        "fkt_setup"},
{FKT_KEYCHANGE_CODE,                    "fkt_keychange"},
{FKT_RESET_CODE,                        "fkt_reset"},
{FKT_CALIBRATE0_CODE,                   "fkt_calibrate0"},
{FKT_CALIBRATE1_CODE,                   "fkt_calibrate1"},
{FKT_CALIBRATE2_CODE,                   "fkt_calibrate2"},
{FKT_NEW_LWP_CODE,                      "fkt_new_lwp"},
{FKT_RET_FROM_SYS_CALL_CODE,            "ret_from_sys_call"},
  
  //			{FKT_EXT2_WRITE_ENTRY_CODE,		"ext2_file_write_entry"},
  //			{FKT_EXT2_WRITE_EXIT_CODE,		"ext2_file_write_exit"},
{FKT_LL_RW_BLOCK_ENTRY_CODE,            "ll_rw_block_entry"},
{FKT_LL_RW_BLOCK_EXIT_CODE,             "ll_rw_block_exit"},
{FKT_EXT2_GETBLK_ENTRY_CODE,            "ext2_getblk_entry"},
{FKT_EXT2_GETBLK_EXIT_CODE,             "ext2_getblk_exit"},
  //			{FKT_INODE_GETBLK_ENTRY_CODE,	"inode_getblk_entry"},
  //			{FKT_INODE_GETBLK_EXIT_CODE,	"inode_getblk_exit"},
  //			{FKT_BLOCK_GETBLK_ENTRY_CODE,	"block_getblk_entry"},
  //			{FKT_BLOCK_GETBLK_EXIT_CODE,	"block_getblk_exit"},
{FKT_SWITCH_TO_CODE,                    "switch_to"},
  
  /* sadhna start */
  
  /* socket interface routines */
{FKT_SOCKET_ENTRY_CODE,                 "socket_entry"},
{FKT_SOCKET_EXIT_CODE,                  "socket_exit"},
{FKT_BIND_ENTRY_CODE,                   "bind_entry"},
{FKT_BIND_EXIT_CODE,                    "bind_exit"},
{FKT_LISTEN_ENTRY_CODE,                 "listen_entry"},
{FKT_LISTEN_EXIT_CODE,                  "listen_exit"},
{FKT_ACCEPT_ENTRY_CODE,                 "accept_entry"},
{FKT_ACCEPT_EXIT_CODE,                  "accept_exit"},
{FKT_CONNECT_ENTRY_CODE,                "connect_entry"},
{FKT_CONNECT_EXIT_CODE,                 "connect_exit"},
{FKT_SEND_ENTRY_CODE,                   "send_entry"},
{FKT_SEND_EXIT_CODE,                    "send_exit"},
{FKT_SENDTO_ENTRY_CODE,                 "sendto_entry"},
{FKT_SENDTO_EXIT_CODE,                  "sendto_exit"},
{FKT_RECVFROM_ENTRY_CODE,               "recvfrom_entry"},
{FKT_RECVFROM_EXIT_CODE,                "recvfrom_exit"},
{FKT_SHUTDOWN_ENTRY_CODE,               "shutdown_entry"},
{FKT_SHUTDOWN_EXIT_CODE,                "shutdown_exit"},
  
  /* lamp routines */
{FKT_LAMP_CREATE_ENTRY_CODE,            "lamp_create_entry"},
{FKT_LAMP_CREATE_EXIT_CODE,             "lamp_create_exit"},
{FKT_LAMP_BIND_ENTRY_CODE,              "lamp_bind_entry"},
{FKT_LAMP_BIND_EXIT_CODE,               "lamp_bind_exit"},
{FKT_LAMP_LISTEN_ENTRY_CODE,            "lamp_listen_entry"},
{FKT_LAMP_LISTEN_EXIT_CODE,             "lamp_listen_exit"},
{FKT_LAMP_ACCEPT_ENTRY_CODE,            "lamp_accept_entry"},
{FKT_LAMP_ACCEPT_EXIT_CODE,             "lamp_accept_exit"},
{FKT_LAMP_CONNECT_ENTRY_CODE,           "lamp_connect_entry"},
{FKT_LAMP_CONNECT_EXIT_CODE,            "lamp_connect_exit"},
{FKT_LAMP_SENDMSG_ENTRY_CODE,           "lamp_sendmsg_entry"},
{FKT_LAMP_SENDMSG_EXIT_CODE,            "lamp_sendmsg_exit"},
{FKT_LAMP_RECVMSG_ENTRY_CODE,           "lamp_recvmsg_entry"},
{FKT_LAMP_RECVMSG_EXIT_CODE,            "lamp_recvmsg_exit"},
{FKT_LAMP_DATA_RCV_ENTRY_CODE,          "lamp_data_rcv_entry"},
{FKT_LAMP_DATA_RCV_EXIT_CODE,           "lamp_data_rcv_exit"},
{FKT_LAMP_SIGNAL_RCV_ENTRY_CODE,        "lamp_signal_rcv_entry"},
{FKT_LAMP_SIGNAL_RCV_EXIT_CODE,         "lamp_signal_rcv_exit"},
{FKT_LAMP_SHUTDOWN_ENTRY_CODE,          "lamp_shutdown_entry"},
{FKT_LAMP_SHUTDOWN_EXIT_CODE,           "lamp_shutdown_exit"},
{FKT_LAMP_RECVMSG_SCHEDI_CODE,          "lamp_recvmsg_sched_entry"},
{FKT_LAMP_RECVMSG_SCHEDO_CODE,          "lamp_recvmsg_sched_exit"},
  
  /*	inet routines */
{FKT_INET_CREATE_ENTRY_CODE,            "inet_create_entry"},
{FKT_INET_CREATE_EXIT_CODE,             "inet_create_exit"},
{FKT_INET_BIND_ENTRY_CODE,              "inet_bind_entry"},
{FKT_INET_BIND_EXIT_CODE,               "inet_bind_exit"},
{FKT_INET_LISTEN_ENTRY_CODE,            "inet_listen_entry"},
{FKT_INET_LISTEN_EXIT_CODE,             "inet_listen_exit"},
{FKT_INET_ACCEPT_ENTRY_CODE,            "inet_accept_entry"},
{FKT_INET_ACCEPT_EXIT_CODE,             "inet_accept_exit"},
{FKT_INET_STREAM_CONNECT_ENTRY_CODE,    "inet_stream_connect_entry"},
{FKT_INET_STREAM_CONNECT_EXIT_CODE,     "inet_stream_connect_exit"},
{FKT_INET_SENDMSG_ENTRY_CODE,           "inet_sendmsg_entry"},
{FKT_INET_SENDMSG_EXIT_CODE,            "inet_sendmsg_exit"},
{FKT_INET_RECVMSG_ENTRY_CODE,           "inet_recvmsg_entry"},
{FKT_INET_RECVMSG_EXIT_CODE,            "inet_recvmsg_exit"},
{FKT_INET_SHUTDOWN_ENTRY_CODE,          "inet_shutdown_entry"},
{FKT_INET_SHUTDOWN_EXIT_CODE,           "inet_shutdown_exit"},
  
  /*	net routines */
{FKT_NET_RX_ACTION_ENTRY_CODE,          "net_rx_action_entry"},
{FKT_NET_RX_ACTION_EXIT_CODE,           "net_rx_action_exit"},
  //			{FKT_NET_BH_QUEUE_ENTRY_CODE, 	   "net_bh_queue_entry"},
  //			{FKT_NET_BH_QUEUE_EXIT_CODE, 	   "net_bh_queue_exit"},
{FKT_NETIF_RX_ENTRY_CODE,               "netif_rx_entry"},
{FKT_NETIF_RX_EXIT_CODE,                "netif_rx_exit"},
{FKT_NET_RX_DEQUEUE_ENTRY_CODE,         "net_rx_dequeue_entry"},
{FKT_NET_RX_DEQUEUE_EXIT_CODE,          "net_rx_dequeue_exit"},
  
  /* sadhna end */
  
  /* miru start */
  
  /*	tcp routines */
{FKT_TCP_SENDMSG_ENTRY_CODE,            "tcp_sendmsg_entry"},
{FKT_TCP_SENDMSG_EXIT_CODE,             "tcp_sendmsg_exit"},
{FKT_TCP_SEND_SKB_ENTRY_CODE,           "tcp_send_skb_entry"},
{FKT_TCP_SEND_SKB_EXIT_CODE,            "tcp_send_skb_exit"},
{FKT_TCP_TRANSMIT_SKB_ENTRY_CODE,       "tcp_transmit_skb_entry"},
{FKT_TCP_TRANSMIT_SKB_EXIT_CODE,        "tcp_transmit_skb_exit"},
{FKT_TCP_V4_RCV_ENTRY_CODE,             "tcp_v4_rcv_entry"},
{FKT_TCP_V4_RCV_EXIT_CODE,              "tcp_v4_rcv_exit"},
  //			{FKT_TCP_RECVMSG_SCHEDI_CODE,	"tcp_data_wait_entry"},
  //			{FKT_TCP_RECVMSG_SCHEDO_CODE,	"tcp_data_wait_exit"},
{FKT_TCP_RCV_ESTABLISHED_ENTRY_CODE,    "tcp_rcv_established_entry"},
{FKT_TCP_RCV_ESTABLISHED_EXIT_CODE,     "tcp_rcv_established_exit"},
{FKT_TCP_DATA_ENTRY_CODE,               "tcp_data_entry"},
{FKT_TCP_DATA_EXIT_CODE,                "tcp_data_exit"},
{FKT_TCP_ACK_ENTRY_CODE,                "tcp_ack_entry"},
{FKT_TCP_ACK_EXIT_CODE,                 "tcp_ack_exit"},
  
{FKT_TCP_RECVMSG_ENTRY_CODE,            "tcp_recvmsg_entry"},
{FKT_TCP_RECVMSG_EXIT_CODE,             "tcp_recvmsg_exit"},
  //			{FKT_TCP_V4_RCV_CSUM_ENTRY_CODE,   "tcp_v4_checksum_init_entry"},
  //			{FKT_TCP_V4_RCV_CSUM_EXIT_CODE,    "tcp_v4_checksum_init_exit"},
{FKT_TCP_RECVMSG_OK_ENTRY_CODE,         "tcp_recvmsg_ok_entry"},
{FKT_TCP_RECVMSG_OK_EXIT_CODE,          "tcp_recvmsg_ok_exit"},
{FKT_TCP_SEND_ACK_ENTRY_CODE,           "tcp_send_ack_entry"},
{FKT_TCP_SEND_ACK_EXIT_CODE,            "tcp_send_ack_exit"},
  //			{FKT_TCP_V4_RCV_HW_ENTRY_CODE,     "tcp_v4_rcv_hw_entry"},
  //			{FKT_TCP_V4_RCV_HW_EXIT_CODE,      "tcp_v4_rcv_hw_exit"},
  //			{FKT_TCP_V4_RCV_NONE_ENTRY_CODE,   "tcp_v4_rcv_none_entry"},
  //			{FKT_TCP_V4_RCV_NONE_EXIT_CODE,    "tcp_v4_rcv_none_exit"},
{FKT_WAIT_FOR_TCP_MEMORY_ENTRY_CODE,    "wait_for_tcp_memory_entry"},
{FKT_WAIT_FOR_TCP_MEMORY_EXIT_CODE,     "wait_for_tcp_memory_exit"},
  //			{FKT_TCP_RECVMSG_CSUM_ENTRY_CODE,  "tcp_recvmsg_csum_entry"},
  //			{FKT_TCP_RECVMSG_CSUM_EXIT_CODE,   "tcp_recvmsg_csum_exit"},
  //			{FKT_TCP_RECVMSG_CSUM2_ENTRY_CODE, "tcp_recvmsg_csum2_entry"},
  //			{FKT_TCP_RECVMSG_CSUM2_EXIT_CODE,  "tcp_recvmsg_csum2_exit"},
{FKT_TCP_SYNC_MSS_ENTRY_CODE,           "tcp_sync_mss_entry"},
{FKT_TCP_SYNC_MSS_EXIT_CODE,            "tcp_sync_mss_exit"},
  
  /* udp routines */
{FKT_UDP_SENDMSG_ENTRY_CODE,            "udp_sendmsg_entry"},
{FKT_UDP_SENDMSG_EXIT_CODE,             "udp_sendmsg_exit"},
{FKT_UDP_QUEUE_RCV_SKB_ENTRY_CODE,      "udp_queue_rcv_skb_entry"},
{FKT_UDP_QUEUE_RCV_SKB_EXIT_CODE,       "udp_queue_rcv_skb_exit"},
{FKT_UDP_GETFRAG_ENTRY_CODE,            "udp_getfrag_entry"},
{FKT_UDP_GETFRAG_EXIT_CODE,             "udp_getfrag_exit"},
{FKT_UDP_RECVMSG_ENTRY_CODE,            "udp_recvmsg_entry"},
{FKT_UDP_RECVMSG_EXIT_CODE,             "udp_recvmsg_exit"},
{FKT_UDP_RCV_ENTRY_CODE,                "udp_rcv_entry"},
{FKT_UDP_RCV_EXIT_CODE,                 "udp_rcv_exit"},
  
  /*	ip routines */
{FKT_IP_RCV_ENTRY_CODE,                 "ip_rcv_entry"},
{FKT_IP_RCV_EXIT_CODE,                  "ip_rcv_exit"},
{FKT_IP_QUEUE_XMIT_ENTRY_CODE,          "ip_queue_xmit_entry"},
{FKT_IP_QUEUE_XMIT_EXIT_CODE,           "ip_queue_xmit_exit"},
{FKT_IP_BUILD_XMIT_ENTRY_CODE,          "ip_build_xmit_entry"},
{FKT_IP_BUILD_XMIT_EXIT_CODE,           "ip_build_xmit_exit"},
{FKT_IP_LOCAL_DELIVER_ENTRY_CODE,       "ip_local_deliver_entry"},
{FKT_IP_LOCAL_DELIVER_EXIT_CODE,        "ip_local_deliver_exit"},
{FKT_IP_DEFRAG_ENTRY_CODE,              "ip_defrag_entry"},
{FKT_IP_DEFRAG_EXIT_CODE,               "ip_defrag_exit"},
{FKT_IP_FRAG_REASM_ENTRY_CODE,          "ip_frag_reasm_entry"},
{FKT_IP_FRAG_REASM_EXIT_CODE,           "ip_frag_reasm_exit"},
  
  /*	ethernet cards */
{FKT_TULIP_START_XMIT_ENTRY_CODE,       "tulip_start_xmit_entry"},
{FKT_TULIP_START_XMIT_EXIT_CODE,        "tulip_start_xmit_exit"},
{FKT_TULIP_INTERRUPT_ENTRY_CODE,        "tulip_interrupt_entry"},
{FKT_TULIP_INTERRUPT_EXIT_CODE,         "tulip_interrupt_exit"},
{FKT_TULIP_RX_ENTRY_CODE,               "tulip_rx_entry"},
{FKT_TULIP_RX_EXIT_CODE,                "tulip_rx_exit"},
{FKT_TULIP_TX_ENTRY_CODE,               "tulip_tx_entry"},
{FKT_TULIP_TX_EXIT_CODE,                "tulip_tx_exit"},
  
{FKT_VORTEX_INTERRUPT_ENTRY_CODE,       "vortex_interrupt_entry"},
{FKT_VORTEX_INTERRUPT_EXIT_CODE,        "vortex_interrupt_exit"},
{FKT_BOOMERANG_INTERRUPT_ENTRY_CODE,    "boomerang_interrupt_entry"},
{FKT_BOOMERANG_INTERRUPT_EXIT_CODE,     "boomerang_interrupt_exit"},
{FKT_BOOMERANG_START_XMIT_ENTRY_CODE,   "boomerang_start_xmit_entry"},
{FKT_BOOMERANG_START_XMIT_EXIT_CODE,    "boomerang_start_xmit_exit"},
{FKT_BOOMERANG_RX_ENTRY_CODE,           "boomerang_rx_entry"},
{FKT_BOOMERANG_RX_EXIT_CODE,            "boomerang_rx_exit"},
{FKT_BOOMERANG_TX_ENTRY_CODE,           "boomerang_tx_entry"},
{FKT_BOOMERANG_TX_EXIT_CODE,            "boomerang_tx_exit"},
  
{FKT_DO_SOFTIRQ_ENTRY_CODE,             "do_softirq_entry"},
{FKT_DO_SOFTIRQ_EXIT_CODE,              "do_softirq_exit"},
  
{FKT_ACE_INTERRUPT_ENTRY_CODE,          "ace_interrupt_entry"},
{FKT_ACE_INTERRUPT_EXIT_CODE,           "ace_interrupt_exit"},
{FKT_ACE_RX_INT_ENTRY_CODE,             "ace_rx_int_entry"},
{FKT_ACE_RX_INT_EXIT_CODE,              "ace_rx_int_exit"},
{FKT_ACE_TX_INT_ENTRY_CODE,             "ace_tx_int_entry"},
{FKT_ACE_TX_INT_EXIT_CODE,              "ace_tx_int_exit"},
{FKT_ACE_START_XMIT_ENTRY_CODE,         "ace_start_xmit_entry"},
{FKT_ACE_START_XMIT_EXIT_CODE,          "ace_start_xmit_exit"},
  
  /* miru end */
  
  
  /*	the sys_xxx functions */
  /* achadda start */
{FKT_READ_ENTRY_CODE,                   "sys_read_entry"},
{FKT_READ_EXIT_CODE,                    "sys_read_exit"},
  
  /* vs start */
{FKT_WRITE_ENTRY_CODE,                  "sys_write_entry"},
{FKT_WRITE_EXIT_CODE,                   "sys_write_exit"},
  
  /* rdr */
{FKT_LSEEK_ENTRY_CODE,                  "sys_lseek_entry"},
{FKT_LSEEK_EXIT_CODE,                   "sys_lseek_exit"},
  
  /* vs start */
{FKT_FSYNC_ENTRY_CODE,                  "sys_fsync_entry"},
{FKT_FSYNC_EXIT_CODE,                   "sys_fsync_exit"},
{FKT_NANOSLEEP_ENTRY_CODE,              "sys_nanosleep_entry"},
{FKT_NANOSLEEP_EXIT_CODE,               "sys_nanosleep_exit"},
  
  
  /* rdr start */
  //			{FKT_DO_SD_REQUEST_ENTRY_CODE,	"do_sd_request_entry"},
  //			{FKT_DO_SD_REQUEST_EXIT_CODE,	"do_sd_request_exit"},
  //			{FKT_REQUEUE_SD_REQUEST_ENTRY_CODE, "requeue_sd_request_entry"},
  //			{FKT_REQUEUE_SD_REQUEST_EXIT_CODE, "requeue_sd_request_exit"},
{FKT_SD_OPEN_ENTRY_CODE,                "sd_open_entry"},
{FKT_SD_OPEN_EXIT_CODE,                 "sd_open_exit"},
{FKT_SD_RELEASE_ENTRY_CODE,             "sd_release_entry"},
{FKT_SD_RELEASE_EXIT_CODE,              "sd_release_exit"},
{FKT_SD_IOCTL_ENTRY_CODE,               "sd_ioctl_entry"},
{FKT_SD_IOCTL_EXIT_CODE,                "sd_ioctl_exit"},
{FKT_SCSI_DO_CMD_ENTRY_CODE,            "scsi_do_cmd_entry"},
{FKT_SCSI_DO_CMD_EXIT_CODE,             "scsi_do_cmd_exit"},
{FKT_SCSI_IOCTL_ENTRY_CODE,             "scsi_ioctl_entry"},
{FKT_SCSI_IOCTL_EXIT_CODE,              "scsi_ioctl_exit"},
{FKT_SCSI_DONE_ENTRY_CODE,              "scsi_done_entry"},
{FKT_SCSI_DONE_EXIT_CODE,               "scsi_done_exit"},
{FKT_SCSI_DISPATCH_CMD_ENTRY_CODE,      "scsi_dispatch_cmd_entry"},
{FKT_SCSI_DISPATCH_CMD_EXIT_CODE,       "scsi_dispatch_cmd_exit"},
{FKT_SCSI_RETRY_COMMAND_ENTRY_CODE,     "scsi_retry_command_entry"},
{FKT_SCSI_RETRY_COMMAND_EXIT_CODE,      "scsi_retry_command_exit"},
{FKT_SG_OPEN_ENTRY_CODE,                "sg_open_entry"},
{FKT_SG_OPEN_EXIT_CODE,                 "sg_open_exit"},
{FKT_SG_RELEASE_ENTRY_CODE,             "sg_release_entry"},
{FKT_SG_RELEASE_EXIT_CODE,              "sg_release_exit"},
{FKT_SG_IOCTL_ENTRY_CODE,               "sg_ioctl_entry"},
{FKT_SG_IOCTL_EXIT_CODE,                "sg_ioctl_exit"},
{FKT_SG_READ_ENTRY_CODE,                "sg_read_entry"},
{FKT_SG_READ_EXIT_CODE,                 "sg_read_exit"},
{FKT_SG_NEW_READ_ENTRY_CODE,            "sg_new_read_entry"},
{FKT_SG_NEW_READ_EXIT_CODE,             "sg_new_read_exit"},
{FKT_SG_WRITE_ENTRY_CODE,               "sg_write_entry"},
{FKT_SG_WRITE_EXIT_CODE,                "sg_write_exit"},
{FKT_SG_NEW_WRITE_ENTRY_CODE,           "sg_new_write_entry"},
{FKT_SG_NEW_WRITE_EXIT_CODE,            "sg_new_write_exit"},
{FKT_SG_GET_RQ_MARK_ENTRY_CODE,         "sg_get_rq_mark_entry"},
{FKT_SG_GET_RQ_MARK_EXIT_CODE,          "sg_get_rq_mark_exit"},
{FKT_SG_CMD_DONE_BH_ENTRY_CODE,         "sg_cmd_done_bh_entry"},
{FKT_SG_CMD_DONE_BH_EXIT_CODE,          "sg_cmd_done_bh_exit"},
{FKT_SCSI_SEP_IOCTL_ENTRY_CODE,         "scsi_sep_ioctl_entry"},
{FKT_SCSI_SEP_IOCTL_EXIT_CODE,          "scsi_sep_ioctl_exit"},
{FKT_SCSI_SEP_QUEUECOMMAND_ENTRY_CODE,  "scsi_sep_queuecommand_entry"},
{FKT_SCSI_SEP_QUEUECOMMAND_EXIT_CODE,   "scsi_sep_queuecommand_exit"},
  /* rdr end */
  
  /* vs start */
  //			{FKT_SEAGATE_INTERNAL_CMD_ENTRY_CODE,  "seagate_internal_cmd_entry"},
  //			{FKT_SEAGATE_INTERNAL_CMD_EXIT_CODE,   "seagate_internal_cmd_exit"},
  //			{FKT_QLOGIC_QCMND_ENTRY_CODE,          "qlogic_qcmnd_entry"},
  //			{FKT_QLOGIC_QCMND_EXIT_CODE,           "qlogic_qcmnd_exit"},
  //			{FKT_ISP_RET_STATUS_ENTRY_CODE,        "qlogic_ret_status_entry"},
  //			{FKT_ISP_RET_STATUS_EXIT_CODE,         "qlogic_ret_status_exit"},
  //			{AM53C974_QCMD_ENTRY_CODE,             "am53c974_qcmd_entry"},
  //			{AM53C974_QCMD_EXIT_CODE,              "am53c974_qcmd_exit"},
  
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
