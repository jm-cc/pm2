#-*-sh-*-
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

create_test_flavor() {
    # Creation de la flavor
    case "$flavor" in
	test_marcel_barrier)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor="$flavor"			\
		--ext=""							\
		--modules="\"marcel tbx init\"" --marcel="mono"			\
		--marcel="spinlock" --marcel="marcel_main" --marcel="pmarcel"	\
		--marcel="standard_main"					\
		--marcel="bug_on" --marcel="malloc_preempt_debug"		\
		--tbx="safe_malloc" --tbx="parano_malloc"			\
		--all="gdb" --all="build_static" $_output_redirect
	    ;;
	test_marcel_barrier_1MiB_stack)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor="$flavor"			\
		--ext=""							\
		--modules="\"marcel tbx init\"" --marcel="mono"			\
		--marcel="spinlock" --marcel="marcel_main" --marcel="pmarcel"	\
		--marcel="standard_main" --marcel="stacksize:1024"		\
		--marcel="bug_on" 			                        \
		--tbx="safe_malloc" --tbx="parano_malloc"			\
		--all="gdb" --all="build_static" $_output_redirect
	    ;;
	test_marcel_bubble_memory)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor="$flavor"		\
		--ext=""						\
		--modules="\"marcel tbx init\"" --marcel="numa"		\
		--marcel="spinlock" --marcel="marcel_main"		\
		--marcel="standard_main" --marcel="pmarcel"		\
		--marcel="enable_stats"	--marcel="enable_mami"	        \
		--marcel="bubble_sched_memory"			        \
		--marcel="bug_on" --marcel="malloc_preempt_debug"	\
		--tbx="safe_malloc" --tbx="parano_malloc"		\
		--all="gdb" --all="build_static" $_output_redirect
	    ;;
	test_marcel_pthread_abi)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor="$flavor"	\
		--ext="" --all="build_dynamic" --marcel="build_dynamic"	\
		--modules="\"marcel tbx init puk\""			\
		--marcel="smp"						\
		--marcel="spinlock" --marcel="standard_main"		\
		--marcel="pmarcel" --marcel="pthread"			\
		--marcel="enable_cleanup" --marcel="enable_deviation"	\
		--marcel="enable_keys" --marcel="enable_signals"	\
		--marcel="enable_once" --marcel="dont_use_pthread"	\
		--marcel="bug_on" --puk="enable_pukabi"			\
		--puk="disable_fd_virtualization"			\
		--tbx="safe_malloc" --tbx="parano_malloc"		\
		--all="gdb" $_output_redirect
	    ;;
	test_marcel_pthread_abi_stackalign)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor="$flavor"	\
		--ext="" --all="build_dynamic" --marcel="build_dynamic"	\
		--modules="\"marcel tbx init puk stackalign\""		\
		--marcel="smp"						\
		--marcel="spinlock" --marcel="main_as_func"		\
		--marcel="pmarcel" --marcel="pthread"			\
		--marcel="enable_cleanup" --marcel="enable_deviation"	\
		--marcel="enable_keys" --marcel="enable_signals"	\
		--marcel="enable_once" --marcel="dont_use_pthread"	\
		--marcel="bug_on" --puk="enable_pukabi"			\
		--puk="disable_fd_virtualization"			\
		--tbx="safe_malloc" --tbx="parano_malloc"		\
		--all="gdb" $_output_redirect
	    ;;
	test_marcel_pthread_cpp)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor="$flavor"		\
		--ext=""							\
		--modules="\"marcel tbx init\"" --marcel="mono"			\
		--marcel="spinlock" --marcel="marcel_main" --marcel="pmarcel"	\
		--marcel="standard_main"					\
		--marcel="enable_keys" --marcel="enable_deviation"		\
		--marcel="enable_cleanup" --marcel="enable_once"		\
		--marcel="bug_on"						\
		--all="gdb" --all="build_static" $_output_redirect
	    ;;
	test_marcel_bubble)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor="$flavor"		\
		--ext=""						\
		--modules="\"marcel tbx init\"" --marcel="numa"		\
		--marcel="spinlock" --marcel="marcel_main"		\
		--marcel="standard_main" --marcel="pmarcel"		\
		--marcel="enable_stats"					\
		--marcel="bubble_sched_cache"			\
		--marcel="bug_on" --marcel="malloc_preempt_debug"	\
		--tbx="safe_malloc" --tbx="parano_malloc"		\
		--all="gdb" --all="build_static" $_output_redirect
	    ;;
	test_marcel_dynamic)
	        eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
		    --ext=\"\" \
		    --modules=\"marcel tbx init\" \
		    --marcel=\"mono\" --marcel=\"spinlock\" --marcel=\"marcel_main\" \
		    --tbx=\"safe_malloc\" --tbx=\"parano_malloc\" \
		    --all=\"opt\" --all=\"gdb\" --all=\"build_dynamic\" $_output_redirect
		;;
	test_marcel)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
		--ext=\"\" \
		--modules=\"marcel tbx init\" \
		--marcel=\"mono\" --marcel=\"spinlock\" --marcel=\"marcel_main\" \
		--tbx=\"safe_malloc\" --tbx=\"parano_malloc\" \
		--all=\"opt\" --all=\"gdb\" --all=\"build_static\" $_output_redirect
	    ;;
	test_marcel_tls)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor="$flavor"			\
		--ext=""							\
		--modules="\"marcel tbx init\"" --marcel="mono"			\
		--marcel="bug_on" --marcel="spinlock" --marcel="marcel_main"	\
		--marcel="standard_main" --marcel="dont_use_pthread"		\
		--marcel="malloc_preempt_debug"					\
		--tbx="safe_malloc" --tbx="parano_malloc"			\
		--all="gdb" --all="build_static" $_output_redirect
	    ;;
	test_marcel_smp)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
		--ext=\"\" \
		--modules=\"marcel tbx init\" \
		--marcel=\"smp\" --marcel=\"marcel_main\" \
		--marcel=\"bubble_sched_null\" \
		--tbx=\"safe_malloc\" --tbx=\"parano_malloc\" \
		--all=\"opt\" --all=\"gdb\" --all=\"build_static\" $_output_redirect
	    ;;
	test_marcel_dynamic_smp)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
		--ext=\"\" \
		--modules=\"marcel tbx init\" \
		--marcel=\"smp\" --marcel=\"marcel_main\" \
		--tbx=\"safe_malloc\" --tbx=\"parano_malloc\" \
		--all=\"opt\" --all=\"gdb\" --all=\"build_dynamic\" $_output_redirect
	    ;;
	test_marcel_cleanup)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
		--ext=\"\" \
		--modules=\"marcel tbx init\" \
		--marcel=\"mono\" --marcel=\"spinlock\" --marcel=\"enable_cleanup\" --marcel=\"marcel_main\" \
		--tbx=\"safe_malloc\" --tbx=\"parano_malloc\" \
		--all=\"opt\" --all=\"gdb\" --all=\"build_static\" $_output_redirect
	    ;;
	test_marcel_cpp_pmarcel)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
		--ext=\"\" \
		--modules=\"marcel tbx init\" \
		--marcel=\"smp\" --marcel=\"spinlock\" --marcel=\"marcel_main\" \
		--marcel=\"enable_cleanup\" --marcel=\"enable_once\" --marcel=\"enable_keys\" \
		--marcel=\"pmarcel\" \
		--tbx=\"safe_malloc\" --tbx=\"parano_malloc\" \
		--all=\"opt\" --all=\"gdb\" --all=\"build_static\" $_output_redirect
	    ;;
	test_marcel_cleanup_once)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
		--ext=\"\" \
		--modules=\"marcel tbx init\" \
		--marcel=\"mono\" --marcel=\"spinlock\" --marcel=\"enable_cleanup\" --marcel=\"enable_once\" --marcel=\"marcel_main\" \
		--tbx=\"safe_malloc\" --tbx=\"parano_malloc\" \
		--all=\"opt\" --all=\"gdb\" --all=\"build_static\" $_output_redirect
	    ;;
	test_marcel_jump)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
		--ext=\"\" \
		--modules=\"marcel tbx init\" \
		--marcel=\"mono\" --marcel=\"spinlock\" --marcel=\"marcel_main\" --marcel=\"stack_jump\" \
		--tbx=\"safe_malloc\" --tbx=\"parano_malloc\" \
		--all=\"opt\" --all=\"gdb\" --all=\"build_static\" $_output_redirect
	    ;;
	test_marcel_virtual_timer)
	        eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
		    --ext=\"\" \
		    --modules=\"marcel tbx init\" \
		    --marcel=\"mono\" --marcel=\"spinlock\" --marcel=\"marcel_main\" \
		    --marcel=\"use_virtual_timer\" \
		    --tbx=\"safe_malloc\" --tbx=\"parano_malloc\" \
		    --all=\"gdb\" --all=\"build_static\" $_output_redirect
		;;
	test_marcel_userspace)
	    eval ${PM2_ROOT}/bin/pm2-flavor set --flavor=\"$flavor\" \
		--ext=\"\" \
		--modules=\"marcel tbx init\" \
		--marcel=\"mono\" --marcel=\"enable_userspace\" --marcel=\"spinlock\" --marcel=\"marcel_main\" \
		--tbx=\"safe_malloc\" --tbx=\"parano_malloc\" \
		--all=\"opt\" --all=\"gdb\" --all=\"build_static\" $_output_redirect
	    ;;
    esac
}

