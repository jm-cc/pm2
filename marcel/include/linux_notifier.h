/*
 *	Routines to manage notifier chains for passing status changes to any
 *	interested routines. We need this instead of hard coded call lists so
 *	that modules can poke their nose into the innards. The network devices
 *	needed them so here they are for the rest of you.
 *
 *				Alan Cox <Alan.Cox@linux.org>
 */
 
#section types
struct ma_notifier_block;
struct ma_notifier_chain;

#section marcel_structures
struct ma_notifier_block
{
	int (*notifier_call)(struct ma_notifier_block *self, unsigned long, void *);
	struct ma_notifier_block *next;
	int priority;
	const char* name;
	int nb_actions;
	const char **actions_name;
};

struct ma_notifier_chain
{
	struct ma_notifier_block *chain;
	const char* name;
};

#section marcel_macros
#define MA_DEFINE_NOTIFIER_BLOCK(var, func, help) MA_DEFINE_NOTIFIER_BLOCK_PRIO(var, func, 0, help)
#define MA_DEFINE_NOTIFIER_BLOCK_PRIO(var, func, prio, help) \
  MA_DEFINE_NOTIFIER_BLOCK_INTERNAL(var, func, prio, help, 0, NULL)
#define MA_DEFINE_NOTIFIER_BLOCK_INTERNAL(var, func, prio, help, nb, helps) \
  struct ma_notifier_block var = { \
        .notifier_call = (func), \
        .next           = NULL, \
        .priority       = prio, \
        .name           = help, \
        .nb_actions     = nb, \
        .actions_name   = helps, \
  }

#define MA_DEFINE_NOTIFIER_CHAIN(var, help) \
  struct ma_notifier_chain var = { \
        .chain = NULL, \
        .name  = help, \
  }

#section marcel_functions
#depend "tbx_compiler.h"
extern int ma_notifier_chain_register(struct ma_notifier_chain *c, struct ma_notifier_block *n);
extern int ma_notifier_chain_unregister(struct ma_notifier_chain *c, struct ma_notifier_block *n);
extern int ma_notifier_call_chain(struct ma_notifier_chain *c, unsigned long val, void *v);

#section marcel_macros
#define MA_NOTIFY_DONE		0x0000		/* Don't care */
#define MA_NOTIFY_OK		0x0001		/* Suits me */
#define MA_NOTIFY_STOP_MASK	0x8000		/* Don't call further */
#define MA_NOTIFY_BAD		(MA_NOTIFY_STOP_MASK|0x0002)	/* Bad/Veto action	*/

/*
 *	Declared notifiers so far. I can imagine quite a few more chains
 *	over time (eg laptop power reset chains, reboot chain (to clean 
 *	device units up), device [un]mount chain, module load/unload chain,
 *	low memory chain, screenblank chain (for plug in modular screenblankers) 
 *	VC switch chains (for loadable kernel svgalib VC switch helpers) etc...
 */
 
#define MA_LWP_ONLINE		0x0002 /* LWP (ma_lwp_t)v is up */
#define MA_LWP_UP_PREPARE	0x0003 /* LWP (ma_lwp_t)v coming up */
#define MA_LWP_UP_CANCELED	0x0004 /* LWP (ma_lwp_t)v NOT coming up */
#define MA_LWP_OFFLINE		0x0005 /* LWP (ma_lwp_t)v offline (still scheduling) */
#define MA_LWP_DEAD		0x0006 /* LWP (ma_lwp_t)v dead */


