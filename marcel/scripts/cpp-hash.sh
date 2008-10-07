#!/bin/sh

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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


debug_log ()
{
    # Uncomment the following line to get debugging output.
    #echo $* >&2
    :
}

# Usage: filter_out_external_headers ALLOWED_DIRECTORY
#
# Read CPP output on stdin, filtering out files not in
# ALLOWED_DIRECTORY.
filter_out_external_headers ()
{
    allowed_directory="$1"

    skipping_file=no
    line=""

    while read line
    do
	case "$line" in
	    \#\ [0-9]*${allowed_directory}*)
	      debug_log "noskip \`$line'"
	      skipping_file=no;;

	    \#\ [0-9]*)
	      debug_log "skip \`$line'"
	      skipping_file=yes;;

	    *)
	      if test "x$skipping_file" != "xyes"
	      then
		  echo "$line"
	      fi;;
	esac
    done
}

# Usage: find_hash_program
#
# Return the name of a hash program.
find_hash_program ()
{
    for prog in "sha1sum" "md5sum" "sha1" "md5" "wc -c"
    do
	if ( echo foobar | $prog ) >/dev/null 2>&1
	then
	    echo "$prog"
	    break
	fi
    done
}


if [ $# -ne 2 ] || [ x"$PM2_ROOT" = "x" ]
then
    echo "Usage: `basename $0` FLAVOR HEADER-FILE"
    echo
    echo "Preprocess HEADER-FILE with \`cpp' using FLAVOR and return a hash"
    echo "of the preprocessed output.  The resulting hash depends on FLAVOR's"
    echo "options; it does not depend on the contents of external headers."
    echo "The \`PM2_ROOT' environment variable must be set."
    exit 1
fi

flavor="$1"
header_file="$2"

export flavor
CPPFLAGS="$(pm2-config --flavor=$flavor --cflags)"
CPPFLAGS="$CPPFLAGS -DMARCEL_INTERNAL_INCLUDE"

hash_prog="$(find_hash_program)"

debug_log "using PM2 flavor \`$flavor' and header file \`$header_file'"
debug_log "using hash program \`$hash_prog'"
debug_log "PM2_ROOT: \`$PM2_ROOT"
debug_log "CPPFLAGS: \`$CPPFLAGS'"

cat "$header_file" | cpp $CPPFLAGS		\
| filter_out_external_headers "$PM2_ROOT"	\
| $hash_prog | grep -o '\([[:xdigit:]]\+\)'


# Local Variables:
# mode: sh
# tab-width: 8
# End:
