#!/usr/bin/perl -n
##################

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2007 "the PM2 team" (see AUTHORS file)
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
#

# parse and display a merged Nmad/FUT trace (NS/NAM version)

use warnings;
use strict;
use Math::BigFloat;
use Getopt::Std;
use POSIX qw(floor);

our $link_rate;			# global NAM link rate
our $link_delay;		# global NAM link delay
our $time_divisor;		# time stamp divisor
our $pseudo_time;		# (bool) pseudo time mode
our $pseudo_time_counter;	# virtual time counter
our $annotate_id;		# next unique annotation id
our $preamble;			# (bool) need to insert preamble before comms
our $nb_events;			# event counter

# FUT codes for nmad events
our $FUT_NMAD_CODE;

our $FUT_NMAD_EVENT0_CODE;
our $FUT_NMAD_EVENT1_CODE;
our $FUT_NMAD_EVENT2_CODE;
our $FUT_NMAD_EVENTSTR_CODE;

our $FUT_NMAD_EVENT_CONFIG_CODE;
our $FUT_NMAD_EVENT_DRV_ID_CODE;
our $FUT_NMAD_EVENT_DRV_NAME_CODE;
our $FUT_NMAD_EVENT_CNX_CONNECT_CODE;
our $FUT_NMAD_EVENT_CNX_ACCEPT_CODE;
our $FUT_NMAD_EVENT_NEW_TRK_CODE;
our $FUT_NMAD_EVENT_SND_START_CODE;
our $FUT_NMAD_EVENT_RCV_END_CODE;

our $FUT_NMAD_EVENT_ANNOTATE_CODE;

our %fut_code_mapping;

# Topology information
our %hosts;

# Current time stamp
our $time;

