#!/bin/sh

#
# This generates the PM2_FLAVOR_OPTIONS macro that provides --enable options for
# pm2 options:
#
# --enable-modulename
# --enable-modulename-moduleoption
# --enable-modulename-modulevar=value
# --enable-all-genericmoduleoption
# --enable-shared is an alias to --enable-all-build-dynamic
# --enable-static is an alias to --enable-all-build-static
#
# It also collects user-provided options in PM2_CONFIG_OPTIONS
#

set -e

# TODO: automatically enable dependencies :)
# Notes: the dependency hacks are just temporary hacks for the conveniency of
# the user for now. Proper automatic generation would be necessary

(
	echo "# Do not modify this file, it is generated by $0"
	
	echo 'AC_DEFUN([PM2_FLAVOR_OPTIONS],['

	# We start with an empty flavor
	echo "PM2_FLAVOR_CONTENT=''"

	echo "PM2_CONFIG_OPTIONS=''"
	
	# add --enable-shared/--enable-static, default to static only
	cat << "EOF"
AC_ARG_ENABLE([shared],
	[AS_HELP_STRING([--enable-shared],
	[build shared libraries [default=no]])],
	[PM2_CONFIG_OPTIONS="$PM2_CONFIG_OPTIONS --enable-shared" ; enable_all_build_dynamic=$enableval],
	[PM2_CONFIG_OPTIONS="$PM2_CONFIG_OPTIONS --disable-shared" ; enable_all_build_dynamic=no])
AC_ARG_ENABLE([static],
	[AS_HELP_STRING([--enable-static],
	[build static libraries [default=yes]])],
	[PM2_CONFIG_OPTIONS="$PM2_CONFIG_OPTIONS --enable-static" ; enable_all_build_static=$enableval],
	[PM2_CONFIG_OPTIONS="$PM2_CONFIG_OPTIONS --enable-static" ; enable_all_build_static=yes])
EOF

	# if both are set, enable both build instead
	cat << EOF
if test "\$enable_all_build_dynamic" = yes -a \
        "\$enable_all_build_static" = yes
then
	unset enable_all_build_dynamic 
	unset enable_all_build_static
	enable_all_build_both=yes
fi
EOF

	add_option () {
		# usage: add_option module output_module option
                # This creates an autoconf --enable-output_module-bar option for
                # a given pm2 option of some module.

		# extra can be used for extra dependencies
		# the default value can be given

		module="$1"
		output_module="$2"
		option="$3"
		extra="$4"
		default="$5"

		# strip path/extension to get the PM2 flavor option
		pm2_opt="`echo "$option" | sed -e "s:^modules/$module/config/options/[0-9]*\\(.*\\).sh\$:\\1:"`"
		pm2_opt="${pm2_opt%.sh}"

		# replace _ by - to get the autoconf option
		ac_opt="`echo "$pm2_opt" | tr _ -`"

		# dependency hacks

		# profile/debug deps
		[ "$pm2_opt" != profile -a "$pm2_opt" != gcc_instrument ] || extra="$extra --modules=profile"
		[ "$pm2_opt" != debug ] || extra="$extra --tbx=debug"

		# marcel deps
		[ "$pm2_opt" != marcel_poll ] || extra="$extra --modules=marcel"
		[ "$pm2_opt" != pthread ] || extra="$extra --marcel=pmarcel --marcel=dont_use_pthread --marcel=enable_signals --marcel=enable_deviation --modules=puk --puk=enable_pukabi"
		[ "$pm2_opt" != enable_once ] || extra="$extra --marcel=enable_cleanup"
		[ "$pm2_opt" != enable_postexit ] || extra="$extra --marcel=enable_atexit"
		[ "$pm2_opt" != enable_signals ] || extra="$extra --marcel=enable_deviation"
		[ "$pm2_opt" != enable_suspend ] || extra="$extra --marcel=enable_deviation"
		[ "$pm2_opt" != numa ] || extra="$extra --marcel=enable_stats --init=topology"
		[ "$pm2_opt" != smp ] || extra="$extra --marcel=enable_stats --init=topology"

		# nmad deps
		[ "$pm2_opt" != ib_rcache ] || extra="$extra --modules=puk --puk=enable_pukabi"
		[ "$pm2_opt" != strat_qos ] || extra="$extra --nmad=quality_of_service"

		# tbx deps
		[ "$pm2_opt" != parano_malloc ] || extra="$extra --tbx=safe_malloc"

		# if there is a trailing ':', we'll take the argument
		case "$ac_opt" in
			*:*)	val='$enableval';;
			*)	val='';;
		esac
				
		# and now drop trailing :
		ac_opt="${ac_opt%:}"

		# drop duplicate enable_ part which don't look pretty
		ac_opt="${ac_opt#enable-}"
		ac_opt_="`echo "$ac_opt" | tr - _`"

		# a priori, put the pm2 option in PM2_FLAVOR_CONTENT
		VAR=PM2_FLAVOR_CONTENT
		# but for options which do not default to no, put them appart to
		# avoid believing the user gave them
		[ "$default" = no -a "$pm2_opt" != build_static ] || VAR=PM2_FLAVOR_CONTENT_DEFAULT

		cat << EOF
AC_ARG_ENABLE([$output_module-$ac_opt],
	,
	[PM2_CONFIG_OPTIONS="\$PM2_CONFIG_OPTIONS --enable-$output_module-$ac_opt=\$enableval"],
	[enable_${output_module}_${ac_opt_}=$default])
