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

BEGIN {
    # link rate and delay values are arbitrary
    our $link_rate	= 10000;
    our $link_delay	= 0.002;
    our $time_divisor	= Math::BigFloat->new('100000000');
    our $pseudo_time	= 0;
    our $pseudo_time_counter	= 0;
    our $annotate_id	= 0;
    our $preamble	= 1;
    our $nb_events	= 0;

    my %opts;
    my $ret	= getopts('pd:r:t:n:', \%opts);	# -p -d <DELAY> -r <RATE> -t <TIME_DIVISOR> -n <NB_EVENTS>

    if (exists $opts{'p'}) {
        $pseudo_time	= $opts{'p'};
        $link_rate	= 250000;
        $link_delay	= 0.002;
    }

    if (exists $opts{'d'}) {
        $link_delay	= $opts{'d'};
    }

    if (exists $opts{'r'}) {
        $link_rate	= $opts{'r'};
    }

    if (exists $opts{'t'}) {
        $time_divisor	= Math::BigFloat->new($opts{'t'});
    }

    if (exists $opts{'n'}) {
        $nb_events	= $opts{'n'};
    }

    our $FUT_NMAD_CODE	= 0xfe00;

    our $FUT_NMAD_EVENT0_CODE	= $FUT_NMAD_CODE + 0x00;
    our $FUT_NMAD_EVENT1_CODE	= $FUT_NMAD_CODE + 0x01;
    our $FUT_NMAD_EVENT2_CODE	= $FUT_NMAD_CODE + 0x02;
    our $FUT_NMAD_EVENTSTR_CODE	= $FUT_NMAD_CODE + 0x03;

    our $FUT_NMAD_EVENT_CONFIG_CODE		= $FUT_NMAD_CODE + 0x10;
    our $FUT_NMAD_EVENT_DRV_ID_CODE		= $FUT_NMAD_CODE + 0x11;
    our $FUT_NMAD_EVENT_DRV_NAME_CODE		= $FUT_NMAD_CODE + 0x12;
    our $FUT_NMAD_EVENT_CNX_CONNECT_CODE	= $FUT_NMAD_CODE + 0x13;
    our $FUT_NMAD_EVENT_CNX_ACCEPT_CODE		= $FUT_NMAD_CODE + 0x14;
    our $FUT_NMAD_EVENT_NEW_TRK_CODE		= $FUT_NMAD_CODE + 0x15;
    our $FUT_NMAD_EVENT_SND_START_CODE		= $FUT_NMAD_CODE + 0x16;
    our $FUT_NMAD_EVENT_RCV_END_CODE		= $FUT_NMAD_CODE + 0x17;

    our $FUT_NMAD_EVENT_ANNOTATE_CODE		= $FUT_NMAD_CODE + 0x20;

    our %fut_code_mapping;

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

    our %hosts;

#    our $config_size;
#    our $config_rank;
#    our %drivers;
#    our $drv_id;
#    our %gate2rank_mapping;
#    our %rank2gate_mapping;

    print "V -t * -v 1.0a5 -a 0\n";

    my $i = 0;
    print "c -t * -i ${i} -n magenta1\n";		 $i++;
    print "c -t * -i ${i} -n violetred1\n";		 $i++;
    print "c -t * -i ${i} -n red1\n";		 	$i++;
    print "c -t * -i ${i} -n chocolate1\n";		 $i++;
    print "c -t * -i ${i} -n yellow1\n";		 $i++;
    print "c -t * -i ${i} -n chartreuse1\n";		 $i++;
    print "c -t * -i ${i} -n springgreen1\n";		 $i++;
    print "c -t * -i ${i} -n turquoise1\n";		 $i++;
    print "c -t * -i ${i} -n deepskyblue1\n";		 $i++;
    print "c -t * -i ${i} -n slateblue1\n";		 $i++;
    print "c -t * -i ${i} -n magenta2\n";		 $i++;
    print "c -t * -i ${i} -n violetred2\n";		 $i++;
    print "c -t * -i ${i} -n red2\n";		 	$i++;
    print "c -t * -i ${i} -n chocolate2\n";		 $i++;
    print "c -t * -i ${i} -n yellow2\n";		 $i++;
    print "c -t * -i ${i} -n chartreuse2\n";		 $i++;
    print "c -t * -i ${i} -n springgreen2\n";		 $i++;
    print "c -t * -i ${i} -n turquoise2\n";		 $i++;
    print "c -t * -i ${i} -n deepskyblue2\n";		 $i++;
    print "c -t * -i ${i} -n slateblue2\n";		 $i++;
    print "c -t * -i ${i} -n magenta3\n";		 $i++;
    print "c -t * -i ${i} -n violetred3\n";		 $i++;
    print "c -t * -i ${i} -n red3\n";		 	$i++;
    print "c -t * -i ${i} -n chocolate3\n";		 $i++;
    print "c -t * -i ${i} -n yellow3\n";		 $i++;
    print "c -t * -i ${i} -n chartreuse3\n";		 $i++;
    print "c -t * -i ${i} -n springgreen3\n";		 $i++;
    print "c -t * -i ${i} -n turquoise3\n";		 $i++;
    print "c -t * -i ${i} -n deepskyblue3\n";		 $i++;
    print "c -t * -i ${i} -n slateblue3\n";		 $i++;
}

