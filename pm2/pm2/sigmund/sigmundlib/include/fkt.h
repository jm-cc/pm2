/*	fkt.h */

#ifndef __FKT_H__
#define __FKT_H__

/*	fkt = Fast Kernel Tracing */


/*	"how" parameter values, analagous to "how" parameters to sigprocmask */
#define FKT_DISABLE		0		/* for disabling probes with 1's in keymask */
#define FKT_ENABLE		1		/* for enabling probes with 1's in keymask */
#define FKT_SETMASK		2		/* for enabling 1's, disabling 0's in keymask */

/*	Simple keymasks */
#define FKT_KEYMASK0				0x00000001	/* IRQs, exceptions, syscalls */
#define FKT_KEYMASK1				0x00000002
#define FKT_KEYMASK2				0x00000004
#define FKT_KEYMASK3				0x00000008
#define FKT_KEYMASK4				0x00000010
#define FKT_KEYMASK5				0x00000020
#define FKT_KEYMASK6				0x00000040
#define FKT_KEYMASK7				0x00000080
#define FKT_KEYMASK8				0x00000100
#define FKT_KEYMASK9				0x00000200
#define FKT_KEYMASK10				0x00000400
#define FKT_KEYMASK11				0x00000800
#define FKT_KEYMASK12				0x00001000
#define FKT_KEYMASK13				0x00002000
#define FKT_KEYMASK14				0x00004000
#define FKT_KEYMASK15				0x00008000
#define FKT_KEYMASK16				0x00010000
#define FKT_KEYMASK17				0x00020000
#define FKT_KEYMASK18				0x00040000
#define FKT_KEYMASK19				0x00080000
#define FKT_KEYMASK20				0x00100000
#define FKT_KEYMASK21				0x00200000
#define FKT_KEYMASK22				0x00400000
#define FKT_KEYMASK23				0x00800000
#define FKT_KEYMASK24				0x01000000
#define FKT_KEYMASK25				0x02000000
#define FKT_KEYMASK26				0x04000000
#define FKT_KEYMASK27				0x08000000
#define FKT_KEYMASK28				0x10000000
#define FKT_KEYMASK29				0x20000000
#define FKT_KEYMASK30				0x40000000
/*****  FKT_KEYMASK31				0x80000000	looks negative! don't use! ****/
#define FKT_KEYMASKALL				0x7fffffff

/*	Assigned keymasks to separate probes into sets by protocol stack layers */
#define FKT_DRIVER_KEYMASK		FKT_KEYMASK1	/* driver (MAC) layer routines*/
#define FKT_NETWORK_KEYMASK		FKT_KEYMASK2	/* network (IP) layer routines*/
#define FKT_TRANSPORT_KEYMASK	FKT_KEYMASK3	/* transport (TCP/UDP) layer */
#define FKT_SOCKET_KEYMASK		FKT_KEYMASK4	/* socket interface routines */
#define FKT_SCSI_KEYMASK		FKT_KEYMASK5	/* scsi subsystem routines */
#define FKT_FILES_KEYMASK		FKT_KEYMASK6	/* file system routines */

#define FKT_APP_KEYMASK			FKT_KEYMASK7   /* test for read mchavan */

/*	Fixed parameters of the fkt coding scheme */
#define FKT_GENERIC_EXIT_OFFSET		0x100	/* exit this much above entry */
#define FKT_SYS_CALL_LIMIT_CODE		0x100	/* all system call nos. below this*/
#define FKT_TRAP_LIMIT_CODE			0x120	/* all trap nos. below this */
#define FKT_IRQ_TIMER				0x120	/* timer IRQ always 0 */
#define FKT_UNSHIFTED_LIMIT_CODE	0x200	/* all unshifted codes below this */

/*	Codes for fkt use */
#define FKT_SETUP_CODE				0x210
#define FKT_KEYCHANGE_CODE			0x211
#define FKT_RESET_CODE				0x212
#define FKT_CALIBRATE0_CODE			0x220
#define FKT_CALIBRATE1_CODE			0x221
#define FKT_CALIBRATE2_CODE			0x222
#define FKT_NEW_LWP_CODE                        0x230
#define FKT_RET_FROM_SYS_CALL_CODE	0x2ff	/* generic ret_from_sys_call */

/*	Codes for use with fkt items */
//#define	FKT_EXT2_WRITE_ENTRY_CODE 	0x304	/* enter ext2_file_write, P=3 */
//#define	FKT_EXT2_WRITE_EXIT_CODE 	0x404	/* exit ext2_file_write, P=0 */
#define FKT_LL_RW_BLOCK_ENTRY_CODE	0x303	/* enter ll_rw_block, P=3 */
#define FKT_LL_RW_BLOCK_EXIT_CODE	0x403	/* exit ll_rw_block, P=1 */
#define FKT_EXT2_GETBLK_ENTRY_CODE	0x305	/* enter ext2_getblk, P=3 */
#define FKT_EXT2_GETBLK_EXIT_CODE	0x405	/* exit  ext2_getblk, P=3 */
//#define FKT_INODE_GETBLK_ENTRY_CODE	0x306	/* enter inode_getblk, P=4 */
//#define FKT_INODE_GETBLK_EXIT_CODE	0x406	/* exit  inode_getblk, P=3 */
//#define FKT_BLOCK_GETBLK_ENTRY_CODE	0x307	/* enter block_getblk, P=5 */
//#define FKT_BLOCK_GETBLK_EXIT_CODE	0x407	/* exit  block_getblk, P=3 */
#define	FKT_SWITCH_TO_CODE			0x31a	/* switch_to in schedule, P=2 */

/* sadhna start */

/*	socket interface routines, 380->38f */
#define FKT_SOCKET_ENTRY_CODE		0x380	/* enter sys_socket, P=3          */
#define FKT_SOCKET_EXIT_CODE		0x480	/* exit sys_socket, P=1           */
#define FKT_BIND_ENTRY_CODE			0x381	/* enter sys_bind, P=1            */
#define FKT_BIND_EXIT_CODE			0x481	/* exit sys_bind, P=1             */
#define FKT_LISTEN_ENTRY_CODE		0x382	/* enter sys_listen, P=2          */
#define FKT_LISTEN_EXIT_CODE		0x482	/* exit sys_listen, P=1           */
#define FKT_ACCEPT_ENTRY_CODE		0x383	/* enter sys_accept, P=1          */
#define FKT_ACCEPT_EXIT_CODE		0x483	/* exit sys_accept, P=1           */
#define FKT_CONNECT_ENTRY_CODE		0x384	/* enter sys_connect, P=1         */
#define FKT_CONNECT_EXIT_CODE		0x484	/* exit sys_connect, P=1          */
#define FKT_SEND_ENTRY_CODE			0x385	/* enter sys_send, P=3            */
#define FKT_SEND_EXIT_CODE			0x485	/* exit sys_send, P=1             */
#define FKT_SENDTO_ENTRY_CODE		0x386	/* enter sys_sendto, P=3          */
#define FKT_SENDTO_EXIT_CODE		0x486	/* exit sys_sendto, P=1           */
#define FKT_RECVFROM_ENTRY_CODE		0x387	/* enter sys_recvfrom, P=3        */
#define FKT_RECVFROM_EXIT_CODE		0x487	/* exit sys_recvfrom, P=1         */
#define FKT_SHUTDOWN_ENTRY_CODE		0x388	/* enter sys_shutdown, P=2        */
#define FKT_SHUTDOWN_EXIT_CODE		0x488	/* exit sys_shutdown, P=1         */

