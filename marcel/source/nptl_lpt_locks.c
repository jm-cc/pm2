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


#include "marcel.h"
#include "nptl_flockfile.h"


#ifdef MA__LIBPTHREAD


__tbx_inline__ static int lpt_iolock_acquire(_lpt_IO_lock_t * lock LWPS_VAR_UNUSED)
{
	ma_preempt_disable();
#ifdef MA__LWPS
	ma_bit_spin_lock(1, (unsigned long *) &lock->spin);
#endif
	return 0;
}

__tbx_inline__ static int lpt_iolock_release(_lpt_IO_lock_t * lock LWPS_VAR_UNUSED)
{
#ifdef MA__LWPS
	ma_bit_spin_unlock(1, (unsigned long *) &lock->spin);
#endif
	ma_preempt_enable();
	return 0;
}



void _lpt_IO_lock_lock(_lpt_IO_lock_t * _name)
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
			c.task = (self) ? : ma_self();
			first = (blockcell *) (_name->__status & ~1);
			if (first == NULL) {
				/* On est le premier à attendre */
				_name->__status = 1 | ((unsigned long) &c);
				first = &c;
			} else {
				first->last->next = &c;
			}
			first->last = &c;
			MARCEL_LOG("registering %p (cell %p) in reclock %p\n", self,
				   &c, _name);


			MARCEL_LOG("blocking %p (cell %p) in reclock %p\n", self,
				   &c, _name);
			INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(c.blocked,
								   lpt_iolock_release
								   (_name),
								   lpt_iolock_acquire
								   (_name));
			MARCEL_LOG("unblocking %p (cell %p) in reclock %p\n", self, &c,
				   _name);

		}

		_name->owner = __self;
	}
	MARCEL_LOG("getting reclock %p in lock %p\n", self, _name);
	_name->spin_cnt += 4;
	lpt_iolock_release(_name);
}

int _lpt_IO_lock_trylock(_lpt_IO_lock_t * _name)
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

void _lpt_IO_lock_unlock(_lpt_IO_lock_t * _name)
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
			MARCEL_LOG("releasing reclock %p in lock %p to %p\n",
				   ma_self(), _name, first->task);
			first->blocked = tbx_false;
			ma_smp_wmb();
			ma_wake_up_thread(first->task);
		} else {
			MARCEL_LOG("releasing reclock %p in lock %p\n", ma_self(), _name);
			_name->__status = 0;	/* free */
		}

	}
	lpt_iolock_release(_name);
}


#endif /** MA__LIBPTHREAD **/