END {
    our $pseudo_time;
    our $pseudo_time_counter;
    our $time;

    # Final flush
    if ($pseudo_time) {
        $pseudo_time_counter++;
        print "T -t ${pseudo_time_counter}\n";
    } else {
        $time++;
        print "T -t ${time}\n";
    }

}

our $link_rate;
our $link_delay;
our $time_divisor;
our $pseudo_time;
our $pseudo_time_counter;
our $annotate_id;
our $preamble;
our $nb_events;

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

our %hosts;

our $time;

sub preamble {
    $preamble = 0;

    my $time_info;
    if ($pseudo_time) {
        $time_info = "${pseudo_time_counter}";
    } else {
        $time_info = "${time}";
    }

    if ($pseudo_time) {
        print "v -t ${time_info} -e set_rate_ext 0.2ms 1\n";
    }

    my $i = 0;

    foreach my $_host_num (keys %hosts) {
        my $_host		= $hosts{$_host_num};
        my $_config_rank	= ${$_host}{'config_rank'};

        print "a -t ${time_info} -n node_${_config_rank} -s ${_config_rank}\n";
        print "v -t ${time_info} -e monitor_agent ${_config_rank} node_${_config_rank}\n";
        $i++;
    }
}

chomp;

my @params	= split ',';
my $num		= shift @params;
my $host_num	= shift @params;
my $ev_time	= Math::BigFloat->new(shift @params);
my $ev_tid	= shift @params;
my $ev_num	= shift @params;
my $ev_nb_params	= shift @params;

next
    unless (($ev_num & 0xff00) == 0xfe00);

$time	= $ev_time->copy()->bdiv($time_divisor);

#printf "$num: \t$host_num - [%16u] %x = %16s \t", $ev_time, $ev_num, $fut_code_mapping{$ev_num};