/*	lamp routines, 390->39f */
#define FKT_LAMP_CREATE_ENTRY_CODE		0x390	/* enter lamp_create, P=0     */
#define FKT_LAMP_CREATE_EXIT_CODE		0x490	/* exit  lamp_create, P=1     */
#define FKT_LAMP_BIND_ENTRY_CODE		0x391	/* enter lamp_bind, P=0       */
#define FKT_LAMP_BIND_EXIT_CODE			0x491	/* exit  lamp_bind, P=1       */
#define FKT_LAMP_LISTEN_ENTRY_CODE		0x392	/* enter lamp_listen, P=0     */
#define FKT_LAMP_LISTEN_EXIT_CODE		0x492	/* exit  lamp_listen, P=1     */
#define FKT_LAMP_ACCEPT_ENTRY_CODE		0x393	/* enter lamp_accept, P=0     */
#define FKT_LAMP_ACCEPT_EXIT_CODE		0x493	/* exit  lamp_accept, P=1     */
#define FKT_LAMP_CONNECT_ENTRY_CODE		0x394	/* enter lamp_connect, P=0    */
#define FKT_LAMP_CONNECT_EXIT_CODE		0x494	/* exit  lamp_connect, P=1    */
#define FKT_LAMP_SENDMSG_ENTRY_CODE		0x396	/* enter lamp_sendmsg, P=0    */
#define FKT_LAMP_SENDMSG_EXIT_CODE		0x496	/* exit  lamp_sendmsg, P=1    */
#define FKT_LAMP_RECVMSG_ENTRY_CODE		0x397	/* enter lamp_recvmsg, P=0    */
#define FKT_LAMP_RECVMSG_EXIT_CODE		0x497	/* exit  lamp_recvmsg, P=1    */
#define FKT_LAMP_DATA_RCV_ENTRY_CODE	0x398	/* enter lamp_data_rcv, P=0   */
#define FKT_LAMP_DATA_RCV_EXIT_CODE		0x498	/* exit  lamp_data_rcv, P=1   */
#define FKT_LAMP_SIGNAL_RCV_ENTRY_CODE	0x399	/* enter lamp_signal_rcv, P=0 */
#define FKT_LAMP_SIGNAL_RCV_EXIT_CODE	0x499	/* exit  lamp_signal_rcv, P=1 */
#define FKT_LAMP_SHUTDOWN_ENTRY_CODE	0x39a	/* enter lamp_shutdown, P=0   */
#define FKT_LAMP_SHUTDOWN_EXIT_CODE		0x49a	/* exit  lamp_shutdown, P=1   */
#define FKT_LAMP_RECVMSG_SCHEDI_CODE	0x39b	/* lamp_recvmsg_sched_call P=0*/
#define FKT_LAMP_RECVMSG_SCHEDO_CODE	0x49b	/* lamp_recvmsg_sched_ret, P=0*/

/*	inet routines, 3a0->3af */
#define FKT_INET_CREATE_ENTRY_CODE		   0x3a0 /* enter inet_create, P=0    */
#define FKT_INET_CREATE_EXIT_CODE		   0x4a0 /* exit  inet_create, P=1    */
#define FKT_INET_BIND_ENTRY_CODE		   0x3a1 /* enter inet_bind, P=0      */
#define FKT_INET_BIND_EXIT_CODE			   0x4a1 /* exit  inet_bind, P=1      */
#define FKT_INET_LISTEN_ENTRY_CODE		   0x3a2 /* enter inet_listen, P=0    */
#define FKT_INET_LISTEN_EXIT_CODE		   0x4a2 /* exit  inet_listen, P=1    */
#define FKT_INET_ACCEPT_ENTRY_CODE		   0x3a3 /* enter inet_accept, P=0    */
#define FKT_INET_ACCEPT_EXIT_CODE		   0x4a3 /* exit  inet_accept, P=1    */
#define FKT_INET_STREAM_CONNECT_ENTRY_CODE 0x3a4 /* enter inet_connect, P=0   */
#define FKT_INET_STREAM_CONNECT_EXIT_CODE  0x4a4 /* exit  inet_connect, P=1   */
#define FKT_INET_SENDMSG_ENTRY_CODE		   0x3a6 /* enter inet_sendmsg, P=0   */
#define FKT_INET_SENDMSG_EXIT_CODE		   0x4a6 /* exit  inet_sendmsg, P=1   */
#define FKT_INET_RECVMSG_ENTRY_CODE		   0x3a7 /* enter inet_recvmsg, P=0   */
#define FKT_INET_RECVMSG_EXIT_CODE		   0x4a7 /* exit  inet_recvmsg, P=1   */
#define FKT_INET_SHUTDOWN_ENTRY_CODE	   0x3aa /* enter inet_shutdown, P=0  */
#define FKT_INET_SHUTDOWN_EXIT_CODE		   0x4aa /* exit  inet_shutdown, P=1  */

