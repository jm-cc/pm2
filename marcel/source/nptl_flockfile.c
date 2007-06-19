/* lockfile - Handle locking and unlocking of stream.
   Copyright (C) 1996, 1998, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "marcel.h" //VD:
#ifdef MA__LIBPTHREAD //VD:
#include <errno.h> //VD:

/* The locking here is very inexpensive, even for inlining.  */
#define _IO_lock_inexpensive	1

/* On doit rester en dessous de cette taille pour un lock rÃ©cursif :-((( */
//typedef struct { int lock; int cnt; void *owner; } _lpt_IO_lock_t;

#include "marcel_fastlock.h"
typedef struct {
	union {
		struct {
			int spin_cnt;
			int owner;
		};
		long spin; /* Attention : le bit 0 de spin doit Ãªtre le bit 0
			      de spin_cnt. En 32 bits : ok. En 64 bits,
			      il faut les indiens d'Intel */
	};
	long int __status; /* "Free" or "taken" or head of waiting list */
} _lpt_IO_lock_t;

struct _LPT_IO_FILE {
  int _flags;           /* High-order word is _IO_MAGIC; rest is flags. */
#define _IO_file_flags _flags

  /* The following pointers correspond to the C++ streambuf protocol. */
  /* Note:  Tk uses the _IO_read_ptr and _IO_read_end fields directly. */
  char* _IO_read_ptr;   /* Current read pointer */
  char* _IO_read_end;   /* End of get area. */
  char* _IO_read_base;  /* Start of putback+get area. */
  char* _IO_write_base; /* Start of put area. */
  char* _IO_write_ptr;  /* Current put pointer. */
  char* _IO_write_end;  /* End of put area. */
  char* _IO_buf_base;   /* Start of reserve area. */
  char* _IO_buf_end;    /* End of reserve area. */
  /* The following fields are used to support backing up and undo. */
  char *_IO_save_base; /* Pointer to start of non-current get area. */
  char *_IO_backup_base;  /* Pointer to first valid character of backup area */
  char *_IO_save_end; /* Pointer to end of non-current get area. */

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;
#if 0
  int _blksize;
#else
  int _flags2;
#endif
  _IO_off_t _old_offset; /* This used to be _offset but it's too small.  */

#define __HAVE_COLUMN /* temporary */
  /* 1+column number of pbase(); 0 is unknown. */
  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];

  /*  char* _save_gptr;  char* _save_egptr; */

  _lpt_IO_lock_t *_lock;
};

typedef struct _LPT_IO_FILE _LPT_IO_FILE;

#if 0
#define _IO_lock_initializer { 0, 0, NULL }

#define _IO_lock_init(_name) \
  ((_name) = (_lpt_IO_lock_t) _IO_lock_initializer , 0)

#define _IO_lock_fini(_name) \
  ((void) 0)
#endif

__tbx_inline__ static int lpt_iolock_acquire(_lpt_IO_lock_t *lock)
{
        ma_preempt_disable();
#ifdef MA__LWPS
        ma_bit_spin_lock(1, (unsigned long*)&lock->spin);
#endif
        return 0;
}

__tbx_inline__ static int lpt_iolock_release(_lpt_IO_lock_t *lock)
{
#ifdef MA__LWPS
        ma_bit_spin_unlock(1, (unsigned long*)&lock->spin);
#endif
        ma_preempt_enable();
        return 0;
}

__tbx_inline__ static void _lpt_IO_lock_lock(_lpt_IO_lock_t *_name)
{
	int __self = SELF_GETMEM(number);
	marcel_t self = MARCEL_SELF;
	lpt_iolock_acquire(_name);

	if (_name->__status == 0) {	/* is free */
		_name->__status = 1;
	} else {
		if (_name->owner != __self) {	/* busy */
			blockcell c, *first;

			c.next = NULL;
			c.blocked = tbx_true;
			c.task = (self) ? : marcel_self();
			first = (blockcell *) (_name->__status & ~1);
			if (first == NULL) {
				/* On est le premier Ã  attendre */
				_name->__status = 1 | ((unsigned long) &c);
				first = &c;
			} else {
				first->last->next = &c;
			}
			first->last = &c;
			mdebug("registering %p (cell %p) in reclock %p\n", self,
			    &c, _name);


			mdebug("blocking %p (cell %p) in reclock %p\n", self,
			    &c, _name);
			INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(c.blocked,
			    lpt_iolock_release(_name),
			    lpt_iolock_acquire(_name));
			mdebug("unblocking %p (cell %p) in reclock %p\n", self,
			    &c, _name);

		}

		_name->owner = __self;
	}
	mdebug("getting reclock %p in lock %p\n", self, _name);
	_name->spin_cnt += 4;
	lpt_iolock_release(_name);
}

__tbx_inline__ static int _lpt_IO_lock_trylock(_lpt_IO_lock_t *_name)
{
	int __result = 0;
	int __self = SELF_GETMEM(number);
	lpt_iolock_acquire(_name);
	if (_name->owner != __self) {
		if (_name->__status == 0) {	/* free */
			_name->__status = 1;
			_name->spin_cnt += 4;
		} else {
			__result = EBUSY;
		}
	} else
		_name->spin_cnt += 4;

	lpt_iolock_release(_name);

	return __result;
}

__tbx_inline__ static void _lpt_IO_lock_unlock(_lpt_IO_lock_t *_name)
{
	lpt_iolock_acquire(_name);
	_name->spin_cnt -= 4;
	if (_name->spin_cnt == 0) {
		blockcell *first;

		first = (blockcell *) (_name->__status & ~1);
		if (first != 0) {	/* waiting threads */
			_name->__status = (unsigned long) first->next | 1;
			if (first->next) {
				first->next->last = first->last;
			}
			mdebug("releasing reclock %p in lock %p to %p\n",
			    marcel_self(), _name, first->task);
			first->blocked = 0;
			ma_smp_wmb();
			ma_wake_up_thread(first->task);
		} else {
			mdebug("releasing reclock %p in lock %p\n",
			    marcel_self(), _name);
			_name->__status = 0;	/* free */
		}

	}
	lpt_iolock_release(_name);
}


void
__flockfile (stream)
	_LPT_IO_FILE *stream;
{
#warning workaround tant que marcel ne peut pas être initialisé assez tôt...
	if (marcel_activity)
		_lpt_IO_lock_lock (&(*stream->_lock));
}
strong_alias (__flockfile, _IO_flockfile)
weak_alias (__flockfile, flockfile)
	

void
__funlockfile (stream)
	_LPT_IO_FILE *stream;
{
	if (marcel_activity)	
		_lpt_IO_lock_unlock (&(*stream->_lock));
}
strong_alias (__funlockfile, _IO_funlockfile)
weak_alias (__funlockfile, funlockfile)

int
__ftrylockfile (stream)
	_LPT_IO_FILE *stream;
{
	if (marcel_activity)
		return _lpt_IO_lock_trylock (&(*stream->_lock));
	return 0;
}
strong_alias (__ftrylockfile, _IO_ftrylockfile)
weak_alias (__ftrylockfile, ftrylockfile)

#endif //VD:
