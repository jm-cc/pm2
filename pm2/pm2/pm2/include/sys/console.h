

/*******************************************************/

BEGIN_LRPC_LIST
        LRPC_CONSOLE_THREADS,
        LRPC_CONSOLE_PING,
	LRPC_CONSOLE_USER,
	LRPC_CONSOLE_MIGRATE,
	LRPC_CONSOLE_FREEZE
END_LRPC_LIST


/*******************************************************/
LRPC_DECL_REQ(LRPC_CONSOLE_MIGRATE, int destination; char *thread_id; long thread;);
LRPC_DECL_RES(LRPC_CONSOLE_MIGRATE,);


/*******************************************************/
LRPC_DECL_REQ(LRPC_CONSOLE_FREEZE, char *thread_id; long thread;);
LRPC_DECL_RES(LRPC_CONSOLE_FREEZE,);


/*******************************************************/

typedef char _user_args_tab[1024];

LRPC_DECL_REQ(LRPC_CONSOLE_USER, _user_args_tab tab;);
LRPC_DECL_RES(LRPC_CONSOLE_USER, _user_args_tab tab; int tid;);
 

/*******************************************************/
	
LRPC_DECL_REQ(LRPC_CONSOLE_PING, int version;);
LRPC_DECL_RES(LRPC_CONSOLE_PING, int version; int tid;);
 

/*******************************************************/

LRPC_DECL_RES(LRPC_CONSOLE_THREADS,);
LRPC_DECL_REQ(LRPC_CONSOLE_THREADS,);

/*******************************************************/


void pm2_console_init_rpc(void);