/*	net routines, 3b0->3bf */
#define FKT_NET_RX_ACTION_ENTRY_CODE		0x3b0 /* enter net_rx_action, P=0 */
#define FKT_NET_RX_ACTION_EXIT_CODE			0x4b0 /* exit net_rx_action, P=0  */
//#define FKT_NET_BH_QUEUE_ENTRY_CODE       0x3b1 /* enter net_bh_queue       */
//#define FKT_NET_BH_QUEUE_EXIT_CODE        0x4b1 /* exit net_bh_queue        */
#define FKT_NETIF_RX_ENTRY_CODE             0x3b2 /* enter netif_rx           */
#define FKT_NETIF_RX_EXIT_CODE              0X4b2 /* exit netif_rx            */
#define FKT_NET_RX_DEQUEUE_ENTRY_CODE       0x3b3 /* enter net_rx_dequeue     */
#define FKT_NET_RX_DEQUEUE_EXIT_CODE        0x4b3 /* exit net_rx_dequeue      */

/* sadhna end */

/* miru start */

/*	tcp routines, 320->32f and 330->33f */
#define FKT_TCP_SENDMSG_ENTRY_CODE          0x320 /* enter tcp_sendmsg.       */
#define FKT_TCP_SENDMSG_EXIT_CODE           0x420 /* exit tcp_sendmsg         */
#define FKT_TCP_SEND_SKB_ENTRY_CODE         0x321 /* enter tcp_send_skb.      */
#define FKT_TCP_SEND_SKB_EXIT_CODE          0x421 /* exit tcp_send_skb.       */
#define FKT_TCP_TRANSMIT_SKB_ENTRY_CODE     0x322 /* enter tcp_transmit_skb.  */
#define FKT_TCP_TRANSMIT_SKB_EXIT_CODE      0x422 /* exit tcp_transmit_skb.   */
#define FKT_TCP_V4_RCV_ENTRY_CODE			0x329 /* enter tcp_v4_rcv, P=0    */
#define FKT_TCP_V4_RCV_EXIT_CODE			0x429 /* exit  tcp_v4_rcv, P=1    */
//#define FKT_TCP_RECVMSG_SCHEDI_CODE		0x32b /*tcp_recvmsg_sched_call P=0*/
//#define FKT_TCP_RECVMSG_SCHEDO_CODE		0x42b /*tcp_recvmsg_sched_ret, P=0*/
#define FKT_TCP_RCV_ESTABLISHED_ENTRY_CODE  0x32d /* enter tcp_rcv_established*/
#define FKT_TCP_RCV_ESTABLISHED_EXIT_CODE   0x42d /* exit tcp_rcv_established */
#define FKT_TCP_DATA_ENTRY_CODE             0x32e /* enter tcp_data           */
#define FKT_TCP_DATA_EXIT_CODE              0x42e /* exit tcp_data            */
#define FKT_TCP_ACK_ENTRY_CODE              0x32f /* enter tcp_ack            */
#define FKT_TCP_ACK_EXIT_CODE               0x42f /* exit tcp_ack             */