if test "\$enable_${output_module}_${ac_opt_}" != no
then
	$VAR="\$$VAR --$output_module=$pm2_opt$val $extra"
fi
EOF
	}


	# for each module
	for i in modules/*
	do
		[ -d $i ] || continue

		module="${i#modules/}"

		# add an option to enable it
		if [ "$module" != appli \
		  -a "$module" != common \
		  -a "$module" != generic ]
		then
			extra=

			# dependency hacks
			[ "$module" != bubblegum ] || extra="$extra --modules=bubblelib --modules=bubbles --modules=marcel --marcel=profile --modules=profile"
			[ "$module" != bubbles ] || extra="$extra --modules=bubblelib --modules=marcel --marcel=profile --modules=profile"
			[ "$module" != bubblelib ] || extra="$extra --modules=marcel --marcel=profile --modules=profile"
			[ "$module" != fut2paje ] || extra="$extra --modules=profile"
			[ "$module" != leonie ] || extra="$extra --modules=ntbx --modules=leoparse"
			[ "$module" != leoparse ] || extra="$extra --modules=ntbx"
			[ "$module" != mad3 ] || extra="$extra --modules=ntbx"
			[ "$module" != nmad ] || extra="$extra --modules=ntbx --modules=puk"
			[ "$module" != memory ] || extra="$extra --modules=ntbx"
			[ "$module" != stackalign ] || extra="$extra --modules=marcel"
			[ "$module" = ezflavor ] || extra="$extra --modules=init --modules=tbx"

			cat << EOF
AC_ARG_ENABLE([$module],
	[AS_HELP_STRING([--enable-$module],
	[enable the \`$module' module [default=no]])],
	[PM2_CONFIG_OPTIONS="\$PM2_CONFIG_OPTIONS --enable-$module"],
	[enable_$module=no])
if test "\$enable_$module" != no
then
	PM2_FLAVOR_CONTENT="\$PM2_FLAVOR_CONTENT --modules=$module $extra"
fi
EOF
		fi


		# for each of this module's options
		if [ "$module" != generic ]
		then
			for j in modules/$module/config/options/*.sh
			do
				# add an option to enable it
				extra=
				add_option "$module" "$module" "$j" "$extra" "no"
			done
		fi
	done

	# add --enable-all-foo for generic options
	for j in modules/generic/config/options/*.sh
	do
		if test "$j" = modules/generic/config/options/00opt.sh
		then
			add_option generic all "$j" "" "yes"
		else
			add_option generic all "$j" "" "no"
		fi
	done

	# Enable some options by default for some modules
	cat << "EOF"
if test -n "$PM2_FLAVOR_CONTENT"
then
	# The use gave some options, complete with the default options if any
	PM2_FLAVOR_CONTENT="$PM2_FLAVOR_CONTENT $PM2_FLAVOR_CONTENT_DEFAULT"

	# Memory enabled but no memory manager set?
	if echo "$PM2_FLAVOR_CONTENT" | grep -- --modules=memory >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --memory=enable_mami >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --memory=enable_heap_allocator >/dev/null
	then
		# Default to mami
		PM2_FLAVOR_CONTENT="$PM2_FLAVOR_CONTENT --memory=enable_mami"
	fi

	# Memory enabled but no thread interface set set?
	if echo "$PM2_FLAVOR_CONTENT" | grep -- --modules=memory >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --memory=marcel >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --memory=pthread >/dev/null
	then
		# Default to marcel
		PM2_FLAVOR_CONTENT="$PM2_FLAVOR_CONTENT --modules=marcel --memory=marcel"
	fi

	# Marcel enabled but no marcel type set?
	if echo "$PM2_FLAVOR_CONTENT" | grep -- --modules=marcel >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --marcel=mono >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --marcel=smp  >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --marcel=numa >/dev/null
	then
		# Default to numa
		PM2_FLAVOR_CONTENT="$PM2_FLAVOR_CONTENT --marcel=numa --marcel=enable_stats --init=topology"
	fi

	# NewMadeleine enabled but no protocol set?
	if echo "$PM2_FLAVOR_CONTENT" | grep -- --modules=nmad >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --nmad=ibverbs >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --nmad=mx >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --nmad=qsnet >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --nmad=sisci >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --nmad=tcp > /dev/null
	then
		# Default to TCP
		PM2_FLAVOR_CONTENT="$PM2_FLAVOR_CONTENT --nmad=tcp"
	fi
	if echo "$PM2_FLAVOR_CONTENT" | grep -- --modules=nmad >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --nmad=tag_ >/dev/null
	then
		# Default to huge tags
		PM2_FLAVOR_CONTENT="$PM2_FLAVOR_CONTENT --nmad=tag_as_hashtable"
	fi

	# Mad3 enabled but no protocol set?
	if echo "$PM2_FLAVOR_CONTENT" | grep -- --modules=mad3 >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=bip >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=gm >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=madico >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=mpi >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=mpl >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=mx >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=quadrics >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=rand >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=sbp >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=sctp >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=sisci >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=tcp >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=udp >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=via >/dev/null && \
		! echo "$PM2_FLAVOR_CONTENT" | grep -- --mad3=vrp >/dev/null
	then
		# Default to TCP
		PM2_FLAVOR_CONTENT="$PM2_FLAVOR_CONTENT --mad3=tcp"
	fi
fi
EOF
	# default mad3 proto (tcp I guess)

	echo '])'
) > m4/pm2-flavor-options.m4

ACLOCAL="aclocal -I m4" exec autoreconf -vfi