if ($ev_num == $FUT_NMAD_EVENT_CONFIG_CODE) {
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
} elsif ($ev_num == $FUT_NMAD_EVENT_DRV_ID_CODE) {
    my $host	= $hosts{$host_num};
    my $drivers	= ${$host}{'drivers'};

    my $drv_id	= shift @params;
    ${$host}{'drv_id'}		= $drv_id;

    ${$drivers}{$drv_id}	= {};

    #print "id = $drv_id";
} elsif ($ev_num == $FUT_NMAD_EVENT_DRV_NAME_CODE) {
    my $host	= $hosts{$host_num};
    my $drivers	= ${$host}{'drivers'};

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

    #print "name = $drv_name";
} elsif ($ev_num == $FUT_NMAD_EVENT_NEW_TRK_CODE) {
    my $host	= $hosts{$host_num};
    my $drivers	= ${$host}{'drivers'};

    my $gate_id	= shift @params;
    my $drv_id	= shift @params;
    my $trk_id	= shift @params;

    my $driver	= ${$drivers}{$drv_id};

    my $links	= ${$driver}{'links'};
    ${$links}{"$gate_id:$trk_id"}	= 1;

    my $pending_cnx	= ${$driver}{'pending_cnx'};
    push @{$pending_cnx}, [$gate_id, $drv_id, $trk_id];

    #print "gate id = $gate_id, drv id = $drv_id (${$driver}{'name'}), trk id = $trk_id";
} elsif ($ev_num == $FUT_NMAD_EVENT_CNX_CONNECT_CODE) {
    my $host		= $hosts{$host_num};

    my $drivers		= ${$host}{'drivers'};
    my $gate2rank	= ${$host}{'gate2rank'};
    my $rank2gate	= ${$host}{'rank2gate'};
    my $config_rank	= ${$host}{'config_rank'};

    my $remote_rank	= shift @params;
    my $gate_id		= shift @params;
    my $drv_id		= shift @params;
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

    #print "$config_rank -> $remote_rank, gate id = $gate_id, drv id = $drv_id (${$driver}{'name'})";
    print "l -t ${time_info} -s ${config_rank} -d ${remote_rank} -S UP -c black -r ${link_rate} -D ${link_delay}\n";
    #print "l -t * -s ${config_rank} -d ${remote_rank} -S UP -c black\n";
} elsif ($ev_num == $FUT_NMAD_EVENT_CNX_ACCEPT_CODE) {
    my $host		= $hosts{$host_num};

    my $drivers		= ${$host}{'drivers'};
    my $gate2rank	= ${$host}{'gate2rank'};
    my $rank2gate	= ${$host}{'rank2gate'};
    my $config_rank	= ${$host}{'config_rank'};

    my $remote_rank	= shift @params;
    my $gate_id		= shift @params;
    my $drv_id		= shift @params;
    my $driver	= ${$drivers}{$drv_id};

    unless (exists ${$gate2rank}{$gate_id}) {
        ${$gate2rank}{$gate_id}		= $remote_rank;
        ${$rank2gate}{$remote_rank}	= $gate_id;
    }

    #print "$config_rank <- $remote_rank, gate id = $gate_id, drv id = $drv_id (${$driver}{'name'})";
} elsif ($ev_num == $FUT_NMAD_EVENT_SND_START_CODE) {
    if ($preamble) {
        preamble();
    }

    my $host		= $hosts{$host_num};

    my $drivers		= ${$host}{'drivers'};
    my $gate2rank	= ${$host}{'gate2rank'};
    my $config_rank	= ${$host}{'config_rank'};


    my $gate_id	= shift @params;
    my $drv_id	= shift @params;
    my $trk_id	= shift @params;
    my $length	= shift @params;

    my $remote_rank	= ${$gate2rank}{$gate_id};
    my $driver		= ${$drivers}{$drv_id};
    my $drv_name	= ${$driver}{'name'};

    #print "$config_rank -> $remote_rank, drv id = $drv_id ($drv_name), trk id = $trk_id, length = $length";


    if ($pseudo_time) {
        my $pseudo_length = POSIX::floor(log($length)/log(2));
        print "f -t ${pseudo_time_counter} -s ${config_rank} -a node_${config_rank} -T v -n length -v ${length} -o\n";
        print "+ -t ${pseudo_time_counter} -e ${pseudo_length} -a ${pseudo_length} -s ${config_rank} -d ${remote_rank}\n";
        print "- -t ${pseudo_time_counter} -e ${pseudo_length} -a ${pseudo_length} -s ${config_rank} -d ${remote_rank}\n";
        print "h -t ${pseudo_time_counter} -e ${pseudo_length} -a ${pseudo_length} -s ${config_rank} -d ${remote_rank}\n";
        $pseudo_time_counter += $link_delay * $pseudo_length / ($link_rate * $link_delay);
        print "r -t ${pseudo_time_counter} -e ${pseudo_length} -a ${pseudo_length} -s ${remote_rank} -d ${config_rank}\n";
        $pseudo_time_counter += $link_delay ;
        print "f -t ${pseudo_time_counter} -s ${config_rank} -a node_${config_rank} -T v -n length -o ${length} -x -v\n";
    } else {
        print "f -t ${time} -s ${config_rank} -a node_${config_rank} -T v -n length -v ${length} -o\n";
        print "+ -t ${time} -e ${length} -s ${config_rank} -d ${remote_rank}\n";
        print "- -t ${time} -e ${length} -s ${config_rank} -d ${remote_rank}\n";
        print "h -t ${time} -e ${length} -s ${config_rank} -d ${remote_rank}\n";
    }
} elsif ($ev_num == $FUT_NMAD_EVENT_RCV_END_CODE) {
    if ($preamble) {
        preamble();
    }

    my $host	= $hosts{$host_num};

    my $drivers		= ${$host}{'drivers'};
    my $gate2rank	= ${$host}{'gate2rank'};
    my $config_rank	= ${$host}{'config_rank'};

    my $gate_id	= shift @params;
    my $drv_id	= shift @params;
    my $trk_id	= shift @params;
    my $length	= shift @params;

    my $remote_rank	= ${$gate2rank}{$gate_id};
    my $driver		= ${$drivers}{$drv_id};
    my $drv_name	= ${$driver}{'name'};

    #print "$config_rank <- $remote_rank, drv id = $drv_id ($drv_name), trk id = $trk_id, length = $length";

    unless ($pseudo_time) {
        print "r -t ${time} -e ${length} -s ${remote_rank} -d ${config_rank}\n";
        print "f -t ${time} -s ${remote_rank} -a node_${remote_rank} -T v -n length -o ${length} -x -v\n";
    }
} elsif ($ev_num == $FUT_NMAD_EVENT_ANNOTATE_CODE) {
    my $host	= $hosts{$host_num};
    my $rank	= ${$host}{'config_rank'};
    my $txt	= '';
    foreach my $param (@params) {
        $txt	.= pack ("V", $param);
    }

    ($txt)	= unpack "Z*", $txt;

    if ($pseudo_time) {
        print "v -t ${pseudo_time_counter} -e sim_annotation ${pseudo_time_counter} ${annotate_id} ${rank}: ${txt}\n";
        ${pseudo_time_counter} += $link_delay / 10;
    } else {
        print "v -t ${time} -e sim_annotation ${time} ${annotate_id} ${txt}\n";
    }

    $annotate_id++;
}
#print "\n";
#print STDERR "$num\n"

exit
    if $nb_events && ($. >= $nb_events);