#define FKT_TCP_RECVMSG_ENTRY_CODE          0x330 /* enter tcp_recvmsg        */
#define FKT_TCP_RECVMSG_EXIT_CODE           0x430 /* exit tcp_recvmsg         */
//#define FKT_TCP_V4_RCV_CSUM_ENTRY_CODE    0x331 /* enter tcp_v4_rcv_csum    */
//#define FKT_TCP_V4_RCV_CSUM_EXIT_CODE     0x431 /* exit tcp_v4_rcv_csum     */
#define FKT_TCP_RECVMSG_OK_ENTRY_CODE       0x332 /* enter tcp_recvmsg_ok     */
#define FKT_TCP_RECVMSG_OK_EXIT_CODE        0x432 /* exit tcp_recvmsg_ok      */
#define FKT_TCP_SEND_ACK_ENTRY_CODE         0x337 /* enter tcp_send_ack       */
#define FKT_TCP_SEND_ACK_EXIT_CODE          0x437 /* exit tcp_send_ack        */
//#define FKT_TCP_V4_RCV_HW_ENTRY_CODE      0x338 /* enter hw_csum            */
//#define FKT_TCP_V4_RCV_HW_EXIT_CODE       0x438 /* exit hw_csum             */
//#define FKT_TCP_V4_RCV_NONE_ENTRY_CODE    0x339 /* enter no_csum            */
//#define FKT_TCP_V4_RCV_NONE_EXIT_CODE     0x439 /* exit no_csum             */
#define FKT_WAIT_FOR_TCP_MEMORY_ENTRY_CODE  0x33a /*enter wait_for_tcp_memory */
#define FKT_WAIT_FOR_TCP_MEMORY_EXIT_CODE   0x43a /*exit wait_for_tcp_memory  */
//#define FKT_TCP_RECVMSG_CSUM_ENTRY_CODE   0x33b /* enter tcp_recvmsg_csum   */
//#define FKT_TCP_RECVMSG_CSUM_EXIT_CODE    0x43b /* exit tcp_recvmsg_csum    */
//#define FKT_TCP_RECVMSG_CSUM2_ENTRY_CODE  0x33c /* enter tcp_recvmsg_csum2  */
//#define FKT_TCP_RECVMSG_CSUM2_EXIT_CODE   0x43c /* exit tcp_recvmsg_csum2   */
#define FKT_TCP_SYNC_MSS_ENTRY_CODE         0x33d /* enter tcp_sync_mss       */
#define FKT_TCP_SYNC_MSS_EXIT_CODE          0x43d /* exit tcp_sync_mss        */

/*	udp routines, 340->34f */
#define FKT_UDP_SENDMSG_ENTRY_CODE          0x341 /* enter udp_sendmsg        */
#define FKT_UDP_SENDMSG_EXIT_CODE           0x441 /* exit udp_sendmsg         */
#define FKT_UDP_QUEUE_RCV_SKB_ENTRY_CODE    0x342 /* enter udp_queue_rcv_skb  */
#define FKT_UDP_QUEUE_RCV_SKB_EXIT_CODE     0x442 /* exit udp_queue_rcv_skb   */
#define FKT_UDP_GETFRAG_ENTRY_CODE          0x345 /* enter udp_getfrag        */
#define FKT_UDP_GETFRAG_EXIT_CODE           0x445 /* exit udp_getfrag         */
#define FKT_UDP_RECVMSG_ENTRY_CODE          0x34e /* enter udp_recvmsg        */
#define FKT_UDP_RECVMSG_EXIT_CODE           0x44e /* exit udp_recvmsg         */
#define FKT_UDP_RCV_ENTRY_CODE              0x34f /* enter udp_rcv            */
#define FKT_UDP_RCV_EXIT_CODE               0x44f /* exit udp_rcv             */

