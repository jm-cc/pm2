/* #define DEBUG */
#include "pm2.h"
#include "madeleine.h"
#include "sys/netserver.h"
#include "isomalloc.h"
#include "pm2_timing.h"

#define MAX_NETSERVERS   128

static marcel_t _recv_pid[MAX_NETSERVERS];
static unsigned nb_netservers = 0;

static volatile boolean finished = FALSE;

extern pm2_rawrpc_func_t pm2_rawrpc_func[];

static void netserver_raw_rpc(int num)
{
  marcel_sem_t sem;

  marcel_sem_init(&sem, 0);
  marcel_setspecific(_pm2_mad_key, &sem);

  (*pm2_rawrpc_func[num])();

  marcel_sem_P(&sem);
}

void _netserver_term_func(void *arg)
{
#ifdef DEBUG
  tfprintf(stderr, "netserver is ending.\n");
#endif

  slot_free(NULL, marcel_stackbase((marcel_t)arg));
}

#ifdef MAD2
void pm2_send_stop_server(int i);
extern int pm2_zero_halt;
#endif

static any_t netserver(any_t arg)
{
  unsigned tag;
  
  marcel_cleanup_push(_netserver_term_func, marcel_self());

  while(!finished) {
#ifdef MAD2
    mad_receive((p_mad_channel_t)arg);
#else
    mad_receive();
#endif
    pm2_unpack_int(SEND_SAFER, RECV_EXPRESS, &tag, 1);
    if(tag >= NETSERVER_RAW_RPC)
      netserver_raw_rpc(tag - NETSERVER_RAW_RPC);
    else {
      switch(tag) {
      case NETSERVER_END : {
#ifdef MAD2
	mad_recvbuf_receive();
	if ((__pm2_self==0) && !pm2_zero_halt) {
	  mdebug("Netserver handling NETSERVER_END node 0 first...");
	  pm2_send_stop_server(1);  
	  pm2_zero_halt=TRUE;
	} else {
	  mdebug("Netserver handling NETSERVER_END\n");
	  finished = TRUE;
	}
#else
	finished = TRUE;
#endif
	break;
      }
      default : {
	fprintf(stderr, "NETSERVER ERROR: %d is not a valid tag.\n", tag);
      }
      }
    }
  }
#ifdef MINIMAL_PREEMPTION
   marcel_setspecialthread(NULL);
#endif

  return NULL;
}

#ifdef MAD2
void netserver_start(p_mad_channel_t channel, unsigned priority)
#else
void netserver_start(unsigned priority)
#endif
{
  marcel_attr_t attr;
  unsigned granted;
#ifndef MAD2
  void *channel = NULL;
#endif

  marcel_attr_init(&attr);
  marcel_attr_setprio(&attr, priority);
  marcel_attr_setstackaddr(&attr,
			   slot_general_alloc(NULL, 0, &granted, NULL, NULL));
  marcel_attr_setstacksize(&attr, granted);

  marcel_create(&_recv_pid[nb_netservers], &attr,
		netserver, (any_t)channel);
  nb_netservers++;

#ifdef MINIMAL_PREEMPTION
  marcel_setspecialthread(_recv_pid[0]);
#endif
}

void netserver_wait_end(void)
{
  int i;

  for(i=0; i<nb_netservers; i++) {
    mdebug("netserveur waiting for %p\n", _recv_pid[i]);
    marcel_join(_recv_pid[i], NULL);
    mdebug("netserveur wait for %p done\n", _recv_pid[i]);
  }
}

void netserver_stop(void)
{
#ifndef MAD2
  int i;

  for(i=0; i<nb_netservers; i++) {
    if(_recv_pid[i] == marcel_self())
      finished = TRUE;
    else {
      mdebug("netserveur killing %p\n", _recv_pid[i]);
      marcel_cancel(_recv_pid[i]);
    }
  }
#endif
}