BEGIN {
    # link rate and delay values are arbitrary
    $link_rate			= 10000;
    $link_delay			= 0.002;
    $time_divisor		= Math::BigFloat->new('100000000');
    $pseudo_time		= 0;
    $pseudo_time_counter	= 0;
    $annotate_id		= 0;
    $preamble			= 1;
    $nb_events			= 0;

    my %opts;
    my $ret	= getopts('pd:r:t:n:', \%opts);	# -p -d <DELAY> -r <RATE> -t <TIME_DIVISOR> -n <NB_EVENTS>

    # switch on virtual timer
    if (exists $opts{'p'}) {
        $pseudo_time	= $opts{'p'};
        $link_rate	= 250000;
        $link_delay	= 0.002;
    }

    # set global NAM link delay
    if (exists $opts{'d'}) {
        $link_delay	= $opts{'d'};
    }

    # set global NAM link rate
    if (exists $opts{'r'}) {
        $link_rate	= $opts{'r'};
    }

    # set timestamp divisor
    if (exists $opts{'t'}) {
        $time_divisor	= Math::BigFloat->new($opts{'t'});
    }

    # stop after nb events (until the end if == 0)
    if (exists $opts{'n'}) {
        $nb_events	= $opts{'n'};
    }

    $FUT_NMAD_CODE	= 0xfe00;

    $FUT_NMAD_EVENT0_CODE	= $FUT_NMAD_CODE + 0x00;
    $FUT_NMAD_EVENT1_CODE	= $FUT_NMAD_CODE + 0x01;
    $FUT_NMAD_EVENT2_CODE	= $FUT_NMAD_CODE + 0x02;
    $FUT_NMAD_EVENTSTR_CODE	= $FUT_NMAD_CODE + 0x03;

    $FUT_NMAD_EVENT_CONFIG_CODE			= $FUT_NMAD_CODE + 0x10;
    $FUT_NMAD_EVENT_DRV_ID_CODE			= $FUT_NMAD_CODE + 0x11;
    $FUT_NMAD_EVENT_DRV_NAME_CODE		= $FUT_NMAD_CODE + 0x12;
    $FUT_NMAD_EVENT_CNX_CONNECT_CODE		= $FUT_NMAD_CODE + 0x13;
    $FUT_NMAD_EVENT_CNX_ACCEPT_CODE		= $FUT_NMAD_CODE + 0x14;
    $FUT_NMAD_EVENT_NEW_TRK_CODE		= $FUT_NMAD_CODE + 0x15;
    $FUT_NMAD_EVENT_SND_START_CODE		= $FUT_NMAD_CODE + 0x16;
    $FUT_NMAD_EVENT_RCV_END_CODE		= $FUT_NMAD_CODE + 0x17;

    $FUT_NMAD_EVENT_ANNOTATE_CODE		= $FUT_NMAD_CODE + 0x20;

    $fut_code_mapping{$FUT_NMAD_EVENT0_CODE  }	= 'EVENT0';
    $fut_code_mapping{$FUT_NMAD_EVENT1_CODE  }	= 'EVENT1';
    $fut_code_mapping{$FUT_NMAD_EVENT2_CODE  }	= 'EVENT2';
    $fut_code_mapping{$FUT_NMAD_EVENTSTR_CODE}	= 'EVENTSTR';

    $fut_code_mapping{$FUT_NMAD_EVENT_CONFIG_CODE     }	= 'CONFIG';
    $fut_code_mapping{$FUT_NMAD_EVENT_DRV_ID_CODE     }	= 'DRV_ID';
    $fut_code_mapping{$FUT_NMAD_EVENT_DRV_NAME_CODE   }	= 'DRV_NAME';
    $fut_code_mapping{$FUT_NMAD_EVENT_CNX_CONNECT_CODE}	= 'CNX_CONNECT';
    $fut_code_mapping{$FUT_NMAD_EVENT_CNX_ACCEPT_CODE }	= 'CNX_ACCEPT';
    $fut_code_mapping{$FUT_NMAD_EVENT_NEW_TRK_CODE    }	= 'NEW_TRK';
    $fut_code_mapping{$FUT_NMAD_EVENT_SND_START_CODE  }	= 'SND_START';
    $fut_code_mapping{$FUT_NMAD_EVENT_RCV_END_CODE    }	= 'RCV_END';

    $fut_code_mapping{$FUT_NMAD_EVENT_ANNOTATE_CODE}	= 'ANNOTATE';

    # write trace header
    print "V -t * -v 1.0a5 -a 0\n";

    # write colormap declaration
    my $i = 0;
    print "c -t * -i ${i} -n magenta1\n";		 $i++;
    print "c -t * -i ${i} -n violetred1\n";		 $i++;
    print "c -t * -i ${i} -n red1\n";		 	 $i++;
    print "c -t * -i ${i} -n chocolate1\n";		 $i++;
    print "c -t * -i ${i} -n yellow1\n";		 $i++;
    print "c -t * -i ${i} -n chartreuse1\n";		 $i++;
    print "c -t * -i ${i} -n springgreen1\n";		 $i++;
    print "c -t * -i ${i} -n turquoise1\n";		 $i++;
    print "c -t * -i ${i} -n deepskyblue1\n";		 $i++;
    print "c -t * -i ${i} -n slateblue1\n";		 $i++;
    print "c -t * -i ${i} -n magenta2\n";		 $i++;
    print "c -t * -i ${i} -n violetred2\n";		 $i++;
    print "c -t * -i ${i} -n red2\n";		 	 $i++;
    print "c -t * -i ${i} -n chocolate2\n";		 $i++;
    print "c -t * -i ${i} -n yellow2\n";		 $i++;
    print "c -t * -i ${i} -n chartreuse2\n";		 $i++;
    print "c -t * -i ${i} -n springgreen2\n";		 $i++;
    print "c -t * -i ${i} -n turquoise2\n";		 $i++;
    print "c -t * -i ${i} -n deepskyblue2\n";		 $i++;
    print "c -t * -i ${i} -n slateblue2\n";		 $i++;
    print "c -t * -i ${i} -n magenta3\n";		 $i++;
    print "c -t * -i ${i} -n violetred3\n";		 $i++;
    print "c -t * -i ${i} -n red3\n";		 	 $i++;
    print "c -t * -i ${i} -n chocolate3\n";		 $i++;
    print "c -t * -i ${i} -n yellow3\n";		 $i++;
    print "c -t * -i ${i} -n chartreuse3\n";		 $i++;
    print "c -t * -i ${i} -n springgreen3\n";		 $i++;
    print "c -t * -i ${i} -n turquoise3\n";		 $i++;
    print "c -t * -i ${i} -n deepskyblue3\n";		 $i++;
    print "c -t * -i ${i} -n slateblue3\n";		 $i++;
}

END {
    # force a flush of events at the end of the trace with a dummy event T
    if ($pseudo_time) {
        $pseudo_time_counter++;
        print "T -t ${pseudo_time_counter}\n";
    } else {
        $time++;
        print "T -t ${time}\n";
    }

}