/*	ip routines, 3c0->3cf */
#define FKT_IP_RCV_ENTRY_CODE				0x3c0 /* enter ip_rcv, P=0        */
#define FKT_IP_RCV_EXIT_CODE				0x4c0 /* exit  ip_rcv, P=1        */
#define FKT_IP_QUEUE_XMIT_ENTRY_CODE        0x3c1 /* enter ip_queue_xmit      */
#define FKT_IP_QUEUE_XMIT_EXIT_CODE         0x4c1 /* exit ip_queue_xmit       */
#define FKT_IP_BUILD_XMIT_ENTRY_CODE        0x3c2 /* enter ip_build_xmit      */
#define FKT_IP_BUILD_XMIT_EXIT_CODE         0x4c2 /* exit ip_build_xmit       */
#define FKT_IP_LOCAL_DELIVER_ENTRY_CODE     0x3c3 /* enter ip_local_deliver   */
#define FKT_IP_LOCAL_DELIVER_EXIT_CODE      0x4c3 /* exit ip_local_deliver    */
#define FKT_IP_DEFRAG_ENTRY_CODE            0x3c4 /* enter ip_defrag          */
#define FKT_IP_DEFRAG_EXIT_CODE             0x4c4 /* exit ip_defrag           */
#define FKT_IP_FRAG_REASM_ENTRY_CODE        0x3c8 /* enter ip_frag_reasm      */
#define FKT_IP_FRAG_REASM_EXIT_CODE         0x4c8 /* exit ip_frag_reasm       */

/*	ethernet cards, 350->35f and 360->36f */
#define FKT_TULIP_START_XMIT_ENTRY_CODE     0x350 /* enter tulip_start_xmit   */
#define FKT_TULIP_START_XMIT_EXIT_CODE      0x450 /* exit tulip_start_xmit    */
#define FKT_TULIP_INTERRUPT_ENTRY_CODE      0x351 /* enter tulip_interrupt    */
#define FKT_TULIP_INTERRUPT_EXIT_CODE       0x451 /* exit tulip_interrupt     */
#define FKT_TULIP_RX_ENTRY_CODE             0x352 /* enter tulip_rx           */
#define FKT_TULIP_RX_EXIT_CODE              0x452 /* exit tulip_rx            */
#define FKT_TULIP_TX_ENTRY_CODE             0x353 /* enter tulip_tx           */
#define FKT_TULIP_TX_EXIT_CODE              0x453 /* exit tulip_tx_exit       */

#define FKT_VORTEX_INTERRUPT_ENTRY_CODE     0x358 /* enter vortex_interrupt   */
#define FKT_VORTEX_INTERRUPT_EXIT_CODE      0x458 /* exit vortex_interrupt    */
#define FKT_BOOMERANG_INTERRUPT_ENTRY_CODE  0x359 /*enter boomerang_interrupt */
#define FKT_BOOMERANG_INTERRUPT_EXIT_CODE   0x459 /* exit boomerang_interrupt */
#define FKT_BOOMERANG_START_XMIT_ENTRY_CODE 0x35a /*enter boomerang_start_xmit*/
#define FKT_BOOMERANG_START_XMIT_EXIT_CODE  0x45a /*exit boomerang_start_xmit */
#define FKT_BOOMERANG_RX_ENTRY_CODE         0x35b /* enter boomerang_rx       */
#define FKT_BOOMERANG_RX_EXIT_CODE          0x45b /* exit boomerang_rx        */
#define FKT_BOOMERANG_TX_ENTRY_CODE         0x35c /* enter boomerang_tx       */
#define FKT_BOOMERANG_TX_EXIT_CODE          0x45c /* exit boomerang_tx_exit   */

#define FKT_ACE_INTERRUPT_ENTRY_CODE        0x360  /* enter ace_interrupt     */
#define FKT_ACE_INTERRUPT_EXIT_CODE         0x460  /* exit ace_interrupt      */
#define FKT_ACE_RX_INT_ENTRY_CODE           0x361  /* enter ace_rx_int        */
#define FKT_ACE_RX_INT_EXIT_CODE            0x461  /* exit ace_rx_int         */
#define FKT_ACE_TX_INT_ENTRY_CODE           0x362  /* enter ace_tx_int        */
#define FKT_ACE_TX_INT_EXIT_CODE            0x462  /* exit ace_tx_int         */
#define FKT_ACE_START_XMIT_ENTRY_CODE       0x363  /* enter ace_start_xmit    */
#define FKT_ACE_START_XMIT_EXIT_CODE        0x463  /* exit ace_start_xmit     */

