#include <marcel.h>
#include <fcntl.h>
#include <errno.h>

struct ma_supervisor_msg {
	uint32_t cmd;
	uint32_t arg;
};

enum ma_supervisor_cmd {
	ma_sc_enable_vp	= 1,
	ma_sc_disable_vp	= 2,
	ma_sc_enable_vp_list	= 3,
	ma_sc_disable_vp_list	= 4,
	ma_sc_enable_vp_list_item	= 5,
	ma_sc_disable_vp_list_item	= 6,
};

static int ma_supervisor_fd;
static int ma_supervisor_neighbour_fd;
static marcel_t ma_supervisor_thread;

static
int
ma_supervisor_read_command (struct ma_supervisor_msg *p_msg) {
	int r;
	int _errno;
again:
	marcel_extlib_protect ();
	r	= read (ma_supervisor_fd, p_msg, sizeof (*p_msg));
	_errno	= errno;
	marcel_extlib_unprotect ();
	if (r < 0) {
		if (_errno == EINTR)
			goto again;
		if (_errno == EWOULDBLOCK || _errno == EAGAIN || _errno == 0) {
			marcel_yield();
			goto again;
		}
		marcel_fprintf (stderr, "read: r = %d, _errno = %d, %s\n", r, _errno, strerror(_errno));
		exit (1);
	} else if (r == 0) {
		return 0;
	} else if (r < sizeof (*p_msg)) {
		marcel_fprintf (stderr, "ma_supervisor_read_command: command message truncated\n");
		exit (1);
	}

	return 1;
}

static
void
ma_supervisor_write_command (struct ma_supervisor_msg *p_msg) {
	int r;
again:
	r	= marcel_write (ma_supervisor_neighbour_fd, p_msg, sizeof (*p_msg));
	if (r <= 0) {
		if (errno == EINTR)
			goto again;
		perror ("marcel_write");
		exit (1);
	} else if (r < sizeof (*p_msg)) {
		marcel_fprintf (stderr, "ma_supervisor_write_command: command message truncated\n");
		exit (1);
	}
}

static void *
ma_supervisor (void *_args TBX_UNUSED) {
	struct ma_supervisor_msg msg;

	marcel_fprintf (stderr, "ma_supervisor: -->\n");
	for (;;) {
		if (!ma_supervisor_read_command (&msg))
			break;
		marcel_fprintf (stderr, "ma_supervisor: - new command %u, arg %u\n", msg.cmd, msg.arg);
		switch ((enum ma_supervisor_cmd) msg.cmd) {
			case ma_sc_enable_vp: {
				marcel_vpset_t vpset;
				if (msg.arg > MA_NR_VPS) {
					marcel_fprintf (stderr, "ma_supervisor: enable_vp - invalid arg %u\n", (unsigned) msg.arg);
					exit (1);
				}
				marcel_vpset_zero (&vpset);
				marcel_vpset_set (&vpset, msg.arg);
				marcel_enable_vps (&vpset);
			} break;

			case ma_sc_disable_vp: {
				marcel_vpset_t vpset;
				if (msg.arg > MA_NR_VPS) {
					marcel_fprintf (stderr, "ma_supervisor: disable_vp - invalid arg %u\n", (unsigned) msg.arg);
					exit (1);
				}
				marcel_vpset_zero (&vpset);
				marcel_vpset_set (&vpset, msg.arg);
				marcel_disable_vps (&vpset);
			} break;

			case ma_sc_enable_vp_list: {
				int count = msg.arg;
				int i;
				marcel_vpset_t vpset;
				marcel_vpset_zero (&vpset);
				for (i = 0; i < count; i++) {
					if (!ma_supervisor_read_command (&msg)) {
						marcel_fprintf (stderr, "ma_supervisor: fd closed unexpectedly\n");
						exit (1);
					}
					if (msg.cmd != ma_sc_enable_vp_list_item) {
						marcel_fprintf (stderr, "ma_supervisor: enable_vp_list - unexpected sub command %d\n", (int) msg.cmd);
						exit (1);
					}
					if (msg.arg > MA_NR_VPS) {
						marcel_fprintf (stderr, "ma_supervisor: enable_vp_list_item - invalid arg %u\n", (unsigned) msg.arg);
						exit (1);
					}
					marcel_vpset_set (&vpset, msg.arg);
				}
				marcel_enable_vps (&vpset);
			} break;

			case ma_sc_disable_vp_list: {
				int count = msg.arg;
				int i;
				marcel_vpset_t vpset;
				marcel_vpset_zero (&vpset);
				for (i = 0; i < count; i++) {
					if (!ma_supervisor_read_command (&msg)) {
						marcel_fprintf (stderr, "ma_supervisor: fd closed unexpectedly\n");
						exit (1);
					}
					if (msg.cmd != ma_sc_disable_vp_list_item) {
						marcel_fprintf (stderr, "ma_supervisor: disable_vp_list - unexpected sub command %d\n", (int) msg.cmd);
						exit (1);
					}
					if (msg.arg > MA_NR_VPS) {
						marcel_fprintf (stderr, "ma_supervisor: disable_vp_list_item - invalid arg %u\n", (unsigned) msg.arg);
						exit (1);
					}
					marcel_vpset_set (&vpset, msg.arg);
				}
				marcel_disable_vps (&vpset);
			} break;

			default: {
				 marcel_fprintf (stderr, "ma_supervisor: unknown command code %d\n", (int)msg.cmd);
				 exit (1);
			}
		}
	}

	marcel_fprintf (stderr, "ma_supervisor: <--\n");
	return NULL;
}

