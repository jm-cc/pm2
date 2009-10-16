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

# parse and display a merged Nmad/FUT trace (text version)

# -n perl flag adds an implicit line-by-line reading loop

use warnings;
use strict;
use Math::BigInt;	# 64bit time stamp processing on 32bit archs

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

BEGIN {

    $FUT_NMAD_CODE	= 0xfe00;

    $FUT_NMAD_EVENT0_CODE	= $FUT_NMAD_CODE + 0x00;
    $FUT_NMAD_EVENT1_CODE	= $FUT_NMAD_CODE + 0x01;
    $FUT_NMAD_EVENT2_CODE	= $FUT_NMAD_CODE + 0x02;
    $FUT_NMAD_EVENTSTR_CODE	= $FUT_NMAD_CODE + 0x03;

    $FUT_NMAD_EVENT_CONFIG_CODE		= $FUT_NMAD_CODE + 0x10;
    $FUT_NMAD_EVENT_DRV_ID_CODE		= $FUT_NMAD_CODE + 0x11;
    $FUT_NMAD_EVENT_DRV_NAME_CODE		= $FUT_NMAD_CODE + 0x12;
    $FUT_NMAD_EVENT_CNX_CONNECT_CODE	= $FUT_NMAD_CODE + 0x13;
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
}

chomp;	# chomp the trailing '\n' from the current line

my @params	= split ',';	# split the line into fields

# read the fields
my $num		= shift @params;
my $host_num	= shift @params;
my $ev_time	= Math::BigInt->new(shift @params);
my $ev_tid	= shift @params;
my $ev_num	= shift @params;
my $ev_nb_params	= shift @params;

# skip unwanted events
next
    unless (($ev_num & 0xff00) == 0xfe00);

# process and display events
printf "$num: \t$host_num - [%16s] %x = %16s \t", $ev_time, $ev_num, $fut_code_mapping{$ev_num};

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

    print "$config_rank/$config_size";
} else {
    my $host		= $hosts{$host_num};
    my $config_rank	= ${$host}{'config_rank'};
    my $gate2rank	= ${$host}{'gate2rank'};
    my $rank2gate	= ${$host}{'rank2gate'};
    my $drivers		= ${$host}{'drivers'};


    if ($ev_num == $FUT_NMAD_EVENT_DRV_ID_CODE) {
        my $drv_id	= shift @params;
        ${$host}{'drv_id'}		= $drv_id;

        ${$drivers}{$drv_id}	= {};

        print "id = $drv_id";
    } elsif ($ev_num == $FUT_NMAD_EVENT_DRV_NAME_CODE) {
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

        ($drv_name)	= unpack "Z*", $drv_name;

        ${$driver}{'name'}		= $drv_name;
        ${$driver}{'links'}		= {};
        ${$driver}{'pending_cnx'}	= [];

        delete ${$host}{'drv_id'};

        print "name = $drv_name";
    } elsif ($ev_num == $FUT_NMAD_EVENT_NEW_TRK_CODE) {
        my $gate_id	= shift @params;
        my $drv_id	= shift @params;
        my $trk_id	= shift @params;

        my $driver	= ${$drivers}{$drv_id};

        my $links	= ${$driver}{'links'};
        ${$links}{"$gate_id:$trk_id"}	= 1;

        my $pending_cnx	= ${$driver}{'pending_cnx'};
        push @{$pending_cnx}, [$gate_id, $drv_id, $trk_id];

        print "gate id = $gate_id, drv id = $drv_id (${$driver}{'name'}), trk id = $trk_id";
    } elsif ($ev_num == $FUT_NMAD_EVENT_CNX_CONNECT_CODE or $ev_num == $FUT_NMAD_EVENT_CNX_ACCEPT_CODE) {
        my $remote_rank	= shift @params;
        my $gate_id		= shift @params;
        my $drv_id		= shift @params;
        my $driver	= ${$drivers}{$drv_id};

        unless (exists ${$gate2rank}{$gate_id}) {
            ${$gate2rank}{$gate_id}		= $remote_rank;
            ${$rank2gate}{$remote_rank}	= $gate_id;
        }

        my $symb = ($ev_num == $FUT_NMAD_EVENT_CNX_CONNECT_CODE)?'->':'<-';
        print "$config_rank $symb $remote_rank, gate id = $gate_id, drv id = $drv_id (${$driver}{'name'})";
    } elsif ($ev_num == $FUT_NMAD_EVENT_SND_START_CODE or $ev_num == $FUT_NMAD_EVENT_RCV_END_CODE) {
        my $gate_id	= shift @params;
        my $drv_id	= shift @params;
        my $trk_id	= shift @params;
        my $length	= shift @params;

        my $remote_rank	= ${$gate2rank}{$gate_id};
        my $driver	= ${$drivers}{$drv_id};
        my $drv_name	= ${$driver}{'name'};

        my $symb = ($ev_num == $FUT_NMAD_EVENT_SND_START_CODE)?'->':'<-';

        print "$config_rank $symb $remote_rank, drv id = $drv_id ($drv_name), trk id = $trk_id, length = $length";

    } elsif ($ev_num == $FUT_NMAD_EVENT_ANNOTATE_CODE) {
        my $txt	= '';
        foreach my $param (@params) {
            $txt	.= pack ("V", $param);
        }

        ($txt)	= unpack "Z*", $txt;

        print "-*- ${config_rank}: ${txt} -*-";
    }
}

print "\n";