#define FKT_DO_SOFTIRQ_ENTRY_CODE           0x36f /* enter do_softirq         */
#define FKT_DO_SOFTIRQ_EXIT_CODE            0x46f /* exit do_softirq          */

/* miru end */


/*	the sys_xxx functions -- should be 500->5xx */

/* achadda start */
#define FKT_READ_ENTRY_CODE		0x503	/* enter read == 3, P=3 */
#define FKT_READ_EXIT_CODE		0x603	/*  exit read == 3, P=1 */

/* vs start */
#define FKT_WRITE_ENTRY_CODE	0x504	/* enter sys_write == 4, P=3 */
#define FKT_WRITE_EXIT_CODE		0x604	/*  exit sys_write == 4, P=1 */

/* rdr */
#define FKT_LSEEK_ENTRY_CODE	0x513	/* enter lseek == 19, P=3 */
#define FKT_LSEEK_EXIT_CODE		0x613	/*  exit lseek == 19, P=1 */

/* vs start */
#define FKT_FSYNC_ENTRY_CODE	0x576	/* enter sys_fsync == 118, P=1 */
#define FKT_FSYNC_EXIT_CODE		0x676	/*  exit sys_fsync == 118, P=1 */
#define FKT_NANOSLEEP_ENTRY_CODE 0x5a2	/* enter sys_nanosleep == 162, P=0 */
#define FKT_NANOSLEEP_EXIT_CODE	0x6a2	/*  exit sys_nanosleep == 162, P=1 */