void
marcel_supervisor_enable_nbour_vp (int vpnum) {
	struct ma_supervisor_msg msg;

	msg.cmd	= ma_sc_enable_vp;
	msg.arg	= vpnum;

	ma_supervisor_write_command (&msg);
}

void
marcel_supervisor_disable_nbour_vp (int vpnum) {
	struct ma_supervisor_msg msg;

	msg.cmd	= ma_sc_disable_vp;
	msg.arg	= vpnum;

	ma_supervisor_write_command (&msg);
}

void
marcel_supervisor_enable_nbour_vpset (const marcel_vpset_t * vpset) {
	struct ma_supervisor_msg msg;
	unsigned vp;

	msg.cmd	= ma_sc_enable_vp_list;
	msg.arg	= marcel_vpset_weight (vpset);
	ma_supervisor_write_command (&msg);

	marcel_vpset_foreach_begin(vp, vpset)
		msg.cmd	= ma_sc_enable_vp_list_item;
		msg.arg	= vp;
		ma_supervisor_write_command (&msg);
	marcel_vpset_foreach_end()
}

void
marcel_supervisor_disable_nbour_vpset (const marcel_vpset_t * vpset) {
	struct ma_supervisor_msg msg;
	unsigned vp;

	msg.cmd	= ma_sc_disable_vp_list;
	msg.arg	= marcel_vpset_weight (vpset);
	ma_supervisor_write_command (&msg);

	marcel_vpset_foreach_begin(vp, vpset)
		msg.cmd	= ma_sc_disable_vp_list_item;
		msg.arg	= vp;
		ma_supervisor_write_command (&msg);
	marcel_vpset_foreach_end()
}

void
marcel_supervisor_init (void) {
	char *filename;
	char *neighbour_filename;
	marcel_attr_t attr;

	marcel_fprintf (stderr, "marcel_supervisor_init: -->\n");
	filename = getenv ("MARCEL_SUPERVISOR_FIFO");
	if (!filename) {
		marcel_fprintf (stderr, "MARCEL_SUPERVISOR_FIFO var not set\n");
		exit (1);
	}

	neighbour_filename = getenv ("MARCEL_SUPERVISOR_NBOUR_FIFO");

	marcel_extlib_protect ();
	marcel_fprintf (stderr, "marcel_supervisor_init: - input fifo filename = %s\n", filename);
	ma_supervisor_fd = open (filename, O_RDONLY|O_NONBLOCK);
	if (ma_supervisor_fd < 0) {
		perror ("open");
		exit (1);
	}
	marcel_fprintf (stderr, "marcel_supervisor_init: - input fifo opened\n");
	if (neighbour_filename) {
		marcel_fprintf (stderr, "marcel_supervisor_init: - output fifo filename = %s\n", filename);
		ma_supervisor_neighbour_fd = open (neighbour_filename, O_WRONLY|O_NONBLOCK);
		if (ma_supervisor_neighbour_fd < 0) {
			perror ("open");
			exit (1);
		}
		marcel_fprintf (stderr, "marcel_supervisor_init: - output fifo opened\n");
	}
	marcel_extlib_unprotect ();

	marcel_attr_init (&attr);
	marcel_attr_setdetachstate (&attr, tbx_true);
	marcel_fprintf (stderr, "marcel_supervisor_init: - launching supervisor thread\n");
	marcel_create (&ma_supervisor_thread, &attr, ma_supervisor, NULL);
	marcel_fprintf (stderr, "marcel_supervisor_init: <--\n");
}

