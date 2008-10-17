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

#ifndef NM_ERRNO_H
#define NM_ERRNO_H

/** Error codes.
 */
enum nm_errno {
        NM_ESUCCESS		=  0,	/* successful operation	*/
        NM_EUNKNOWN		=  1,	/* unknown error	*/
        NM_ENOTIMPL		=  2,	/* not implemented	*/
        NM_ESCFAILD		=  3,	/* syscall failed, see	*
                                         * errno		*/
        NM_EAGAIN		=  4,	/* poll again		*/
        NM_ECLOSED		=  5,	/* connection closed	*/
        NM_EBROKEN		=  6,	/* error condition on	*
                                         * connection 		*/
        NM_EINVAL		=  7,	/* invalid parameter	*/
        NM_ENOTFOUND		=  8,	/* not found		*/
        NM_ENOMEM		=  9,	/* out of memory 	*/
        NM_EALREADY		= 10,	/* already in progress  *
                                         * or done		*/

        NM_ETIMEDOUT		= 11,	/* operation timeout	*/
        NM_EINPROGRESS		= 12,	/* operation in		*
                                         * progress		*/
        NM_EUNREACH		= 13,	/* destination		*
                                         * unreachable		*/
        NM_ECANCELED		= 14,	/* operation canceled	*/
        NM_EABORTED		= 15,	/* operation aborted	*/
	NM_EBUSY                = 16    /* gate/track is busy   */
};

#endif /* NM_ERRNO_H */
