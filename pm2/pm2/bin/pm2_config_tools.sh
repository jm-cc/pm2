#!/bin/sh

# Example:
# if defined_in -O6 PM2_MARCEL_CFLAGS ; then
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