sub preamble {
    # write the trace preamble before the first packet send/recv
    $preamble = 0;

    my $time_info;
    if ($pseudo_time) {
        $time_info = "${pseudo_time_counter}";
    } else {
        $time_info = "${time}";
    }

    # set the replay speed when in virtual timer mode
    if ($pseudo_time) {
        print "v -t ${time_info} -e set_rate_ext 0.2ms 1\n";
    }

    # write the agents and monitored variables
    foreach my $_host_num (keys %hosts) {
        my $_host		= $hosts{$_host_num};
        my $_config_rank	= ${$_host}{'config_rank'};

        print "a -t ${time_info} -n node_${_config_rank} -s ${_config_rank}\n";
        print "v -t ${time_info} -e monitor_agent ${_config_rank} node_${_config_rank}\n";
    }
}

# main loop in 'perl -n' mode

# remove trailing \n
chomp;

# parse the event
my @params		= split ',';
my $num			= shift @params;
my $host_num		= shift @params;
my $ev_time		= Math::BigFloat->new(shift @params);
my $ev_tid		= shift @params;
my $ev_num		= shift @params;
my $ev_nb_params	= shift @params;

# skip unwanted events
next
    unless (($ev_num & 0xff00) == 0xfe00);

# scale the time stamp
$time	= $ev_time->copy()->bdiv($time_divisor);

#printf "$num: \t$host_num - [%16u] %x = %16s \t", $ev_time, $ev_num, $fut_code_mapping{$ev_num};