/* rdr */
//#define FKT_DO_SD_REQUEST_ENTRY_CODE    0x710
/* enter do_sd_request, P=0 */
//#define FKT_DO_SD_REQUEST_EXIT_CODE     0x810
/* exit do_sd_request, P=0 */
//#define FKT_REQUEUE_SD_REQUEST_ENTRY_CODE    0x711
/* enter requeue_sd_request, P=0 */
//#define FKT_REQUEUE_SD_REQUEST_EXIT_CODE    0x811
/* exit requeue_sd_request, P=2 */
#define FKT_SD_OPEN_ENTRY_CODE    0x712
/* enter sd_open, P=0 */
#define FKT_SD_OPEN_EXIT_CODE     0x812
/* exit sd_open, P=1 */
#define FKT_SD_RELEASE_ENTRY_CODE    0x713
/* enter sd_release, P=0 */
#define FKT_SD_RELEASE_EXIT_CODE     0x813
/* exit sd_release, P=0 */
#define FKT_SD_IOCTL_ENTRY_CODE    0x714
/* enter sd_ioctl, P=0 */
#define FKT_SD_IOCTL_EXIT_CODE     0x814
/* exit sd_ioctl, P=1 */
#define FKT_SCSI_DO_CMD_ENTRY_CODE    0x721
/* enter scsi_do_cmd P=0 */
#define FKT_SCSI_DO_CMD_EXIT_CODE    0x821
/* exit scsi_do_cmd P=0 */
#define FKT_SCSI_IOCTL_ENTRY_CODE    0x722
/* enter scsi_do_cmd P=0 */
#define FKT_SCSI_IOCTL_EXIT_CODE    0x822
/* exit scsi_do_cmd P=0 */
#define FKT_SCSI_DONE_ENTRY_CODE    0x723
/* enter scsi_done P=1 */
#define FKT_SCSI_DONE_EXIT_CODE    0x823
/* exit scsi_done P=1 */
#define FKT_SCSI_DISPATCH_CMD_ENTRY_CODE    0x724
/* enter scsi_dispatch_cmd P=0 */
#define FKT_SCSI_DISPATCH_CMD_EXIT_CODE    0x824
/* exit scsi_dispatch_cmd P=1 */
#define FKT_SCSI_RETRY_COMMAND_ENTRY_CODE    0x725
/* enter scsi_retry_command P=0 */
#define FKT_SCSI_RETRY_COMMAND_EXIT_CODE    0x825
/* exit scsi_retry_command P=1 */
#define FKT_SG_OPEN_ENTRY_CODE    0x731
/* enter sg_open P=0 */
#define FKT_SG_OPEN_EXIT_CODE    0x831
/* exit sg_open P=0 */
#define FKT_SG_RELEASE_ENTRY_CODE    0x732
/* enter sg_release P=0 */
#define FKT_SG_RELEASE_EXIT_CODE    0x832
/* exit sg_release P=0 */
#define FKT_SG_IOCTL_ENTRY_CODE    0x733
/* enter sg_ioctl P=1 */
#define FKT_SG_IOCTL_EXIT_CODE    0x833
/* exit sg_ioctl P=1 */
#define FKT_SG_READ_ENTRY_CODE    0x734
/* enter sg_read P=0 */
#define FKT_SG_READ_EXIT_CODE    0x834
/* exit sg_read P=1 */
#define FKT_SG_NEW_READ_ENTRY_CODE    0x735
/* enter sg_new_read P=0 */
#define FKT_SG_NEW_READ_EXIT_CODE    0x835
/* exit sg_new_read P=1 */
#define FKT_SG_WRITE_ENTRY_CODE    0x736
/* enter sg_write P=0 */
#define FKT_SG_WRITE_EXIT_CODE    0x836
/* exit sg_write P=1 */
#define FKT_SG_NEW_WRITE_ENTRY_CODE    0x737
/* enter sg_new_write P=0 */
#define FKT_SG_NEW_WRITE_EXIT_CODE    0x837
/* exit sg_new_write P=1 */
#define FKT_SG_GET_RQ_MARK_ENTRY_CODE    0x738
/* enter sg_get_rq_mark P=0 */
#define FKT_SG_GET_RQ_MARK_EXIT_CODE    0x838
/* exit sg_get_rq_mark P=1 */
#define FKT_SG_CMD_DONE_BH_ENTRY_CODE    0x739
/* enter sg_cmd_done_bh P=0 */
#define FKT_SG_CMD_DONE_BH_EXIT_CODE    0x839
/* exit sg_cmd_done_bh P=0 */
#define FKT_SCSI_SEP_IOCTL_ENTRY_CODE    0x743
/* enter sg_ioctl P=1 */
#define FKT_SCSI_SEP_IOCTL_EXIT_CODE    0x843
/* exit sg_ioctl P=1 */
#define FKT_SCSI_SEP_QUEUECOMMAND_ENTRY_CODE    0x744
/* enter sg_queuecommand P=0 */
#define FKT_SCSI_SEP_QUEUECOMMAND_EXIT_CODE    0x844
/* exit sg_queuecommand P=1 */

/* vs */
//#define FKT_SEAGATE_INTERNAL_CMD_ENTRY_CODE     0x751
/* entry in seagate's internal_ommand P=0 */
//#define FKT_SEAGATE_INTERNAL_CMD_EXIT_CODE     0x851
/* exit in seagate's internal_ommand P=0 */
//#define FKT_QLOGIC_QCMND_ENTRY_CODE        0x752
/* entry in qlogic's queuecommand P=0 */
//#define FKT_QLOGIC_QCMND_EXIT_CODE        0x852
/* exit in qlogic's queuecommand P=0 */
//#define FKT_ISP_RET_STATUS_ENTRY_CODE        0x761
/* entry in qlogic's return_status P=0 */
//#define FKT_ISP_RET_STATUS_EXIT_CODE        0x861
/* exit in qlogic's return_status P=1 */
//#define AM53C974_QCMD_ENTRY_CODE	    0x771
/* entry in am53c974 adapter's qcmd P=0 */
//#define AM53C974_QCMD_EXIT_CODE	    0x871
/* exit in am53c974 adapter's qcmd P=0 */
//#define REQUE_DEVM_DEV_BLK_CODE 0x901
/* code in requeue_sd_request P=3 */
//#define REQUE_BLK_CODE 0x911
/* code in requeue_sd_request P=1 */

/* vs end */

#endif
