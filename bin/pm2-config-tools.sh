#!/usr/bin/env pm2-sh
#-*-sh-*-

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# Example:
# if included_in -O6 PM2_MARCEL_CFLAGS ; then
#    ...
# fi
included_in()
{
    eval _l=\"`echo '$'$2`\"
    case " $_l " in
	*\ $1\ *) return 0 ;;
        *) return 1 ;;
    esac
}

not_included_in()
{
    if included_in ${@:+"$@"} ; then
	return 1
    else
	return 0
    fi
}

# Example:
# if defined_in MARCEL_SMP PM2_MARCEL_CFLAGS ; then
#    ...
# fi
defined_in() # option list
{
    if included_in -D$1 $2 ; then
	return 0
    else
	return 1
    fi
}

not_defined_in() # option list
{
    if defined_in ${@:+"$@"} ; then
	return 1
    else
	return 0
    fi
}