if ($ev_num == $FUT_NMAD_EVENT_CONFIG_CODE) {
    # declare a new node

    my $config_size	= shift @params;
    my $config_rank	= shift @params;

    my $host	= {};
    ${$host}{'config_rank'}	= $config_rank;
    ${$host}{'config_size'}	= $config_size;
    ${$host}{'drivers'}		= {};
    ${$host}{'gate2rank'}	= {};
    ${$host}{'rank2gate'}	= {};
    $hosts{$host_num}	= $host;

    #print "$config_rank/$config_size";

    my $time_info = '*';
    unless ($preamble) {
        if ($pseudo_time) {
            $time_info = "${pseudo_time_counter}";
        } else {
            $time_info = "${time}";
        }
    }

    print "n -t ${time_info} -a ${config_rank} -s ${config_rank} -v circle -c tan -i tan\n";
} else {
    # other host-related events

    my $host	= $hosts{$host_num};
    my $drivers	= ${$host}{'drivers'};
    my $gate2rank	= ${$host}{'gate2rank'};
    my $rank2gate	= ${$host}{'rank2gate'};
    my $config_rank	= ${$host}{'config_rank'};

    if ($ev_num == $FUT_NMAD_EVENT_DRV_ID_CODE) {
        # new driver, first part (id)
        my $drv_id	= shift @params;
        ${$host}{'drv_id'}	= $drv_id;
        ${$drivers}{$drv_id}	= {};
    } elsif ($ev_num == $FUT_NMAD_EVENT_DRV_NAME_CODE) {
        # new driver, second part (name)
        unless (exists ${$host}{'drv_id'}) {
            print "\n";
            die "missing drv_id event\n";
        }

        my $drv_id	= ${$host}{'drv_id'};
        my $driver	= ${$drivers}{$drv_id};

        my $drv_name	= '';
        foreach my $param (@params) {
            $drv_name	.= pack ("V", $param);
        }

        ${$driver}{'name'}		= $drv_name;
        ${$driver}{'links'}		= {};
        ${$driver}{'pending_cnx'}	= [];

        delete ${$host}{'drv_id'};
    } elsif ($ev_num == $FUT_NMAD_EVENT_NEW_TRK_CODE) {
        # new track
        my $gate_id	= shift @params;
        my $drv_id	= shift @params;
        my $trk_id	= shift @params;

        my $driver	= ${$drivers}{$drv_id};

        my $links	= ${$driver}{'links'};
        ${$links}{"$gate_id:$trk_id"}	= 1;

        my $pending_cnx	= ${$driver}{'pending_cnx'};
        push @{$pending_cnx}, [$gate_id, $drv_id, $trk_id];
    } elsif ($ev_num == $FUT_NMAD_EVENT_CNX_CONNECT_CODE) {
        # connect
        my $remote_rank	= shift @params;
        my $gate_id	= shift @params;
        my $drv_id	= shift @params;
        my $driver	= ${$drivers}{$drv_id};

        unless (exists ${$gate2rank}{$gate_id}) {
            ${$gate2rank}{$gate_id}		= $remote_rank;
            ${$rank2gate}{$remote_rank}	= $gate_id;
        }

        my $time_info = '*';
        unless ($preamble) {
            if ($pseudo_time) {
                $time_info = "${pseudo_time_counter}";
            } else {
                $time_info = "${time}";
            }
        }

        # write a new link
        print "l -t ${time_info} -s ${config_rank} -d ${remote_rank} -S UP -c black -r ${link_rate} -D ${link_delay}\n";
    } elsif ($ev_num == $FUT_NMAD_EVENT_CNX_ACCEPT_CODE) {
        # accept
        my $remote_rank	= shift @params;
        my $gate_id	= shift @params;
        my $drv_id	= shift @params;
        my $driver	= ${$drivers}{$drv_id};

        unless (exists ${$gate2rank}{$gate_id}) {
            ${$gate2rank}{$gate_id}		= $remote_rank;
            ${$rank2gate}{$remote_rank}	= $gate_id;
        }
    } elsif ($ev_num == $FUT_NMAD_EVENT_SND_START_CODE) {
        # message start

        # insert preamble if needed
        if ($preamble) {
            preamble();
        }

        my $gate_id	= shift @params;
        my $drv_id	= shift @params;
        my $trk_id	= shift @params;
        my $length	= shift @params;

        my $remote_rank	= ${$gate2rank}{$gate_id};
        my $driver	= ${$drivers}{$drv_id};
        my $drv_name	= ${$driver}{'name'};

        if ($pseudo_time) {
            # compute the pseudo length of the message to draw
            my $pseudo_length = POSIX::floor(log($length)/log(2));

            # update the monitored variable 'length' with the length of the message
            print "f -t ${pseudo_time_counter} -s ${config_rank} -a node_${config_rank} -T v -n length -v ${length} -o\n";

            # insert the message in the queue
            print "+ -t ${pseudo_time_counter} -e ${pseudo_length} -a ${pseudo_length} -s ${config_rank} -d ${remote_rank}\n";

            # remove the message from the queue
            print "- -t ${pseudo_time_counter} -e ${pseudo_length} -a ${pseudo_length} -s ${config_rank} -d ${remote_rank}\n";

            # put the message on the link
            print "h -t ${pseudo_time_counter} -e ${pseudo_length} -a ${pseudo_length} -s ${config_rank} -d ${remote_rank}\n";

            # extract the message from the link
            $pseudo_time_counter += $link_delay * $pseudo_length / ($link_rate * $link_delay);
            print "r -t ${pseudo_time_counter} -e ${pseudo_length} -a ${pseudo_length} -s ${remote_rank} -d ${config_rank}\n";

            # reset the monitored variable 'length'
            $pseudo_time_counter += $link_delay ;
            print "f -t ${pseudo_time_counter} -s ${config_rank} -a node_${config_rank} -T v -n length -o ${length} -x -v\n";
        } else {
            print "f -t ${time} -s ${config_rank} -a node_${config_rank} -T v -n length -v ${length} -o\n";
            print "+ -t ${time} -e ${length} -s ${config_rank} -d ${remote_rank}\n";
            print "- -t ${time} -e ${length} -s ${config_rank} -d ${remote_rank}\n";
            print "h -t ${time} -e ${length} -s ${config_rank} -d ${remote_rank}\n";
        }
    } elsif ($ev_num == $FUT_NMAD_EVENT_RCV_END_CODE) {
        # message end

        # insert preamble if needed
        if ($preamble) {
            preamble();
        }

        my $gate_id	= shift @params;
        my $drv_id	= shift @params;
        my $trk_id	= shift @params;
        my $length	= shift @params;

        my $remote_rank	= ${$gate2rank}{$gate_id};
        my $driver		= ${$drivers}{$drv_id};
        my $drv_name	= ${$driver}{'name'};

        unless ($pseudo_time) {
            print "r -t ${time} -e ${length} -s ${remote_rank} -d ${config_rank}\n";
            print "f -t ${time} -s ${remote_rank} -a node_${remote_rank} -T v -n length -o ${length} -x -v\n";

            # when in virtual time mode, the message reception is automatically inserted some delay
            # after the message send
        }
    } elsif ($ev_num == $FUT_NMAD_EVENT_ANNOTATE_CODE) {
        # code annotation

        # build the annotation string
        my $txt	= '';

        # binary concat the params
        foreach my $param (@params) {
            $txt	.= pack ("V", $param);
        }

        # convert the zero-terminated string into a Perl string
        ($txt)	= unpack "Z*", $txt;

        if ($pseudo_time) {
            print "v -t ${pseudo_time_counter} -e sim_annotation ${pseudo_time_counter} ${annotate_id} ${config_rank}: ${txt}\n";
            ${pseudo_time_counter} += $link_delay / 10;
        } else {
            print "v -t ${time} -e sim_annotation ${time} ${annotate_id} ${txt}\n";
        }

        $annotate_id++;
    }
}

exit
    # abort if the message number boundary has been reached
    if $nb_events && ($. >= $nb_events);
